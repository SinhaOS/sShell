#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"
#include "exec.h"
#include "ast.h"
#include "main.h"

typedef struct {
    char *username;
    char *password;
} ShellUser;

static char *custom_prompt_prefix = NULL;
static ShellUser *shell_users = NULL;
static size_t shell_user_count = 0;
static size_t shell_user_capacity = 0;

static void free_shell_users(void) {
    for (size_t i = 0; i < shell_user_count; ++i) {
        free(shell_users[i].username);
        free(shell_users[i].password);
    }

    free(shell_users);
    shell_users = NULL;
    shell_user_count = 0;
    shell_user_capacity = 0;
}

static ShellUser *find_shell_user(const char *username) {
    if (!username || !*username) {
        return NULL;
    }

    for (size_t i = 0; i < shell_user_count; ++i) {
        if (strcmp(shell_users[i].username, username) == 0) {
            return &shell_users[i];
        }
    }

    return NULL;
}

static char *read_line_prompt(const char *prompt) {
    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;

    fputs(prompt, stdout);
    fflush(stdout);

    nread = getline(&line, &cap, stdin);

    if (nread < 0) {
        free(line);
        return NULL;
    }

    if (nread > 0 && line[nread - 1] == '\n') {
        line[nread - 1] = '\0';
    }

    return line;
}

int add_shell_user(const char *username, const char *password) {
    if (!username || !*username || !password) {
        return 1;
    }

    if (find_shell_user(username)) {
        return 1;
    }

    if (shell_user_count == shell_user_capacity) {
        size_t new_capacity = shell_user_capacity == 0 ? 4 : shell_user_capacity * 2;
        ShellUser *new_users = realloc(shell_users, new_capacity * sizeof(*shell_users));

        if (!new_users) {
            return 1;
        }

        shell_users = new_users;
        shell_user_capacity = new_capacity;
    }

    shell_users[shell_user_count].username = strdup(username);
    shell_users[shell_user_count].password = strdup(password);

    if (!shell_users[shell_user_count].username || !shell_users[shell_user_count].password) {
        free(shell_users[shell_user_count].username);
        free(shell_users[shell_user_count].password);
        return 1;
    }

    shell_user_count++;
    return 0;
}

int shell_user_exists(const char *username) {
    return find_shell_user(username) != NULL;
}

int authenticate_shell_user(const char *username, const char *password) {
    ShellUser *user = find_shell_user(username);

    if (!user || !password) {
        return 0;
    }

    return strcmp(user->password, password) == 0;
}

char *read_password_prompt(const char *prompt) {
    struct termios old_termios;
    struct termios new_termios;
    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    int restore_terminal = 0;

    fputs(prompt, stdout);
    fflush(stdout);

    if (tcgetattr(STDIN_FILENO, &old_termios) == 0) {
        new_termios = old_termios;
        new_termios.c_lflag &= (tcflag_t)~ECHO;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == 0) {
            restore_terminal = 1;
        }
    }

    nread = getline(&line, &cap, stdin);

    if (restore_terminal) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios);
    }

    fputc('\n', stdout);

    if (nread < 0) {
        free(line);
        return NULL;
    }

    if (nread > 0 && line[nread - 1] == '\n') {
        line[nread - 1] = '\0';
    }

    return line;
}

static int login_shell_user(void) {
    while (1) {
        char *username = read_line_prompt("login: ");
        char *password;

        if (!username) {
            return 1;
        }

        password = read_password_prompt("Password: ");

        if (!password) {
            free(username);
            return 1;
        }

        if (authenticate_shell_user(username, password)) {
            set_prompt_prefix(username);
            free(username);
            free(password);
            return 0;
        }

        fprintf(stderr, "[zynk] Login incorrect.\n");
        free(username);
        free(password);
    }
}

/* Display prompt */

void print_prompt(void) {
    char cwd[1024];
    const char *dir = getcwd(cwd, sizeof(cwd)) ? cwd : "?";
    char buf[1280];
    if (custom_prompt_prefix && *custom_prompt_prefix) {
        snprintf(buf, sizeof(buf), "%s:%s/sShell> ", custom_prompt_prefix, dir);
    } else {
        snprintf(buf, sizeof(buf), "%s/sShell> ", dir);
    }
    fputs(buf, stdout);
    fflush(stdout);
}

/* Set the current prompt prefix */
void set_prompt_prefix(const char *prefix) {
    free(custom_prompt_prefix);
    custom_prompt_prefix = prefix ? strdup(prefix) : strdup("");
}
int main(void) {

    char *line = NULL;
    size_t cap = 0;

    add_shell_user("root", "password");

    printf("Sinha OS\n");

    if (login_shell_user() != 0) {
        free_shell_users();
        free(custom_prompt_prefix);
        free(line);
        return 1;
    }

    while (1) {

        print_prompt();

        ssize_t nread = getline(&line, &cap, stdin);

        if (nread < 0) {

            if (feof(stdin)) {

                putchar('\n');

                break;

            }

            perror("[sShell] Error");

            break;

        }

        if (nread == 0) continue;

        // Strip trailing newline
        if (line[nread - 1] == '\n') line[nread - 1] = '\0';

        if (line[0] == '\0') continue;

        TokenVector tokens;

        if (lex(line, &tokens) != 0) {

            token_vector_free(&tokens);

            continue;

        }

        AstSequence *sequence = parse_tokens(&tokens);

        if (!sequence) {

            token_vector_free(&tokens);

            continue;

        }

        int status = exec_sequence(sequence);

        free_ast(sequence);
        token_vector_free(&tokens);

        if (status == SHELL_STATUS_EXIT) break;

    }

    free_shell_users();
    free(custom_prompt_prefix);
    free(line);

    return 0;

}
