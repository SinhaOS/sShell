#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "exec.h"
#include "ast.h"

/* Function to display the shell prompt */
void print_prompt(void);

/* Set the current prompt prefix */
void set_prompt_prefix(const char *prefix);

/* User management for shell-local authentication */
int add_shell_user(const char *username, const char *password);
int shell_user_exists(const char *username);
int authenticate_shell_user(const char *username, const char *password);
char *read_password_prompt(const char *prompt);

#endif /* MAIN_H */
