#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>

#define PORT 1234
#define BUFFER_SIZE 1024

#define TIMEOUT_SECONDS 0
#define TIMEOUT_MICROSECONDS 100000 //100ms

class TicToc{
private:
    std::chrono::high_resolution_clock::time_point start_time;
public:
    void tic() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed_ms() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        return elapsed;
    }
};

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    struct timeval timeout;


    // Set the timeout value for read
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = TIMEOUT_MICROSECONDS;
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Attach socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for a connection on port %d...\n", PORT);



    while(true){
        // Accept incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted!\n");

        // Set socket receive timeout
        if (setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // Read commands from client
        TicToc tt;
        tt.tic();
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(new_socket, buffer, BUFFER_SIZE - 1);
            if(valread<0) {
                if(tt.elapsed_ms() > 10000){
                    printf("Connection timeout (10s) => disconnect client\n");
                    break;
                }
                continue;//no data
            }
            //disconnected
            if (valread == 0) {
                printf("Client disconnected.\n");
                break;
            }
            tt.tic();

            printf("Read %d bytes \n",valread);
            printf("Command received: %s\n", buffer);

            // Echo back the command to client
            send(new_socket, buffer, strlen(buffer), 0);
        }

        close(new_socket);
    }
    close(server_fd);
    return 0;
}
