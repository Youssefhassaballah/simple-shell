#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


char** variables;
char** value_of_variables;
int SIZE_INDEX = 0;
#define MAX_ARGS 20



char **read_input() {
    char **x = malloc(MAX_ARGS * sizeof(char *));
    if (x == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    int index = 0;
    char buffer[200]; 
    printf("Enter commands (type 'exit' to stop): ");
    
    while (index < MAX_ARGS - 1) {
        if (scanf("%199s", buffer) != 1) {
            break;
        }
        
        if (buffer[0] == '$') { 
            char *var_name = buffer + 1;
            int found = 0;
            for (int j = 0; j < SIZE_INDEX; j++) {
                if (strcmp(var_name, variables[j]) == 0) {
                    char *value = strdup(value_of_variables[j]);
                    char *sub_token = strtok(value, " ");
                    while (sub_token && index < MAX_ARGS - 1) {
                        x[index++] = strdup(sub_token);
                        sub_token = strtok(NULL, " ");
                    }
                    free(value);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                x[index++] = strdup("");
            }
        } else {
            x[index++] = strdup(buffer);
        }
        char ch = getchar();
        if (ch == '\n' || ch == EOF) {
            break;
        }
    }
    x[index] = NULL;
    return x;
}



void execute_shell_bultin(char** command){
    if (strcmp(command[0], "cd") == 0) {
        if (command[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else if (chdir(command[1]) != 0) {
            perror("cd failed");
        }
    } 
    else if (strcmp(command[0], "echo") == 0) {
        if(command[1][0] == '$'){
            char destination[100];
            strncpy(destination, command[1]+1,99);
            destination[99] = '\0'; 
            for (int i = 0; i <= SIZE_INDEX; i++) {
                if (strcmp(destination, variables[i]) == 0) {
                    printf("%s\n", value_of_variables[i]);
                    break;
                }
            }
        }else{
            for (int i = 1; command[i] != NULL; i++) {
                printf("%s ", command[i]);
            }
            printf("\n");
        }
    } 
    else if (strcmp(command[0], "export") == 0) {
        if (command[1] == NULL) {
            fprintf(stderr, "export: missing argument\n");
        } else {
            char buffer[100];
            strncpy(buffer, command[1], 99);
            buffer[99] = '\0';
            char *variable = strtok(buffer, "=");
            char *value = strtok(NULL, "=");
            if (variable && value) {
                if (value[0] == '"') {
                    value++; 
                    char full_value[100] = {0};
                    strcat(full_value, value);
                    strcat(full_value, " ");

                    if (command[1][strlen(command[1]) - 1] == '"') { 
                        full_value[strlen(full_value) - 2] = '\0'; 
                    }

                    int i = 2;
                    while (command[i] != NULL) {
                        strcat(full_value, command[i]);
                        strcat(full_value, " ");
                        if (command[i][strlen(command[i]) - 1] == '"') { 
                            full_value[strlen(full_value) - 2] = '\0'; 
                            break;
                        }
                        i++;
                    }
                    value = strdup(full_value);
                }
                for (int i = 0; i < SIZE_INDEX; i++) {
                    if (strcmp(variables[i], variable) == 0) {
                        free(value_of_variables[i]);
                        value_of_variables[i] = strdup(value);
                        return;
                    }
                }
                variables[SIZE_INDEX] = strdup(variable);
                value_of_variables[SIZE_INDEX] = strdup(value);
                SIZE_INDEX++;
            } else {
                fprintf(stderr, "export: invalid format. Use variable=\"value with spaces\"\n");
            }
        }
    }
}



void execute_command(char **command, int is_foreground) {
    pid_t child_id = fork(); 
    if (child_id < 0) {
        perror("Fork failed"); 
        exit(1);
    } 
    else if (child_id == 0) {
        execvp(command[0], command);
        perror("execvp failed");
        exit(1);
    } 
    else {
        if (is_foreground) {
            waitpid(child_id, NULL, 0);
        }
    }
}



void shell() {
    while (1) {
        char **command = read_input();
        if (command == NULL || command[0] == NULL) {
            continue;
        }
        if (strcmp(command[0], "exit") == 0) {
            printf("Thanks for using my shell :)\n");
            exit(0);
        }

        int background = 0;
        for (int i = 0; command[i] != NULL; i++) {
            if (strcmp(command[i], "&") == 0) {
                background = 1;
                command[i] = NULL;
                break;
            }
        }

        if (strcmp(command[0], "cd") == 0 || strcmp(command[0], "echo") == 0 || strcmp(command[0], "export") == 0) {
            execute_shell_bultin(command);
        } else {
            execute_command(command, !background);
        }

        for (int i = 0; command[i] != NULL; i++) {
            free(command[i]);
        }
        free(command);
    }
}



void setup_environment() {
    // mkdir("/");

    const char* path = "/mnt/e/second year/2nd term/os/labs/lab1";
    if (chdir(path) != 0) {
        perror("Error changing directory");
    } 
}



void on_child_exit(int signum) {
    (void)signum;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}



void register_child_signal() {
    struct sigaction sa;
    sa.sa_handler = on_child_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }
}



int main() {
    variables = calloc(10, sizeof(char *));
    value_of_variables = calloc(10, sizeof(char *));
    if (variables == NULL || value_of_variables == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    register_child_signal();
    setup_environment();
    shell();
    return 0;
}
