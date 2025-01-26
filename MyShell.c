/***************************************************************************//**
  @file         MyShell.c
  @author       Juhua Hu, Jeremiah Brenio

*******************************************************************************/

/**
 * @file MyShell.c
 * @brief Core header files for shell implementation
 * 
 */
#include <stdio.h> // fprintf, getc
#include <stdlib.h> // malloc, realloc, exit
#include <unistd.h> // fork & exec
#include <string.h> // strcmp, strerror
#include <stddef.h> // for NULL
#include <sys/wait.h> // wait
#include <errno.h> // access the errno variable


/**
DONOT change the existing function definitions. You can add functions, if necessary.
*/

/*
    Note:
    ChatGPT and man pages were only used to understand the functions of the included libraries
    AND to provide most of the documentation.
*/

#define EXIT_SUCCESS 0
#define MAX_STR_BUFFER 256
#define CMD_LINE_BUFFER 1024
#define NEWLINE '\n'
#define NULLCHAR '\0'

int execute(char **args);
char** parse(void);
void free_leftover_string(char *token, int string_length);
void *safe_malloc(size_t size);
void free_args(char **args);

/**
   @brief Main function should run infinitely until terminated manually using CTRL+C or typing in the exit command
   It should call the parse() and execute() functions
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
    char **args; // pointer to pointers of null terminating strings
    int status;
    while (1) {
        fprintf(stdout, "MyShell> ");
        args = parse();
        status = execute(args);
        if (status == 0) {
            free_args(args);
            fprintf(stdout, "exiting...\n");
            break;
        }
        if (status == 1) {
            free_args(args);
        }
    }

  return EXIT_SUCCESS;
}

/**
  @brief Fork a child to execute the command using execvp. The parent should wait for the child to terminate
  @param args Null terminated list of arguments (including program).
  @return returns 1, to continue execution and 0 to terminate the MyShell prompt.
 */
int execute(char **args)
{
    printf("Executing command: %s\n", args[0]);
    if (strcmp(args[0], "exit") == 0) { // command exit check to terminate shell
        return 0;
    }
    int rc = fork();
    if (rc == -1) {
        fprintf(stderr, "fork failed\n");
        return 0;
    } else if (rc == 0) {
        int status = execvp(args[0], args);
        if (status = -1) {
            fprintf(stderr, "Failure to execute command: %s\n", strerror(errno));
            exit(1);
        }
    } else {
        wait(NULL);
        return 1;
    }
}

// TODO: account for quotes/other delimiters
/**
  @brief gets the input from the prompt and splits it into tokens. Prepares the arguments for execvp
  @return returns char** args to be used by execvp
 */
char** parse(void)
{
    char ch;
    size_t command_line_buffer_size = CMD_LINE_BUFFER;
    size_t token_buffer_size = MAX_STR_BUFFER;
    char **args = safe_malloc(sizeof(char *) * command_line_buffer_size);
    char *token = safe_malloc(sizeof(char) * token_buffer_size);
    int string_length = 0, array_length = 0;
    int first_space = 0; // true = 1, false = 0
    // getc consumes a character per input
    while ((ch = getc(stdin)) != EOF) {
        if (ch == NEWLINE && !token[0]) {
            fprintf(stdout, "MyShell> ");
        } else if (ch == NEWLINE) {
            token[string_length] = NULLCHAR;
            *(args + array_length) = token; // store the last token
            array_length++;
            *(args + array_length) = NULL; // null-terminate the args array
            break;
        } else if (ch == ' ' && !first_space) { // add new token when a space is pressed
            token[string_length] = NULLCHAR;
            *(args + array_length) = token;
            array_length++;
            token = safe_malloc(sizeof(char) * token_buffer_size);
            string_length = 0;
            first_space = 1;
        } else if (ch != ' ') {
            token[string_length] = ch;
            string_length++;
            first_space = 0;
        }
    }

    return args;
}

/**
 * Frees memory allocated for command line arguments.
 * 
 * @param args Double pointer to array of command line argument strings to be freed
 *             Each individual argument string and the array itself will be freed
 *             Array must be NULL terminated
 *             
 * @note Assumes args is a valid pointer to a NULL-terminated array of strings
 *       Each string in the array must have been dynamically allocated
 */
void free_args(char **args)
{
    int i = 0;
    while (args[i] != NULL) {
        free(args[i]);
        i++;
    }
    free(args);
}

/**
 * Allocate SIZE bytes of memory with error checking.
 * 
 * @param token The string buffer to resize
 * @param string_length The number of bytes + 1 to reallocate
 * @note Exits with status 1 if memory reallocation fails
 */
void free_leftover_string(char *token, int string_length) {
    char *resized_token = realloc(token, string_length + 1); // manage unused memory
    if (resized_token == NULL) {
        fprintf(stderr, "Failed to reallocate memory");
        free(token); // Free original buffer if realloc fails
        exit(1);
    }
    token = resized_token;
    return;
}

/**
 * Allocate SIZE bytes of memory with error checking.
 * 
 * @param size The number of bytes to allocate
 * @return A generic pointer to the allocated memory if successful
 * @note Exits with status 1 if memory allocation fails
 */
void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed for size %zu\n", size);
        exit(1);
    }
    return ptr;
}