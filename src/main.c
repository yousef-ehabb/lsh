#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char **environ;

#define LSH_HISTORY_MAX 100
static char *lsh_hist[LSH_HISTORY_MAX];
static int lsh_hist_count = 0;

void lsh_add_history(const char *line) {
  if (lsh_hist_count < LSH_HISTORY_MAX) {
    lsh_hist[lsh_hist_count++] = strdup(line);
  }
}

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_export(char **args);

char *builtin_str[] = {
  "cd", "help", "exit", "pwd", "echo", "history", "export"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd, &lsh_help, &lsh_exit, &lsh_pwd, &lsh_echo, &lsh_history, &lsh_export
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) perror("lsh");
  }
  return 1;
}

int lsh_help(char **args) {
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");
  for (i = 0; i < lsh_num_builtins(); i++) printf("  %s\n", builtin_str[i]);
  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args) { return 0; }

int lsh_pwd(char **args) {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
  else perror("lsh: pwd");
  return 1;
}

int lsh_echo(char **args) {
  int i = 1;
  if (args[1] == NULL) { printf("\n"); return 1; }
  while (args[i] != NULL) {
    printf("%s", args[i]);
    if (args[i+1] != NULL) printf(" ");
    i++;
  }
  printf("\n");
  return 1;
}

int lsh_history(char **args) {
  int i;
  if (lsh_hist_count == 0) printf("No commands in history.\n");
  else for (i = 0; i < lsh_hist_count; i++) printf("%d  %s\n", i+1, lsh_hist[i]);
  return 1;
}

int lsh_export(char **args) {
  if (args[1] == NULL) { fprintf(stderr, "lsh: export: expected VAR=VALUE\n"); return 1; }
  char *eq = strchr(args[1], '=');
  if (eq == NULL) { fprintf(stderr, "lsh: export: use VAR=VALUE format\n"); return 1; }
  *eq = '\0';
  if (setenv(args[1], eq+1, 1) != 0) perror("lsh: export");
  return 1;
}

int lsh_launch(char **args) {
  pid_t pid; int status;
  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) perror("lsh");
    exit(EXIT_FAILURE);
  } else if (pid < 0) perror("lsh");
  else { do { waitpid(pid, &status, WUNTRACED); } while (!WIFEXITED(status) && !WIFSIGNALED(status)); }
  return 1;
}

int lsh_execute(char **args) {
  int i;
  if (args[0] == NULL) return 1;
  for (i = 0; i < lsh_num_builtins(); i++)
    if (strcmp(args[0], builtin_str[i]) == 0) return (*builtin_func[i])(args);
  return lsh_launch(args);
}

char *lsh_read_line(void) {
  int bufsize = 1024, position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;
  if (!buffer) { fprintf(stderr, "lsh: allocation error\n"); exit(EXIT_FAILURE); }
  while (1) {
    c = getchar();
    if (c == EOF) exit(EXIT_SUCCESS);
    else if (c == '\n') { buffer[position] = '\0'; return buffer; }
    else buffer[position] = c;
    position++;
    if (position >= bufsize) {
      bufsize += 1024;
      buffer = realloc(buffer, bufsize);
      if (!buffer) { fprintf(stderr, "lsh: allocation error\n"); exit(EXIT_FAILURE); }
    }
  }
}

char **lsh_split_line(char *line) {
  int bufsize = 64, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  if (!tokens) { fprintf(stderr, "lsh: allocation error\n"); exit(EXIT_FAILURE); }
  token = strtok(line, " \t\r\n\a");
  while (token != NULL) {
    tokens[position++] = token;
    if (position >= bufsize) {
      bufsize += 64; tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) { free(tokens_backup); fprintf(stderr, "lsh: allocation error\n"); exit(EXIT_FAILURE); }
    }
    token = strtok(NULL, " \t\r\n\a");
  }
  tokens[position] = NULL;
  return tokens;
}

void lsh_loop(void) {
  char *line; char **args; int status;
  do {
    printf("> ");
    line = lsh_read_line();
    if (strlen(line) > 0) lsh_add_history(line);
    args = lsh_split_line(line);
    status = lsh_execute(args);
    free(line); free(args);
  } while (status);
}

int main(int argc, char **argv) {
  lsh_loop();
  return EXIT_SUCCESS;
}
