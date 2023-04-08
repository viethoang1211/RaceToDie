#include <iostream>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <algorithm>
#ifdef WIN32
    #include <winsock.h>
	#include <winsock2.h>
    typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <sys/un.h>
#endif
// For sockets
#include <unistd.h>
#include <fcntl.h> // for non-blocking sockets
// .h file
#include "player.cpp"
#include "packet.cpp"
using namespace std;

#define MAX_NICKNAME_LENGTH 10
#define SERVER_PORT 8888

int main() {
    char server_ip[16];
    char nickname[MAX_NICKNAME_LENGTH + 1];
    struct sockaddr_in server_addr;
    int server_socket;
    int race_length;
    bool in_progress=false;
    int round=0;
    // Get server IP from user input
    // cout << "Enter server IP: ";
    // cin.getline(server_ip, 16);
    // server_ip= "127.0.0.1";
    // create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL,  flags | O_NONBLOCK);
    // connect to server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    // tam thoi la test = local host
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }
    while (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EINPROGRESS || errno == EALREADY)
        cout << "Still connecting" << endl;
        sleep(1);
    }
    // Get player nickname from user input
    bool valid_nickname = false;
    do {
    cout << "Enter nickname (up to 10 characters): ";
    cin >> nickname; 
    int bytes_sent1=-1;
    while(bytes_sent1<0) {
        bytes_sent1 = send(server_socket, nickname, strlen(nickname), 0);
        if(bytes_sent1==0){
            cout << "Connection closed, can't send" << endl;
            return 1;
            }
        else if (bytes_sent1<0){
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                cout << "Buffer is full so non-blocking socket won't work"<< endl;
            else
                cout << "Send error" << endl;
            return 1;
            }
        }
        char validation[10];
        int byets_received1;
        do{
        byets_received1 = recv(server_socket, validation, 10, 0);
        if (byets_received1 == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                cout << "Waiting for nickname vaildation" << endl;
                // The socket is temporarily unavailable, wait and try again
                sleep(1);
            } 
            else {
                cout << "Error recv data" << endl;
                return 1;
            }
        } 
        else{
            if(strcmp(validation,"Registration Completed Successfully \r\n"))
                valid_nickname=true;
            }
        }
        while(byets_received1<0);
    }
    while (!valid_nickname);

    Player player1(nickname);
    do {
        char buffer[1024];
        int bytes_recv = recv(server_socket, buffer,1024,0);
        if(bytes_recv<0){
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                cout << "Waiting for server to start the game" << endl;
        }
        else if (bytes_recv==0){
            cout << "Server has closed connection" << endl;
            return -1;
        }
        else{
            cout << "The length of the game will be:" << buffer << endl;
            race_length= stoi(buffer);
            in_progress=true;
        }
        delete buffer;
    } while(!in_progress);

    while (in_progress) {
        char q_buffer[1024];
        char answer[1024];
        round++;
        cout << "Round " << round << " will start now" << endl;
        int bytes_received2;
        while((bytes_received2 = recv(server_socket, q_buffer, 1024,0)) < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK)
                cout << "Waiting for server to start the game" << endl;
        else{
            Packet p1;
            cout << "Question: " << q_buffer << endl;
            
        }
        }
        cout << "Your answer:";
        cin.getline(answer, 16);
        delete q_buffer;
    }
    // Close socket
    close(server_socket);
    return 0;
}

