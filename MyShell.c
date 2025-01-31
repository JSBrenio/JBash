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
#include <stdlib.h> // malloc, realloc, free, execvp, exit, EXIT_SUCCESS
#include <unistd.h> // fork
#include <string.h> // strcmp, strerror
#include <stddef.h> // for NULL
#include <sys/wait.h> // wait
#include <errno.h> // access the errno variable
#include <termios.h> // to read character by character


/**
DONOT change the existing function definitions. You can add functions, if necessary.
*/

/*
    Note:
    ChatGPT and man pages were only used to understand the functions of the included libraries
    AND to provide most of the documentation.
*/

#define MAX_STR_BUFFER 16
#define CMD_LINE_BUFFER 16
#define NEWLINE '\n'
#define NULLCHAR '\0'

int execute(char **args);
char** parse(void);
void* realloc_buffer(void *ptr, size_t *current_buffer);
void* realloc_leftover_string(char *string, size_t string_length);
void *safe_malloc(size_t size);
void free_args(char **args);
void disable_raw_mode();
void enable_raw_mode();
void handle_up_key();

static struct termios original_tio;

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
        // Default stdout buffer size is typically 4096 bytes
        printf("MyShell> "); // Writes to buffer
        fflush(stdout);      // Forces immediate display of prompt
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
    // int i = 0;
    // while (args[i] != NULL) {
    //     printf("arg[%d]:  %s\n", i, args[i]);
    //     i++;
    // }
    if (strcmp(args[0], "exit") == 0) { // command 'exit' check to terminate shell
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
    size_t string_buffer_length = MAX_STR_BUFFER;
    char **args = safe_malloc(sizeof(char *) * command_line_buffer_length);
    char *string = safe_malloc(sizeof(char) * string_buffer_length);
    size_t string_length = 0, array_length = 0;
    int first_space = 0; // true = 1, false = 0
    size_t cursor = 0;
    enable_raw_mode();
    while (read(STDIN_FILENO, &ch, 1) == 1) {
        // buffer checks
        if (string_length + 1 >= string_buffer_length) {
            string = realloc_buffer(string, &string_buffer_length);
        }
        if (ch == NEWLINE && !string[0]) {
            fprintf(stdout, "\nMyShell> ");
        }  else if (ch == 0x03) {  // Ctrl+C
            fprintf(stdout, "^C\n");
            exit(1);
        } else if (ch == '\033') { // terminal sends 3 bytes in sequence
            char seq[3]; // seq[0] = '[', seq[1] = Letter code
            if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) break;

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': // Up arrow
                        break;
                    case 'B': // Down arrow
                        break;
                    case 'C': // Right arrow
                        if (cursor < string_length) {
                            fprintf(stdout, "\033[1C");  // Move cursor right
                            cursor++;
                        }
                        break;
                    case 'D': // Left arrow
                        if (cursor > 0) {
                            fprintf(stdout, "\033[1D");  // Move cursor left
                            cursor--;
                        }
                        break;
                }
            }
        } else if (ch == NEWLINE) { // finalize command line
            fprintf(stdout, "\r\n");  // Move to next line
            string = realloc_leftover_string(string, string_length);
            string[string_length] = NULLCHAR;
            *(args) = string;
            *(args + 1) = NULL; // null-terminate the args array
            break;
        } else if ((ch == 127 || ch == '\b') && cursor != 0) { // handle back spacing
            memmove(&string[cursor-1], &string[cursor], string_length - cursor); // truncate string of deleted char
            string_length--;
            cursor--;

            // Update display
            fprintf(stdout, "\b");  // Move back
            // Reprint rest of string
            for (size_t i = cursor; i < string_length; i++) {
                fprintf(stdout, "%c", string[i]);
            }
            fprintf(stdout, " ");  // Clear character
            
            // Reset cursor position
            fprintf(stdout, "\033[%zuD", string_length - cursor + 1);
        } else {
            if (cursor < string_length) {
                // Shift characters right
                memmove(&string[cursor + 1], &string[cursor], string_length - cursor);
                
                // Insert new character
                string[cursor] = ch;
                string_length++;
                cursor++;
                
                // Update display
                fprintf(stdout, "%c", ch);           // Print new char
                fprintf(stdout, "\033[K");           // Clear line after cursor
                fprintf(stdout, "%s", &string[cursor]); // Print rest of string
                fprintf(stdout, "\033[%zuD", string_length - cursor); // Reset cursor
            } else {
                fprintf(stdout, "%c", ch);
                string[cursor] = ch;
                cursor++;
                string_length++;
            }
        }
        fflush(stdout); // flushes character out i.e. prints what's queued up.
    }
    disable_raw_mode();
    string = realloc_leftover_string(string, string_length);

    char *word_start = string;  // Track start of current word
    for (int i = 0; i < string_length; i++) {
        if (array_length + 1 >= command_line_buffer_length) {
            args = realloc_buffer(args, &command_line_buffer_length);
        }
        
        if (string[i] == ' ') {
            string[i] = '\0';  // Null terminate word
            args[array_length] = word_start;  // Add to args
            array_length++;
            word_start = &string[i + 1];  // Start of next word
        }
    }

    // Add final word if exists
    if (word_start[0] != '\0') {
        args[array_length] = word_start;
        array_length++;
    }
    args[array_length] = NULL;  // Null terminate args array

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
    // Free the first string (original buffer)
    if (args[0] != NULL) {
        free(args[0]);
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
 * @param string The string buffer to resize
 * @param string_length The length of the current token
 * @note Exits with status 1 if memory reallocation fails
 */
void* realloc_leftover_string(char *string, size_t string_length) {
    char *resized_string = realloc(string, sizeof(string) * (string_length + 1)); // manage unused memory
    if (resized_string == NULL) {
        fprintf(stderr, "Memory allocation failed for size %zu\n", string_length + 1);
        free(string); // Free original buffer if realloc fails
        exit(1);
    }
    return resized_string;
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

/**
 * @brief Disables raw mode and restores the terminal to its original settings
 * This function is called when exiting the program to restore normal terminal behavior
 */
void disable_raw_mode() {
    // Attempt to restore original terminal settings. TCSAFLUSH will wait for all output to be transmitted
    // and discards any unread input before applying the changes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_tio) == -1) {
        perror("tcsetattr"); // Print error message if restoration fails
    }
}

/**
 * @brief Enables raw mode for terminal input
 * Raw mode allows reading input character by character without waiting for Enter
 * and without showing typed characters (no echo)
 */
void enable_raw_mode() {
    // Save the original terminal settings so we can restore them later
    if (tcgetattr(STDIN_FILENO, &original_tio) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    
    // Register disable_raw_mode to be called automatically when program exits
    atexit(disable_raw_mode);

    // Create new terminal settings based on original ones
    struct termios raw = original_tio;
    
    // Modify settings:
    // ICANON - Disable canonical mode (input is processed character by character)
    // ECHO - Disable automatic echo of input characters
    // using negation and logical AND to change bitmask flags
    raw.c_lflag &= ~(ICANON | ECHO);

    // Apply the new settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}