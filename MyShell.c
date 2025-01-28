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
#define MAX_STR_BUFFER 16
#define CMD_LINE_BUFFER 16
#define NEWLINE '\n'
#define NULLCHAR '\0'

int execute(char **args);
char** parse(void);
void* realloc_buffer(void *ptr, size_t *current_buffer);
void* realloc_leftover_string(char *token, size_t string_length);
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
        free_args(args);
        if (status == 0) {
            fprintf(stdout, "exiting...\n");
            break;
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
    //printf("Executing command: %s\n", args[0]);
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

/**
  @brief gets the input from the prompt and splits it into tokens. Prepares the arguments for execvp
  @return returns char** args to be used by execvp
 */
char** parse(void)
{
    char ch;
    size_t command_line_buffer_length = CMD_LINE_BUFFER;
    size_t token_buffer_length = MAX_STR_BUFFER;
    char **args = safe_malloc(sizeof(char *) * command_line_buffer_length);
    char *token = safe_malloc(sizeof(char) * token_buffer_length);
    size_t string_length = 0, array_length = 0;
    int first_space = 0; // true = 1, false = 0
    // getc consumes a character per input
    while ((ch = getc(stdin)) != EOF) {
        if (string_length + 1 >= token_buffer_length) {
            token = realloc_buffer(token, &token_buffer_length);
        }
        if (array_length + 1 >= command_line_buffer_length) {
            args = realloc_buffer(args, &command_line_buffer_length);
        }

        if (ch == NEWLINE && !token[0]) {
            fprintf(stdout, "MyShell> ");
        } else if (ch == NEWLINE) { // finalize command line
            token[string_length] = NULLCHAR;
            token = realloc_leftover_string(token, string_length);
            *(args + array_length) = token; // store the last token
            array_length++;
            *(args + array_length) = NULL; // null-terminate the args array
            break;
        } else if (ch == ' ' && !first_space) { // add new token when a space is pressed
            token[string_length] = NULLCHAR;
            if (string_length < token_buffer_length) {
                token = realloc_leftover_string(token, string_length);
            }
            *(args + array_length) = token;
            array_length++;
            token_buffer_length = MAX_STR_BUFFER;
            token = safe_malloc(sizeof(char) * token_buffer_length);
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
 * Reallocates the current buffer memory with error checking.
 * 
 * @param ptr The pointer buffer to resize
 * @param current_buffer The current size of the buffer
 * @note Exits with status 1 if memory reallocation fails
 */
void* realloc_buffer(void *ptr, size_t *current_buffer) {
    *current_buffer *= 2;
    char *new_ptr = realloc(ptr, sizeof(ptr) * *current_buffer); // increase
    if (new_ptr == NULL) {
        fprintf(stderr, "Memory allocation failed for size %zu\n", *current_buffer);
        free(ptr); // Free original buffer if realloc fails
        exit(1);
    }
    return new_ptr;
}

/**
 * Reallocates the token with with error checking.
 * 
 * @param token The string buffer to resize
 * @param string_length The length of the current token
 * @note Exits with status 1 if memory reallocation fails
 */
void* realloc_leftover_string(char *token, size_t string_length) {
    char *resized_token = realloc(token, sizeof(token) * (string_length + 1)); // manage unused memory
    if (resized_token == NULL) {
        fprintf(stderr, "Memory allocation failed for size %zu\n", string_length + 1);
        free(token); // Free original buffer if realloc fails
        exit(1);
    }
    return resized_token;
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