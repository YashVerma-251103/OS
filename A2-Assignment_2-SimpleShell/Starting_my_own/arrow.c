#include <stdio.h>
#include <termius.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 1024
#define HISTORY_SIZE 10

struct termios old_settings;
char command_history[HISTORY_SIZE][MAX_COMMAND_LENGTH];
int history_index = -1;  // Points to the most recent command in history
int current_history = -1;  // Used for navigating history
int history_count = 0;  // Number of commands stored in history

// Function to restore terminal settings
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
}

// Function to set non-canonical mode
void set_non_canonical_mode() {
    struct termios new_settings;

    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &old_settings);
    new_settings = old_settings;

    // Disable canonical mode and echo
    new_settings.c_lflag &= ~(ICANON | ECHO);

    // Apply the new settings
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
}

// Function to handle arrow key functionality
void handle_input() {
    char buffer[MAX_COMMAND_LENGTH];
    int length = 0;
    int cursor_position = 0;

    while (1) {
        char c;
        read(STDIN_FILENO, &c, 1);

        if (c == '\x1b') {  // Escape sequence start
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A':  // Up arrow (History previous)
                        if (current_history > 0) {
                            current_history--;
                            strcpy(buffer, command_history[current_history]);
                            length = strlen(buffer);
                            cursor_position = length;
                            printf("\33[2K\r> %s", buffer);  // Clear line and print command
                        }
                        break;

                    case 'B':  // Down arrow (History next)
                        if (current_history < history_count - 1) {
                            current_history++;
                            strcpy(buffer, command_history[current_history]);
                            length = strlen(buffer);
                            cursor_position = length;
                            printf("\33[2K\r> %s", buffer);  // Clear line and print command
                        } else if (current_history == history_count - 1) {
                            current_history++;
                            buffer[0] = '\0';  // Clear the current buffer
                            length = 0;
                            cursor_position = 0;
                            printf("\33[2K\r> ");  // Clear line and print empty prompt
                        }
                        break;

                    case 'C':  // Right arrow (Move cursor right)
                        if (cursor_position < length) {
                            cursor_position++;
                            printf("\033[C");
                        }
                        break;

                    case 'D':  // Left arrow (Move cursor left)
                        if (cursor_position > 0) {
                            cursor_position--;
                            printf("\033[D");
                        }
                        break;
                }
            }
        } else if (c == '\n') {  // Enter key
            buffer[length] = '\0';  // Null-terminate the command
            printf("\nYou entered: %s\n", buffer);

            if (length > 0) {
                // Store command in history
                if (history_count < HISTORY_SIZE) {
                    history_count++;
                }
                history_index = (history_index + 1) % HISTORY_SIZE;
                strcpy(command_history[history_index], buffer);
                current_history = history_count;  // Reset history navigation index
            }

            length = 0;
            cursor_position = 0;
            printf("> ");  // New prompt
        } else if (c == 127) {  // Backspace key
            if (cursor_position > 0) {
                // Shift everything left by one
                for (int i = cursor_position - 1; i < length; i++) {
                    buffer[i] = buffer[i + 1];
                }
                length--;
                cursor_position--;

                // Clear the line and redraw the buffer
                printf("\33[2K\r> %s", buffer);
                for (int i = cursor_position; i < length; i++) {
                    printf("\033[C");
                }
            }
        } else {  // Regular characters
            if (length < MAX_COMMAND_LENGTH - 1) {
                // Insert the character at the cursor position
                for (int i = length; i > cursor_position; i--) {
                    buffer[i] = buffer[i - 1];
                }
                buffer[cursor_position] = c;
                length++;
                cursor_position++;

                // Clear the line and redraw the buffer
                printf("\33[2K\r> %s", buffer);
                for (int i = cursor_position; i < length; i++) {
                    printf("\033[C");
                }
            }
        }
    }
}

// Signal handler for Ctrl+C to restore terminal settings
void signal_handler(int signum) {
    restore_terminal();
    printf("\nTerminal settings restored. Exiting...\n");
    _exit(0);  // Safely exit
}

int main() {
    // Set non-canonical mode
    set_non_canonical_mode();

    // Register signal handler for Ctrl+C (SIGINT)
    signal(SIGINT, signal_handler);

    // Prompt and start handling input
    printf("> ");
    handle_input();

    // Restore terminal settings on exit
    restore_terminal();

    return 0;
}
