/*******************************************************************************
  @file         MyShell.c
  @author       Juhua Hu, Jeremiah Brenio

*******************************************************************************/

/**
 * @file MyShell.c
 * @brief Core header files for shell implementation
 * 
 */
#include <stdio.h> // fprintf, fflush, read, STDIN_FILENO, perror
#include <stdlib.h> // malloc, realloc, free, execvp, exit, EXIT_SUCCESS, atexit
#include <unistd.h> // fork, chdir, STDOUT_FILENO, getcwd
#include <string.h> // strcmp, strerror, memset
#include <stddef.h> // for NULL, size_t (unsigned integer)
#include <sys/wait.h> // wait
#include <errno.h> // access the errno variable
#include <termios.h> // to read character by character, tcgetattr, tcsetattr, TCSAFLUSH
#include <signal.h> // to handle Ctrl+C

/**
DONOT change the existing function definitions. You can add functions, if necessary.
*/

/*
    Note:
    AI and man pages were only used to research/understand the functions of the included libraries
    AND to provide most of the documentation.
*/

#define STR_BUFFER 16 // starting buffer for input string
#define CMD_LINE_BUFFER 16 // starting buffer for args array
#define NEWLINE '\n'
#define NULLCHAR '\0'
#define SHELL_NAME "\033[1;34mMyShell> \033[0m" //  Style: Bold; Color mode: Blue;

int execute(char **args);
char** parse(void);
void* realloc_buffer(void *ptr, size_t *current_buffer);
void* realloc_leftover_string(char *string, size_t *string_length);
void *safe_malloc(size_t size);
void free_args(char **args);
void disable_raw_mode();
void enable_raw_mode();
void handle_sigint(int sig);

static struct termios original_tio; // Original terminal settings

/**
   @brief Main function should run infinitely until terminated manually using CTRL+C or typing in the exit command
   It should call the parse() and execute() functions
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{   
    signal(SIGINT, handle_sigint); // Set up signal handler for Ctrl+C (SIGINT)
    char **args; // pointer to pointers of null terminating strings
    int status; // status to check return of execute
    while (1) {
        /*****************************************************
        - MyShell> is displayed correctly
        - MyShell> is displayed after executing a command 
        - MyShell> is displayed after execvp fails 
        - There is code to display MyShell> prompt infinitely, 
          except when exit command or CTRL+C is given
        *****************************************************/
        printf(SHELL_NAME);
        fflush(stdout); // Forces immediate display of prompt
        /**************************
        MyShell> accepts user input
        **************************/
        args = parse();
        status = execute(args);
        free_args(args); // free **args for next use
        /***********************************************
        MyShell> is not displayed after the exit command
        ***********************************************/
        if (status == 0) { // exit
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
    int rv = 1; // return value, 1 by default, set to 0 for termination.
    // FOR DEBUGGING
    // int i = 0;
    // while (args[i] != NULL) {
    //     printf("arg[%d]:  %s\n", i, args[i]);
    //     i++;
    // }

    if (args[0] == NULL) {} // invalid input i.e. all whitespace, do nothing
    /**********************************************
    When the user types exit the program terminates
    **********************************************/
    else if (strcmp(args[0], "exit") == 0) { // command 'exit' check to terminate shell
        rv = 0; // trigger termination
    }
    else if (strcmp(args[0], "cd") == 0) { // command 'cd' to change directory of current process
        int status;
        if (args[1] == NULL) { // try to default to home when given no argument for cd
            status = chdir(getenv("HOME")); // chdir sys call to change path
        } else {
            status = chdir(args[1]);
        }

        if (status == 0) {
            // FOR DEBUGGING
            // char *cwd = getcwd(NULL, 0);
            // fprintf(stdout, "Current Working Directory: %s\n", cwd);
            // free(cwd);
        } else {
            perror("Failure to Change Directory");
        }
    } else {
        // for non-shell implemented system calls
        int rc = fork();
        if (rc == -1) {
            perror("Fork failed");
            rv = 0; // trigger termination
        } else if (rc == 0) {
            /***************************************************************************
            - The command user types are executed, and the results are displayed
            - The execvp command is appropriately implemented with the correct arguments
            ***************************************************************************/
            int status = execvp(args[0], args);
            if (status == -1) {
                perror("Failure to Execute Command");
                // free allocated memory of child process heap
                free_args(args);
                exit(EXIT_FAILURE);
            }
        } else {
            /******************************************************
            After fork, the parent waits for the child to terminate
            ******************************************************/
            wait(NULL);
        }
    }
    return rv;
}

/**
  @brief gets the input from the prompt and splits it into tokens. Prepares the arguments for execvp
  @return returns char** args to be used by execvp
 */
char** parse(void)
{
    // character of each keystroke input
    char ch;
    // Starting buffer sizes
    size_t command_line_buffer_length = CMD_LINE_BUFFER;
    size_t string_buffer_length = STR_BUFFER;
    // allocate single string and array of tokens to heap.
    char **args = safe_malloc(sizeof(char *) * command_line_buffer_length);
    char *string = safe_malloc(sizeof(char) * string_buffer_length);
    // Initialize the allocated memory with initial values with memset 
    // which is similar to calloc to make sure there are no garbage values
    memset(args, 0, sizeof(char *) * command_line_buffer_length);
    memset(string, 0, sizeof(char) * string_buffer_length);
    // Starting lengths
    size_t string_length = 0, array_length = 0;
    size_t cursor = 0; // cursor; where user is currently typing/editing
    enable_raw_mode(); // turn off canonical mode, take user input char by char
    /****************************************************************************
    MyShell> accepts user input 
    Gets the input typed by the user in the MyShell> prompt in the form of a line
    ****************************************************************************/
    while (read(STDIN_FILENO, &ch, 1) == 1) { // read standard input
        /*********************************************************************
        Extra credit: If your program accepts user input with unlimited length
        *********************************************************************/
        // buffer check, check if string length is close to buffer size
        if (string_length + 1 >= string_buffer_length) {
            string = realloc_buffer(string, &string_buffer_length);
        }
        /*****************************************************************************
        When the user types an empty command, no error message is printed and the next 
        MyShell> prompt is displayed
        *****************************************************************************/
        if (ch == NEWLINE && !string[0]) { // reprint shell for empty input
            fprintf(stdout, "\n%s", SHELL_NAME);
        } else if (ch == NEWLINE) { // finalize command line
            string[string_length] = NULLCHAR; // null terminate string
            fprintf(stdout, "\n");  // Move to next line
            break;
        } else if (ch == '\t') { // Do nothing for tab; future autocomplete feature
            continue;
        }
        // '\033' represents the ASCII escape character (27 in decimal, 0x1B in hex)
        else if (ch == '\033') { // terminal sends 3 bytes in sequence
            char seq[3]; // Ideally: seq[0] = '[', seq[1] = Letter code
            // capture next chars, if error then break
            if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) break;

            // ANSI escape sequences, '[' is the Control Sequence Introducer (CSI)
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': // Up arrow
                        break;
                    case 'B': // Down arrow
                        break;
                    case 'C': // Right arrow
                        // 1 is the number of units to move
                        // C is the command code for "Cursor Forward"
                        if (cursor < string_length) {
                            fprintf(stdout, "\033[1C");  // Move cursor right
                            cursor++;
                        }
                        break;
                    case 'D': // Left arrow
                        // 1 is the number of units to move
                        // D is the command code for "Cursor Backward"
                        if (cursor > 0) {
                            fprintf(stdout, "\033[1D");  // Move cursor left
                            cursor--;
                        }
                        break;
                }
            }
        } else if ((ch == 127 || ch == '\b')) { // handle back spacing
            if (cursor <= 0) { // boundary check
                continue; // do nothing
            }
            // shift characters left
            // Step by step visualization of backspacing with memmove:
            // Initial:    "Hello World"    (delete 'W')
            //              01234567890      string_length = 11, cursor = 7
            //                     ^
            // 1. Source:        "World"    (substring starting from cursor position)
            // 2. Dest:          "orld"     (shifted one position left, starting from cursor-1 position)
            // 3. memmove: Moves the substring to the left, overwriting the characters at cursor-1
            // 4. Result:  "Hello orld"
            //              0123456789       string_length = 10, cursor = 6, bytes to copy = 5
            //                    ^
            memmove(&string[cursor-1], &string[cursor], string_length - cursor + 1); // + 1 to include \0

            // decrement string length and cursor position
            string_length--;
            cursor--;

            // Update display
            fprintf(stdout, "\b"); // Move back
            // Prints the remaining string after cursor
            fprintf(stdout, "%.*s", (int)(string_length - cursor), &string[cursor]);
            fprintf(stdout, " "); // Clear character
            
            // Reset cursor position by moving cursor back
            fprintf(stdout, "\033[%zuD", string_length - cursor + 1);
        } else {
            if (cursor < string_length) { // handle substring insertions
                // Shift characters right
                // Step by step visualization of inserting with memmove:
                // Initial:    "Hello World"   (inserting an 'x')
                //              01234567890     string_length = 11, cursor = 6
                //                    ^
                // 1. Source:        "World"   (substring starting from cursor position)
                // 2. Dest:          "xWorld"  (shifted one position right, starting from cursor+1 position)
                // 3. memmove:  Moves the substring to the right, overwriting the characters at cursor+1
                // 4. Result:  "Hello xWorld"
                //              012345678901    string_length = 12, cursor = 7, bytes to copy = 6
                //                     ^

                memmove(&string[cursor + 1], &string[cursor], string_length - cursor + 1); // + 1 to include \0
                
                // Insert new character
                string[cursor] = ch;
                // Increment string length and cursor position
                string_length++;
                cursor++;
                
                // Update display
                fprintf(stdout, "%c", ch);              // Print new char
                fprintf(stdout, "\033[K");              // Clear line after cursor
                fprintf(stdout, "%s", &string[cursor]); // Print rest of string after cursor

                // Reset cursor position by moving cursor back
                fprintf(stdout, "\033[%zuD", string_length - cursor);

            } else { // end of line insertions
                fprintf(stdout, "%c", ch); // Print new char

                // Insert new character
                string[cursor] = ch;
                // Increment string length and cursor position
                cursor++;
                string_length++;
            }
        }
        fflush(stdout); // flushes character out, essentially prints what's queued up.
    }

    disable_raw_mode(); // return to normal terminal setting state

    /********************************************************************
    - Splits the line into tokens
    - The tokens are used to create a char** pointer which will serve 
    as an argument for execvp in execute() function
    - The user input is parsed and tokenized to suit the execvp arguments
    ********************************************************************/

    // remove preceding whitespace and reallocate unused memory
    string = realloc_leftover_string(string, &string_length);

    int extra_whitespace = 0; // keep track of extra whitespace
    char *word_start = string;  // Track start of current word
    for (int i = 0; i < string_length; i++) { // go through the entire buffer
        /**********************************************************************************
        Extra credit: If your program accepts user input with unlimited number of arguments
        **********************************************************************************/
        // buffer check, check if array length is close to buffer size
        if (array_length + 1 >= command_line_buffer_length) {
            args = realloc_buffer(args, &command_line_buffer_length);
        }

        if (i != 0 && (string[i] == '"' || string[i] == '\'')) { // Check for quotes to include whitespaces
            char quote = string[i];                             // Note which delimiter we track
            i++;
            word_start = &string[i];                            // Ignore beginning quote
            while (i < string_length && string[i] != quote) i++;// Keep adding until closing quote
            string[i] = NULLCHAR;                               // Null terminate word excluding end quote
            args[array_length] = word_start;                    // Add to args
            array_length++;
            word_start = &string[i + 1];                        // Start of next word

        } else if (string[i] == ' ' && string[i + 1] != ' ') {  // End of word
            string[i - extra_whitespace] = NULLCHAR;            // Null terminate word accounting for multiple whitespace
            args[array_length] = word_start;                    // Add token to args
            array_length++;
            word_start = &string[i + 1];                        // Start of next word
            extra_whitespace = 0;                               // Reset whitespace count

        } else if (string[i] == ' ' && string[i + 1] == ' ') {  // Extra whitespace check
            extra_whitespace++;
        }
    }

    // Add final word if exists
    if (word_start[0] != NULLCHAR) {
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
    // Free the first string (original Command Line buffer)
    if (args[0] != NULL) free(args[0]);
    // Free the cmd array
    if (args != NULL) free(args);
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
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

/**
 * Reallocates the string with with error checking.
 * 
 * @param string The string buffer to resize
 * @param string_length The length of the current string
 * @note Exits with status 1 if memory reallocation fails
 */
void* realloc_leftover_string(char *string, size_t *string_length) {
    size_t i = 0;
    while (string[i] == ' ') { // count preceding whitespaces
        i++;
    }
    // shift left by the amount of whitespaces, removing them.
    if (i != 0) {
        memmove(string, string + i, *string_length - i + 1); // + 1 to account for null char
        *string_length -= i;
    }

    // realloc and reduce size to length of string
    char *resized_string = realloc(string, sizeof(string) * (*string_length + 1)); // manage unused memory
    if (resized_string == NULL) {
        fprintf(stderr, "Memory allocation failed for size %zu\n", *string_length + 1);
        free(string); // Free original buffer if realloc fails
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
    return ptr;
}

/**
 * @brief Disables raw mode and restores the terminal to its original settings
 * This function is called when exiting the program to restore normal terminal behavior
 */
void disable_raw_mode() {
    // Attempt to restore original terminal settings
    // TCSAFLUSH will wait for all output to be transmitted
    // and discards any unread input before applying the changes
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_tio) == -1) {
        perror("tcsetattr: Failed to restore terminal settings");
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
        perror("tcgetattr: Failed to save terminal settings");
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
        perror("tcsetattr: Failed to apply new terminal settings");
        exit(EXIT_FAILURE);
    }
}

/**
 * Signal handler for SIGINT (Ctrl+C).
 * Prints ^C to stdout and terminates the program with exit code 1.
 *
 * @param sig The signal number (SIGINT)
 */
void handle_sigint(int sig) {
    printf("^C\n");
    exit(EXIT_FAILURE);
}