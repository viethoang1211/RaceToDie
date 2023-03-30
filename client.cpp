#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <algorithm>

// For sockets
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// .h file
#include "player.cpp"

using namespace std;

#define MAX_NICKNAME_LENGTH 10
#define SERVER_PORT 8888
int main() {
    char server_ip[16];
    char nickname[MAX_NICKNAME_LENGTH + 1];
    struct sockaddr_in server_addr;
    int server_socket;

    // Get server IP from user input
    cout << "Enter server IP: ";
    cin.getline(server_ip, 16);


    // create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // connect to server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Main game loop
    while (true) {
        // Get player nickname from user input
        bool valid_nickname = true;
        do {
        cout << "Enter nickname (up to 10 characters): ";
        cin >> nickname;
        // TODO: Check if nickname is valid (e.g., only contains allowed characters)
        // TODO: Send nickname to server and wait for response
        // If server responds with "Registration Completed Successfully"
        // then set valid_nickname = true and continue
        
        int bytes_sent = send(server_socket, nickname, strlen(nickname), 0);
        if(bytes_sent==0){

        }
        else{

        }
        char validation[10];
        int bytes_received = recv(server_socket, validation, 10, 0);
        
    } while (!valid_nickname);
        Player player1(nickname);
        // TODO: Get player position on track (e.g., using random number)
        // TODO: Send player position to server
        // TODO: Wait for response from server (e.g., game over message)
        // If game over message, break out of loop
    }

    // Close socket
    close(server_socket);
    return 0;
}

