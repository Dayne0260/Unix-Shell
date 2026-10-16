
#include "get_path.h"

int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
int is_builtin(char *cmd);
int expand_wildcards(char **raw_args, int raw_count, char **out_args, int max_args);
void free_expanded_args(char **out_args, int count);

#define PROMPTMAX 32
#define MAXARGS 10
