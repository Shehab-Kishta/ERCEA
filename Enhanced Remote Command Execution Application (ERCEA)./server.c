#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX 100
#define PORT 3500

// Function to handle SIGCHLD to avoid zombie processes
void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to handle SIGINT to clean up server socket
void sigint_handler(int signo) {
    printf("Server is shutting down...\n");
    exit(0);
}

int main() {
    // Create the socket with IPv4 domain and TCP protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // Set options for the socket
    int option_value = 1;
    int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
                            &option_value, sizeof(option_value));
    int client_sockfd;
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
  
    if (status < 0) {
        perror("Couldn't set options");
        exit(EXIT_FAILURE);
    }
  
    // Initialize structure elements for address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
    // Bind the socket with address/port from the sockaddr_in struct
    status = bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (status < 0) {
        perror("Couldn't bind the socket");
        exit(EXIT_FAILURE);
    }

    // Listen on specified port for up to 5 connections
    status = listen(sockfd, 5);
    if (status < 0) {
        perror("Couldn't listen for connections");
        exit(EXIT_FAILURE);
    }

    // Register signal handlers
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);

    printf("Server is listening on port %d\n", PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t length_of_address = sizeof(client_addr);
        client_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &length_of_address);
        if (client_sockfd < 0) {
            perror("Couldn't establish connection with client");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            close(client_sockfd);
            continue;
        }

        if (pid == 0) { // Child process
            close(sockfd); // Child doesn't need the listening socket

            while (1) {
                char received_command[MAX];
                memset(received_command, 0, MAX);
                
                ssize_t bytes_read = read(client_sockfd, received_command, MAX - 1);
                if (bytes_read < 0) {
                    perror("Error reading from client");
                    break;
                } else if (bytes_read == 0) {
                    printf("Client disconnected.\n");
                    break;
                }

                received_command[bytes_read] = '\0';
                printf("Command received from client: %s\n", received_command);

                if (strncmp(received_command, "stop", 4) == 0) {
                    printf("Stopping communication with client\n");
                    break;
                }

                int command_return = system(received_command);
                int return_value = htonl(command_return);
                send(client_sockfd, &return_value, sizeof(return_value), 0);
            }

            close(client_sockfd);
            exit(0);
        } else {
            close(client_sockfd); // Parent process closes client socket
        }
    }

    close(sockfd);
    return 0;
}
