#include <stdio.h> // fprintf, fflush, read, STDIN_FILENO, perror
#include <stdlib.h> // malloc, realloc, free, execvp, exit, EXIT_SUCCESS, atexit
#include <unistd.h> // fork, chdir, STDOUT_FILENO, getcwd
#include <string.h> // strcmp, strerror, memset
#include <stddef.h> // for NULL, size_t (unsigned integer)
#include <sys/wait.h> // wait
#include <errno.h> // access the errno variable
#include <termios.h> // to read character by character, tcgetattr, tcsetattr, TCSAFLUSH
#include <signal.h> // to handle Ctrl+C

#define STR_BUFFER 16 // starting buffer for input string
#define CMD_LINE_BUFFER 16 // starting buffer for args array
#define NEWLINE '\n'
#define NULLCHAR '\0'
#define SHELL_NAME "\033[1;34mJBash> \033[0m" //  Style: Bold; Color mode: Blue;
#define DEBUG 0

static struct termios original_tio; // Original terminal settings
char **args; // pointer to pointers of null terminating strings
char *inputString; // current string
char *cwd;

int execute(char **args);
char** parse(void);
void print_prompt();
void* realloc_buffer(void *ptr, size_t *current_buffer);
void* realloc_leftover_string(char *inputString, size_t *string_length);
void *safe_malloc(size_t size);
void free_args(char **args);
void disable_raw_mode();
void enable_raw_mode();
void handle_sigint(int sig);