#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <glob.h>
#include <fnmatch.h>

ssize_t n_bytes = 0;
size_t command_len = 0;
char *command = NULL;
#define MAX_LINE_LENGTH 1024
#define EXIT_SUCCESS 0

int check_exit_status() {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG); // Non-blocking wait
    if (pid == -1) {
        if (errno == ECHILD) {
            return -2; // Indicate no child processes exist
        } else {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status); // Child exited normally, return its exit status
        } else {
            printf("Child process terminated abnormally\n");
            return -1; // Indicate abnormal termination
        }
    } else {
        // pid == 0, meaning no child has exited yet
        printf("No child process has exited yet.\n");
        return -3; // Indicate that no child process has exited yet
    }
}

int thenelse(char* command[]) {
    if (command[0] != NULL) {
        int exitstat = check_exit_status();

        // Handle "then" and "else" based on the exit status of child process
        if (strcmp(command[0], "then") == 0) {
            if (exitstat == 0) {
                return 1; // Command succeeded; execute the command following "then"
            } else {
                perror("Previous command failed.");
                return 0; // Command failed; do not execute the command following "then"
            }
        } else if (strcmp(command[0], "else") == 0) {
            if (exitstat != 0) {
                return 1; // Previous command failed; execute the command following "else"
            } else {
                perror("Previous command succeeded.");
                return 0; // Command succeeded; do not execute the command following "else"
            }
        } else {
            return -1; // No "then" or "else" found, or improper usage.
        }
    } else {
        perror("Command is empty.");
        return -1; // Indicates an empty command or improper usage.
    }
}

void my_cd(char *path) {
    if (chdir(path) != 0) {
        perror("cd");
    }
}

void my_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
}

void my_which(char *command) {e
    if (command == NULL) {
        fprintf(stderr, "which: too few arguments\n");
        return;
    }

    char resolved_path[PATH_MAX]; // Buffer to store the resolved path

    // Check if the command is an executable file in the current directory
    if (realpath(command, resolved_path) != NULL) {
        printf("%s\n", resolved_path);
        return;
    }

    char *path_env = getenv("PATH");
    if (path_env == NULL) {
        fprintf(stderr, "which: Unable to retrieve the PATH environment variable.\n");
        return;
    }

    char *path = strtok(path_env, ":");
    
    while (path != NULL) {
        char program_path[PATH_MAX];
        snprintf(program_path, sizeof(program_path), "%s/%s", path, command);

        // Check if the program exists at the current path
        if (realpath(program_path, resolved_path) != NULL) {
            printf("%s\n", resolved_path);
            return;
        }

        path = strtok(NULL, ":");
    }

    fprintf(stderr, "which: %s: command not found\n", command);
}

void my_exit(char *command[]) {
    // Print any arguments received, separated by spaces
    for (int i = 1; command[i] != NULL; i++) {
        printf("%s ", command[i]);
    }
    printf("\nExiting my shell...\n");
    exit(EXIT_SUCCESS);
}

void handle_wildcard(char *token, char *args[], int *num_args) {
    glob_t glob_result;

/*    // Perform wildcard expansion
    if (glob(token, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            args[(*num_args)++] = strdup(glob_result.gl_pathv[i]); // Add expanded arguments
        }
        globfree(&glob_result);
    } else {
        perror("Wildcard expansion failed");
        exit(EXIT_FAILURE);
    }
}*/
if (args == NULL || num_args == NULL) {
        fprintf(stderr, "handle_wildcard: Invalid arguments\n");
        exit(EXIT_FAILURE);
    }

    glob_t glob_result;

    // Perform wildcard expansion
    if (glob(token, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            // Add expanded arguments
            args[(*num_args)++] = strdup(glob_result.gl_pathv[i]); 
            if (args[*num_args - 1] == NULL) {
                perror("handle_wildcard: Memory allocation failed");
                exit(EXIT_FAILURE);
            }
        }
        globfree(&glob_result);
    } else {
        perror("Wildcard expansion failed");
        exit(EXIT_FAILURE);
    }
}

void traverse_dict(char *command[], int tokct) {
    const char *directories[] = {"/usr/local/bin", "/usr/bin", "/bin"};
    int num_directories = 3;
    int command_executed = 0;

    for (int i = 0; i < num_directories && !command_executed; i++) {
        const char *directory = directories[i];
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, command[0]);

        if (access(full_path, X_OK) == 0) {
            // Check if the command contains wildcrd characters
            int contains_wildcard = 0;
            for (int j = 0; j < tokct; j++) {
                if (strchr(command[j], '*') != NULL) {
                    contains_wildcard = 1;
                    break;
                }
            }

            if (contains_wildcard) {
                glob_t glob_result;
                int glob_flags = GLOB_TILDE | GLOB_NOCHECK;
                int ret = glob(command[1], glob_flags, NULL, &glob_result);
                if (ret == 0) {
                    // Execute the command with expanded argument
                    for (size_t i = 0; i < glob_result.gl_pathc; i++) {
                        command[1] = glob_result.gl_pathv[i];
                        pid_t pid = fork();
                        if (pid == -1) {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        } else if (pid == 0) { 
                            if (execv(full_path, command) == -1) {
                                perror("execv");
                                exit(EXIT_FAILURE);
                            }
                        } else { 
                            int status;
                            waitpid(pid, &status, 0);
                        }
                    }
                    globfree(&glob_result);
                    command_executed = 1;
                } else if (ret == GLOB_NOMATCH) {
                    printf("No matching files found.\n");
                } else {
                    perror("glob");
                }
            } else {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) { // Child process
                    if (execv(full_path, command) == -1) {
                        perror("execv");
                        exit(EXIT_FAILURE);
                    }
                } else { // Parent process
                    int status;
                    waitpid(pid, &status, 0);
                    command_executed = 1;
                }
            }
        }
    }

    if (!command_executed && strcmp(command[0], "cd") != 0) {
        printf("Command not found in specified directories.\n");
    }
}

void run_command(char *command[], int tokct){
    if (command[0] != NULL){
        if (strcmp(command[0], "cd") == 0) {
            if (tokct < 2) {
                fprintf(stderr, "cd: missing operand\n");
            } else {
                my_cd(command[1]);
            }
        } else if (strcmp(command[0], "pwd") == 0) {
            my_pwd();
            return;
        }
        if (strcmp(command[0], "which") == 0) {
            my_which(command[1]);
            return;
        }
        if (strcmp(command[0], "exit") == 0) {
            my_exit(command);
        }
        else{
            traverse_dict(command, tokct);
        }
    }
}  

void redirection(char *command[], int tokct) {
    char *input_file = NULL;
    char *output_file = NULL;
    
    // Iterate through the command tokens to find redirection tokens
    for (int i = 0; i < tokct; i++) {
        if (strcmp(command[i], "<") == 0) {
            // Redirect input
            if (i + 1 < tokct) {
                input_file = command[i + 1];
                command[i] = NULL; // Remove the "<" token
            } else {
                printf("Error: Missing input file after '<'\n");
                exit(1);
            }
        } else if (strcmp(command[i], ">") == 0) {
            // Redirect output
            if (i + 1 < tokct) {
                output_file = command[i + 1];
                command[i] = NULL; // Remove the ">" token
            } else {
                printf("Error: Missing output file after '>'\n");
                exit(1);
            }
        }
    }

    if (input_file != NULL) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in == -1) {
            perror("Error opening input file");
            exit(1);
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {
            perror("Error redirecting input");
            exit(1);
        }
        close(fd_in);
    }
    
    // Perform output redirection if necessary
    if (output_file != NULL) {
        int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd_out == -1) {
            perror("Error opening output file");
            exit(1);
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            perror("Error redirecting output");
            exit(1);
        }
        close(fd_out);
    }
    run_command(command, tokct);
}

void handle_pipe(char *tokens[], int token_count){
    int pid = fork();
    run_command(tokens, token_count);
}

void new_shell() {
    const char *prompt = "mysh";
    while (1) {
        printf("%s> ", prompt);
        fflush(stdout);
        char input[1024];
        ssize_t bytes_read = read(STDIN_FILENO, input, sizeof(input));
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (bytes_read == 0) {
            printf("Exiting my shell.\n");
            break;
        }
        input[bytes_read] = '\0'; // Null-terminate the input
        
        char *token;
        char *tokens[1024]; // Assuming max 1024 tokens per command
        int token_count = 0;
        token = strtok(input, " \n"); // Tokenize based on space and newline
        while (token != NULL) {
            tokens[token_count++] = token;
            token = strtok(NULL, " \n");
        }
        tokens[token_count] = NULL; // Null-terminate the token array

        //checks for then/else, if contains, removes the then/else from the command and CONTINUES
        for (int i = 0; i < token_count; i++) {
            if(thenelse(tokens) == 1){
                for (int j = i; j < token_count - 1; j++) {
                    tokens[j] = tokens[j + 1];
                }
                token_count--;
                break;
            }       
            else if(thenelse(tokens) == 0){
                perror("Then/Else failed.");
                exit(EXIT_SUCCESS);
            }
        }
        
        for (int i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "|") == 0) {
                handle_pipe(tokens, token_count);
            }
            if ((strcmp(tokens[i], "<") == 0 )|| (strcmp(tokens[i], ">") == 0 )) {
                redirection(tokens, token_count);
            }
        }
        run_command(tokens, token_count);
    }
}

char **splitLine(char *line) {
    char **tokens = (char **)malloc(sizeof(char *) * 64);
    char *token;
    char delim[10] = " \t\n\r\a";
    int pos = 0, bufsize = 64;
    if (!tokens) {
        printf("\nBuffer Allocation Error.");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, delim);
    while (token != NULL) {
        tokens[pos] = strdup(token); // Use strdup to allocate memory for each token
        pos++;
        if (pos >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                printf("\nBuffer Allocation Error.");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, delim);
    }
    tokens[pos] = NULL;
    return tokens;
}

int batchMode(char filename[128]) {
    char line[MAX_LINE_LENGTH];
    char **args;
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("\nUnable to open file.");
        return 1;
    } else {
        ssize_t bytes_read;
        // Read lines from the file and process them one by one
        while ((bytes_read = read(fd, line, MAX_LINE_LENGTH)) > 0) {
            line[bytes_read - 1] = '\0'; // Replace newline with null terminator
            args = splitLine(line);
            if (args == NULL) {
                perror("Splitting line failed.");
                exit(EXIT_FAILURE);
            }
            int count = 0;
            while (args[count] != NULL) {
                count++;
            }
            // Process the command tokens obtained from the line
            for (int i = 0; i < count; i++) {
                if (thenelse(args) == 1) {
                    for (int j = i; j < count - 1; j++) {
                        args[j] = args[j + 1];
                    }
                    count--;
                    break;
                } else if (thenelse(args) == 0) {
                    perror("Then/Else failed.");
                    exit(EXIT_FAILURE);
                }
            }
            for (int i = 0; i < count; i++) {
                if (strcmp(args[i], "|") == 0) {
                    handle_pipe(args, count);
                }
                // Add other conditions as needed for redirection, etc.
            }
            run_command(args, count);
            free(args);
        }
    }
    close(fd);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Check if standard input is coming from a terminal
        if (isatty(STDIN_FILENO)) {
            printf("Welcome to my shell!\n");
            new_shell(); // Run interactive shell
        } else {
            fprintf(stderr, "Error: Cannot run interactive shell in non-terminal mode.\n");
            return EXIT_FAILURE;
        }
    }
    else if (argc == 2) {
        int batch = batchMode(argv[1]); // Run shell in batch mode
    }
    else {
        printf("\nInvalid Number of Arguments\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
