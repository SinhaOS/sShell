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

/* User lookup via the host system account database */
int shell_user_exists(const char *username);
const char *shell_default_user(void);

#endif /* MAIN_H */
