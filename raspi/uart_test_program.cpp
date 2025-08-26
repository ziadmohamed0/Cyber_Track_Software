#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

#define DEVICE_PATH "/dev/rpi4_uart"
#define BUFFER_SIZE 256

static int uart_fd = -1;
static int running = 1;

void signal_handler(int sig) {
    running = 0;
    printf("\nShutting down...\n");
}

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -t <text>    Send text and exit\n");
    printf("  -f <file>    Send file contents\n");
    printf("  -i           Interactive mode (default)\n");
    printf("  -r           Read-only mode\n");
    printf("  -h           Show this help\n");
}

int send_text(const char *text) {
    int bytes_written = write(uart_fd, text, strlen(text));
    if (bytes_written < 0) {
        perror("Error writing to UART");
        return -1;
    }
    printf("Sent %d bytes: %s\n", bytes_written, text);
    return bytes_written;
}

int send_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }
    
    char buffer[BUFFER_SIZE];
    int total_bytes = 0;
    
    while (fgets(buffer, sizeof(buffer), file) && running) {
        int bytes_written = write(uart_fd, buffer, strlen(buffer));
        if (bytes_written < 0) {
            perror("Error writing to UART");
            break;
        }
        total_bytes += bytes_written;
        usleep(1000); // Small delay between lines
    }
    
    fclose(file);
    printf("Sent %d bytes from file %s\n", total_bytes, filename);
    return total_bytes;
}

void interactive_mode() {
    printf("Interactive UART Terminal (Press Ctrl+C to exit)\n");
    printf("Type messages to send via UART:\n");
    
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int max_fd = (uart_fd > STDIN_FILENO) ? uart_fd : STDIN_FILENO;
    
    while (running) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);  // Monitor stdin
        FD_SET(uart_fd, &readfds);       // Monitor UART
        
        struct timeval timeout = {1, 0}; // 1 second timeout
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select error");
            break;
        }
        
        if (activity == 0) continue; // Timeout
        
        // Check for input from stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, sizeof(buffer), stdin)) {
                if (write(uart_fd, buffer, strlen(buffer)) < 0) {
                    perror("Error writing to UART");
                    break;
                }
            }
        }
        
        // Check for data from UART
        if (FD_ISSET(uart_fd, &readfds)) {
            int bytes_read = read(uart_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Received: %s", buffer);
                fflush(stdout);
            } else if (bytes_read < 0 && errno != EAGAIN) {
                perror("Error reading from UART");
                break;
            }
        }
    }
}

void read_only_mode() {
    printf("Read-only UART Monitor (Press Ctrl+C to exit)\n");
    
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    
    while (running) {
        FD_ZERO(&readfds);
        FD_SET(uart_fd, &readfds);
        
        struct timeval timeout = {1, 0}; // 1 second timeout
        int activity = select(uart_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select error");
            break;
        }
        
        if (activity == 0) continue; // Timeout
        
        if (FD_ISSET(uart_fd, &readfds)) {
            int bytes_read = read(uart_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Received: %s", buffer);
                fflush(stdout);
            } else if (bytes_read < 0 && errno != EAGAIN) {
                perror("Error reading from UART");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int opt;
    char *text_to_send = NULL;
    char *file_to_send = NULL;
    int interactive = 1;
    int read_only = 0;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "t:f:irh")) != -1) {
        switch (opt) {
            case 't':
                text_to_send = optarg;
                interactive = 0;
                break;
            case 'f':
                file_to_send = optarg;
                interactive = 0;
                break;
            case 'i':
                interactive = 1;
                break;
            case 'r':
                read_only = 1;
                interactive = 0;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Install signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Open UART device
    uart_fd = open(DEVICE_PATH, O_RDWR | O_NONBLOCK);
    if (uart_fd < 0) {
        perror("Error opening UART device");
        printf("Make sure the kernel module is loaded and device exists\n");
        return 1;
    }
    
    printf("UART device opened successfully\n");
    
    // Execute requested operation
    if (text_to_send) {
        send_text(text_to_send);
    } else if (file_to_send) {
        send_file(file_to_send);
    } else if (read_only) {
        read_only_mode();
    } else if (interactive) {
        interactive_mode();
    }
    
    // Cleanup
    if (uart_fd >= 0) {
        close(uart_fd);
    }
    
    return 0;
}

// Compile with: gcc -o uart_test uart_test.c