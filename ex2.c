#define _XOPEN_SOURCE 500 // Handles usleep function warning.

/*
 * This C code simulates a Shell terminal, and supports all
 * Shell commands with their respective flags, such as: ls,
 * cat,sleep and more.
 * Also includes built-in commands: jobs,cd and exit.
 */

#include <stdio.h>  // Various included libraries used.
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_LENGTH 512  // Definitions of repeated integers and strings.
#define ERROR_IN_CALL "Error in system call\n"
#define ERROR_IN_CD "Error in cd command\n"

// ***************************************************************** Command Declarations *****************************
// Function implementation is located after the main() function.
void CDCommand(char* dir);
void ExitCommand(char** job_array);
void JobsCommand(char** job_array, int* job_array_pid);
void UpdateJobArray(char** job_array, int* job_array_pid, char* jobDescription, int jobPID);
//**************************************************************** Main - Start of Shell ******************************
char curr[100];
char prev[100];
int main() {
    char buffer[BUFFER_LENGTH];  // The variables used to store the buffer, command arguments and process status.
    char* commands[BUFFER_LENGTH];
    char* job_array[BUFFER_LENGTH];  // Create the job_array and job_pid array to store all processes and their PID's.
    int job_array_pid[BUFFER_LENGTH];
    int status; // Used to get status of child ID after waiting.
    int i;  // General index, will be used throughout the code.
    for (i = 0; i < BUFFER_LENGTH; ++i) {  // Initialize the job_array and the job_array_pid.
        job_array[i] = NULL;
        job_array_pid[i] = -1;
    }
    int timer = 1;  // Integer used to tell if a process is waiting or not.
    while (1) {  // Start of shell, will run until closed by exit command.
        usleep(10000); // Sleep at the start to avoid prompt '>' being printed out of order.
        printf("> "); // Print the prompt.
        fgets(buffer, BUFFER_LENGTH, stdin);  // Get the command from the user and store it in the buffer string.
        strtok(buffer, "\n");  // Remove the newline from the buffer string.
        int m; // Handle the x& case when the & is right after the command without space.
        for (m = 1; m < BUFFER_LENGTH; m ++) {
            // Try to move the & one slot forward if possible and if its the last char in the buffer.
            if ((buffer[m] == '&') && (buffer[m - 1] != ' ' ) && (m != BUFFER_LENGTH - 1) && (buffer[m+1] == '\0')) {
                buffer[m] = ' ';   // Move it up and replace the character in it's slot with a ' ' space.
                buffer[m + 1] = '&';
                break;  // and finish.
                // If it can't be moved forward we'll just think of it as an error since it doesn't fit inside the
                // buffer length we assigned.
            }
        }
        char job[BUFFER_LENGTH];  // Place the buffer in the job string to store in the job array.
        strcpy(job, buffer);
        i = 0;
        commands[i] = strtok(buffer, " ");  // Split the buffer by the ' ' space chars.
        while (commands[i] != NULL) {  // Fill command arguments one by one.
            commands[++i] = strtok(NULL, " ");
        }
        commands[i] = NULL; // The last argument MUST be null in order to pass it into the execvp command.
        if (commands[0] == NULL || strcmp(commands[0], "\n") == 0) {  // If there is nothing in the command restart.
            continue;  // Go back the prompt.
        }
        if (strcmp(commands[0], "&") == 0) {  // If '&' is at [0] it's an error, stderr error printed and restart shell.
            fprintf(stderr, ERROR_IN_CALL);
            continue;  // Go back the prompt.
        }
        timer = 1; // timer will be changed if there is a '&' character.
        if ((strcmp(commands[i - 1], "&")) == 0) {  // If there is a '&' we do not wait for the child process.
            timer = 0;  // Set the waiting integer to 0 to notify the shell of this condition.
            commands[i - 1] = NULL;  // Remove the '&' from the command arguments as we are done with it.
            int commandsLength = (int) strlen(job);  // Remove the useless ' ' and '&' in the job string.
            if (commandsLength >= 2) {
                job[commandsLength - 1] = '\0';  // Remove the characters.
                job[commandsLength - 2] = '\0';
            }
        }
        // ****************************************************** Built-in commands ***********************************
        // Implemented in separate function to make code more readable.
        // All these commands are done on the father process without fork().
        // So they will all have the PID of the father process.
        if (strcmp(commands[0], "exit") == 0) {  // Exit command.
            printf("%d\n", (int) getpid());  // Print the ID of the process.
            ExitCommand(job_array);  // Call the function.
        }
        if (strcmp(commands[0], "cd") == 0) {  // CD command.
            printf("%d\n", (int) getpid());
            char dir[100]; // In case of spaces in the directory name.
            memset(dir, 0 ,100);  // Reset the char buffer.
            char temp1[100]; // In case of spaces in the directory name.
            memset(temp1, 0 ,100);  // Reset the char buffer.
            char temp2[100]; // In case of spaces in the directory name.
            memset(temp2, 0 ,100);  // Reset the char buffer.
            int k = 1;
            if (commands[1] != NULL && commands[2] == NULL) {  // Handle a signle word with '\"' characters
                if (commands[1][0] == '\"' && commands[1][strlen(commands[1]) - 1] == '\"') {
                    strcat(temp1, commands[1] + 1);  // Copy without the first char.
                    strncat(temp2, temp1, strlen(temp1) - 1);  // Copy without the last character.
                    memset(commands[1], 0, strlen(commands[1]));  // Reset the char buffer.
                    strcat(commands[1], temp2);  // Replace the commnad without quotes.
                }

            }
            for(k = 1; k < 99; k++) {  // Create a path from the command arguments.
                if(commands[k] == NULL) {  // If the end is reached stop.
                    break;
                }
                // Remove the first character.
                if(commands[k][0] == '\"' ) {  // remove the first " \" " character.
                    strcat(dir, commands[k] + 1);  // Copy without the first char.
                    strcat(dir, " ");  // Add a space.
                    continue;
                }
                if(commands[k][strlen(commands[k]) - 1] == '\"') { // Remove the last " \" " character.
                    strncat(dir, commands[k], strlen(commands[k]) - 1);  // Copy without the last character.
                    strcat(dir, " ");  // Add a space.
                    continue;
                }

                strcat(dir, commands[k]);  // If no " \" " character just copy the argument normally.
                strcat(dir, " ");  // Add a space.
            }
            int l = 0;  // Remove the final space and replace with '\0'.
            for(l = 0; l < 99; l++) {
                if(dir[l] == ' ' && dir[l+1] == '\0') {  // If it's the last character and " " space, remove it.
                    dir[l] = '\0';
                    break;
                }
            }
            CDCommand(dir);  // Call the CD command function.
            continue;  // Go to the end iteration of the main while loop.
        }
        if (strcmp(commands[0], "jobs") == 0) {  // JOBS command.
            JobsCommand(job_array, job_array_pid);  // Does not require PID to be printed.
            continue;
        }
        // ****************************************************** Command execution ***********************************
        pid_t pid = fork(); // Fork the current process.
        if (pid == -1) { // Handle fork() failure.
            perror("fork() failed."); // Print error message and exit.
            exit(-1);
        }
        else if (pid == 0) {  // Upon success of fork() let the child process handle the command.
            printf("%d\n", (int) getpid());  // Print the PID of the child process.
            if (strcmp(commands[0], "man") == 0) {  // The Man command handles separately, since not in "bin" path.
                if (execvp(commands[0], commands) == -1) {  // Execute the command and handle if error.
                    fprintf(stderr, ERROR_IN_CALL);
                };
                exit(EXIT_FAILURE);  // Since execvp leaves the program, we kill the child here if an error occurred.
            }
            char bin[BUFFER_LENGTH] = "/bin/";  // Other commands are taken through bin.
            commands[0] = strcat(bin, commands[0]);  // That's why Man needs it's own implementation.
            if (execvp(commands[0], commands) == -1) {  // Execute the command and handle if error.
                fprintf(stderr, ERROR_IN_CALL);  // Print error from stderr.
            }
            exit(EXIT_FAILURE);  // Since execvp leaves the program, we kill the child here if an error occurred.
        } else {
            // **************************************************** Waiting *******************************************
            if (timer) {  // Wait for child, if the timer integer was set to 1 earlier.
                pid_t wpid = waitpid(pid, &status, WUNTRACED);  // Return if no child has exited or child has ended.
            } else {
                UpdateJobArray(job_array, job_array_pid, job, (int) pid);  // If we don't wait, add it to the job array.
            }
        }
    }
}
// ***************************************************************** Command Implementations **************************
/*
 * Function implements the CD command.
 * @param dir
 *  The directory path we want to go to.
 */
void CDCommand(char* dir) {
    getcwd(curr, 100);  // Get current directory.
    int updated = 1;  // Will tell us if directory has been updated.
    if ((dir != NULL && strcmp(dir, "~") == 0) || (dir == NULL) || (dir[0] == '\0')) {
        if (strcmp(dir,"~") == 0) {
            if (chdir(dir) == -1) { // Try going into a folder called "~".
                if (chdir(getenv("HOME")) == -1) {  // Error handling is done if chdir fails.
                    updated = 0;
                    fprintf(stderr, ERROR_IN_CD);   // Printing the error from stderr.
                }
            }
        }
            // If path is ~ or empty move to HOME directory.
        else if (chdir(getenv("HOME")) == -1) {  // Error handling is done if chdir fails.
            updated = 0;
            fprintf(stderr, ERROR_IN_CD);   // Printing the error from stderr.
        }
    }
        // Handle the '..' flag, go back up to parent directory.
    else if (strcmp(dir, "..") == 0) {
        if (chdir("..") == -1) {  // Error handling is done if chdir fails.
            updated = 0;
            fprintf(stderr, ERROR_IN_CD);   // Printing the error from stderr.
        }
    }
    else if (strcmp(dir, "-") == 0) {  // If path is '-' we go back the previous directory.
        if(chdir(dir) == -1) { // Try going into a folder called "-".
            if (chdir(prev) == -1) {  // Handle chdir error.
                updated = 0;
                printf("OLDPWD not set.\n");  // Print old pwd error if fails.
                fprintf(stderr, ERROR_IN_CD);   // Printing the error from stderr.
            };
        }
    }
    else if (chdir(dir) == -1) {  // If the path is not ~ or empty, we can move into it normally using chdir.
        updated = 0;
        fprintf(stderr, ERROR_IN_CD);  // Print stderr error if fails.
    };
    if (updated == 1) {  // if we have updated our path, we set the prev to the old current.
        int k;
        for (k = 0; k < 100; k++) {
            prev[k] = curr[k];
        }
    }
}
/*
 * Function implements the EXIT command.
 * @param job_array
 *  The array of jobs we use to track the current running processes.
 */
void ExitCommand(char** job_array) {
    int i;
    for (i = 0; i < BUFFER_LENGTH; ++i) { // Free allocated memory in job array.
        if (job_array[i] != NULL) {
            free(job_array[i]);
        }
    }
    exit(EXIT_SUCCESS);  // Exit.
}
/*
 * Function implements the built-in JOBS command.
 * @param job_array
 *  The array of jobs we use to track the current running processes.
 * @param job_array_pid
 *  The array of job process ID's we use to identify the current running processes.
 */
void JobsCommand(char** job_array, int* job_array_pid) {
    int status;  // Remove any processes that are not running anymore from the job array.
    pid_t wpid = -1;
    int i;
    // Check if a process has been terminated to decide if to delete it from the job arrays.
    for (i = 0; i < BUFFER_LENGTH; ++i) {  // Check each process in the job array.
        if (job_array[i] != NULL) {
            wpid = waitpid(job_array_pid[i], &status, WNOHANG);  // Return immediately if no child has exited.
            if (wpid == -1) {  // Handle waitpid() failure.
                perror("waitpid() failed.");  // Print error message and exit.
                exit(EXIT_FAILURE);
            }
        }
        // Deleting the process from both the job array and job PID array.
        if ((int) wpid == job_array_pid[i]) {  // If process terminated and waitpid returns PID matching the process.
            free(job_array[i]);  // Free the slot from the job array.
            job_array[i] = NULL;  // Reset it to NULL.
            job_array_pid[i] = -1;  // Reset the slot of the PID array at the same index.
        }
    }
    for (i = 0; i < BUFFER_LENGTH; ++i) {  // Print and display the current running processes from the job array.
        if (job_array[i] != NULL) {
            printf("%d %s\n", job_array_pid[i], job_array[i]);
        }
    }
}
/*
 * Function updates the job arrays after each process is created.
 * @param job_array
 *  The array of jobs we use to track the current running processes.
 * @param job_array_pid
 *  The array of job process ID's we use to identify the current running processes.
 * @param jobDescription
 *  The text that will go into the job_array.
 * @param jobPID
 *  The ID of the job in the array, will go into job_array_pid.
 */
void UpdateJobArray(char** job_array, int* job_array_pid, char* jobDescription, int jobPID) {
    int i = -1;  // Find the first available slot in the job array.
    int j;
    for (j = 0; j < BUFFER_LENGTH; ++j) {  // Iterate the job array until we reach a NULL slot.
        if (job_array[j] == NULL) {  // If the slot is NULL, we save the index and stop the loop.
            i = j;
            break;
        }
    }
    if (i == -1) {  // The array is of given size 512, if it's full, we can't add the job and print out and error.
        perror("Could not add job to array.");
        return;
    }
    job_array_pid[i] = jobPID; // Set the PID at the correct slot in the PID array.
    job_array[i] = (char*) malloc(strlen(jobDescription) + 1);   // Allocate memory for the job description string.
    if (job_array[i] == NULL) {   // Check if allocation has failed and handle the error if needed.
        perror("Could not allocate memory.");
        return;

    }
    strcpy(job_array[i], jobDescription);  // Add the job description to the array by copying the string to the slot.
}
