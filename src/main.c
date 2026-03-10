#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"
#include "exec.h"
#include "ast.h"
#include "main.h"

static char *custom_prompt_prefix = NULL;

int shell_user_exists(const char *username) {
    if (!username || !*username) {
        return 0;
    }

    return getpwnam(username) != NULL;
}

const char *shell_default_user(void) {
    struct passwd *pw = getpwuid(getuid());
    return pw && pw->pw_name ? pw->pw_name : "user";
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

    set_prompt_prefix(shell_default_user());
    printf("Sinha OS\n");

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

    free(custom_prompt_prefix);
    free(line);

    return 0;

}
