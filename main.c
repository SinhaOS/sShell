#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

void parse_input(char *input, char **args) {
    int i = 0;

    args[i] = strtok(input, " \t\n");

    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }

    args[i] = NULL;
}

int main() {
    char input[MAX_LINE];
    char *args[MAX_ARGS];

    while (1) {
        printf("Sinha Shell");
        printf("© 2026 Sinha Group. All rights reserved.\n");
        printf("sShell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        parse_input(input, args);

        if (args[0] == NULL)
            continue;

        /* builtin: exit */
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        /* builtin: cd */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL)
                fprintf(stderr, "cd: missing argument\n");
            else if (chdir(args[1]) != 0)
                perror("cd");
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            execvp(args[0], args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0) {
            wait(NULL);
        }
        else {
            perror("fork failed");
        }
    }

    return 0;
}