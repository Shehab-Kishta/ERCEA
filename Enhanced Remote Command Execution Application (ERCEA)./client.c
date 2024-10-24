#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX 100

int main(int argc, char *argv[]) {
    int status;
    int socket_descriptor;
    char *ip_address = argv[1];
    int port_number = atoi(argv[2]);
    char command[MAX];
    int command_return;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP Address> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    server_address.sin_addr.s_addr = inet_addr(ip_address);

    status = connect(socket_descriptor, (struct sockaddr *)&server_address, sizeof(server_address));
    if (status < 0) {
        perror("Couldn't connect with the server");
        close(socket_descriptor);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Please enter the command: ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("Failed to read command");
            break;
        }

        size_t length = strlen(command);
        if (length > 0 && command[length - 1] == '\n') {
            command[length - 1] = '\0';
        }

        ssize_t bytes_written = write(socket_descriptor, command, strlen(command));
        if (bytes_written < 0) {
            perror("Failed to send command");
            break;
        }

        if (strncmp(command, "stop", 4) == 0) {
            printf("Terminating client as 'stop' command was sent.\n");
            break;
        }

        ssize_t bytes_read = read(socket_descriptor, &command_return, sizeof(int));
        if (bytes_read < 0) {
            perror("Failed to receive data");
            break;
        }

        printf("Message from server: %d\n", ntohl(command_return));
    }

    close(socket_descriptor);
    return 0;
}
