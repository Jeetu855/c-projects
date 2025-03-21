#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PROXY_PORT 9050
#define BUFFER_SIZE 4096

// SOCKS4 Request Packet Structure
struct socks4_request {
    unsigned char version;    // Should be 4
    unsigned char command;    // 1 = CONNECT
    unsigned short dstport;   // Destination port (network byte order)
    unsigned int dstip;       // Destination IP (network byte order)
    char userid[256];         // User ID (variable size, ends with NULL)
};

// SOCKS4 Response Packet Structure
struct socks4_response {
    unsigned char null_byte;  // Always 0x00
    unsigned char code;       // 90 (success), 91 (failure)
    unsigned short dstport;   // Echoed destination port
    unsigned int dstip;       // Echoed destination IP
};

// Function to handle a SOCKS4 request
void handle_client(int client_sock) {
    struct socks4_request req;
    struct socks4_response res;
    int remote_sock;
    struct sockaddr_in remote_addr;
    char buffer[BUFFER_SIZE];
    int bytes_read, bytes_sent;

    // Read the request from the client
    if (read(client_sock, &req, sizeof(req)) < 8) {
        perror("Failed to read SOCKS4 request");
        close(client_sock);
        return;
    }

    // Validate SOCKS4 request
    if (req.version != 4 || req.command != 1) {
        fprintf(stderr, "Invalid SOCKS4 request\n");
        close(client_sock);
        return;
    }

    // Convert values to host byte order
    unsigned short port = ntohs(req.dstport);
    struct in_addr ip_addr;
    ip_addr.s_addr = req.dstip;

    printf("SOCKS4 request: Connect to %s:%d\n", inet_ntoa(ip_addr), port);

    // Create a socket to connect to the destination server
    if ((remote_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        close(client_sock);
        return;
    }

    // Set up destination server details
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = req.dstport; // Already in network byte order
    remote_addr.sin_addr.s_addr = req.dstip;

    // Try connecting to the destination server
    if (connect(remote_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        perror("Connection to destination failed");

        // Send SOCKS4 failure response
        res.null_byte = 0x00;
        res.code = 91;  // Request rejected or failed
        res.dstport = req.dstport;
        res.dstip = req.dstip;
        write(client_sock, &res, sizeof(res));

        close(client_sock);
        close(remote_sock);
        return;
    }

    printf("Connected to destination %s:%d\n", inet_ntoa(ip_addr), port);

    // Send SOCKS4 success response
    res.null_byte = 0x00;
    res.code = 90;  // Success
    res.dstport = req.dstport;
    res.dstip = req.dstip;
    write(client_sock, &res, sizeof(res));

    // Relay data between client and remote server
    fd_set fds;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(client_sock, &fds);
        FD_SET(remote_sock, &fds);

        if (select((client_sock > remote_sock ? client_sock : remote_sock) + 1, &fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            break;
        }

        if (FD_ISSET(client_sock, &fds)) {
            bytes_read = read(client_sock, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) break;
            send(remote_sock, buffer, bytes_read, 0);
        }

        if (FD_ISSET(remote_sock, &fds)) {
            bytes_read = read(remote_sock, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) break;
            send(client_sock, buffer, bytes_read, 0);
        }
    }

    printf("Closing connection with client\n");
    close(client_sock);
    close(remote_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create a TCP socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PROXY_PORT);

    // Bind socket to port 9050
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("SOCKS4 proxy server running on port %d...\n", PROXY_PORT);

    // Accept and handle client connections
    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) >= 0) {
        printf("Client connected\n");
        handle_client(client_sock);
    }

    // Close server socket
    close(server_sock);
    return 0;
}
