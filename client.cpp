#include <iostream>
#include <cstdlib>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <algorithm>
#include <Ws2tcpip.h>
#include<io.h>
#include <Winsock2.h>
#include<windows.h>
#ifdef WIN32
    #include <winsock.h>
	#include <winsock2.h>
    typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <sys/un.h>
#endif
#pragma comment(lib, "Ws2_32.lib")
// For sockets
#include <unistd.h>
#include <fcntl.h> // for non-blocking sockets
// .h file
#include "player.cpp"
#include "packet.cpp"
#include<SFML/system.hpp> //

using namespace std;

#define MAX_NICKNAME_LENGTH 10
#define SERVER_PORT 8888

// global variable
char server_ip[16];
char nickname[MAX_NICKNAME_LENGTH + 1];
struct sockaddr_in server_addr;
int server_socket;
int race_length;
int answer_time;
bool in_progress=false;
int round=0;
vector<Player> players;

void read_packet(Packet &p){
    int nbytes_read;
    int l;
    char* length= reinterpret_cast<char*>(&l);
    char* point = reinterpret_cast<char*>(&p.point);
    char* position = reinterpret_cast<char*>(&p.position);
    nbytes_read=recv(server_socket,length,2,0);
    // do{
    // if(errno == EAGAIN || errno == EWOULDBLOCK){
    //     // cout << "Waiting for server";
    //     sleep(1);
    // }
    // else{
    l= atoi(length);
    nbytes_read=recv(server_socket,p.Context,l,0);
    nbytes_read=recv(server_socket,point,2,0);
    p.point= atoi(point);
    nbytes_read=recv(server_socket,position,2,0);
    p.position= atoi(position);
    // cout << "Length: " << l << " Context: " << p.Context << " Point: " << p.point << " Position: " << p.position << endl;
    // }
    // }while(nbytes_read<0);
}

int main() {
    // cout << "Enter server IP: ";
    // cin.getline(server_ip, 16);
    // server_ip= "127.0.0.1";
    // create socket
    // cout<<0;
    sf::Clock clock;
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // int flags = fcntl(server_socket, F_GETFL, 0);
    // fcntl(server_socket, F_SETFL,  flags | O_NONBLOCK);
    u_long mode =1;
    int result = ioctlsocket(server_socket, FIONBIO, &mode);
    
    // cout<<result;
    
    // connect to server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    // tam thoi la test = local host
    // if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    //     perror("inet_pton failed");
    //     exit(EXIT_FAILURE);
    // }
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // cout<<"hi"<<endl;
    // inet_addr("127.0.0.1");
    
    
    // ham while fake
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        // cout<<1;
        // if (errno == EINPROGRESS || errno == EALREADY)
        // cout << "Still connecting" << endl;
        // sleep(1);
        
        int error_code = WSAGetLastError();
        // cout<<error_code<<endl;
        if (error_code == WSAEWOULDBLOCK || error_code == WSAEINPROGRESS)
        cout << "Still connecting" << endl;
        Sleep(100); // Sleep for 1 second
    }

    // cout<<2;

    // Get player nickname from user input
    bool valid_nickname = false;
    do {
    cout << "Enter nickname (up to 10 characters): ";
    cin.getline(nickname,10);
    cout << nickname << endl;
    int bytes_sent1=-1;
    while(bytes_sent1<0) {
        bytes_sent1 = send(server_socket,(char*) nickname, sizeof(nickname), 0);
        if(bytes_sent1==0){
            cout << "Connection closed, can't send" << endl;
            return 1;
            }
        else if (bytes_sent1<0){
            int error_code= WSAGetLastError();
            if (error_code == WSAEWOULDBLOCK)
                cout << "Buffer is full so non-blocking socket won't work"<< endl;
            else
                cout << "Send error" << endl;
            return 1;
            }
        }
        char validation[50];
        
        int byets_received1;
        do{
        byets_received1 = recv(server_socket, validation, 50, 0);
        if (byets_received1 <1) {
            int error_code= WSAGetLastError();
            if (error_code == WSAEWOULDBLOCK) {
                cout << "Waiting for nickname vaildation" << endl;
                // The socket is temporarily unavailable, wait and try again
                Sleep(1000);
            } 
            else {
                cout << "Error recv data" << endl;
                return 1;
            }
        } 
        else{
            string tem02(validation);
            // cout<<"check";
            // cout<<validation<<endl;
            if(tem02.substr(0,35)=="Registration Completed Successfully")
            {   valid_nickname=true;
                cout << validation;
                break;

                }
            else{
                cout<<"wrong valid"<<validation<<endl;
                cout << "Nickname not valid, try again." << endl;
            }
            }
        }
        while(byets_received1<0);
    }
    while (!valid_nickname);

    Player player1(nickname);
    players.push_back(player1);
    do {
        char buffer[10];
        int bytes_recv = recv(server_socket, buffer,1,0);
        if(bytes_recv<0){
            int error_code= WSAGetLastError();
            if (error_code == WSAEWOULDBLOCK){
                cout << "Waiting for server to start the game" << endl;
                sleep(1);
                continue;
                }
        }
        else if (bytes_recv==0){
            cout << "Server has closed connection" << endl;
            return -1;
        }
        else{
            Packet p1;
            read_packet(p1);
            race_length= p1.point;
            answer_time= p1.position;
            cout << "type:" << buffer << endl;
            cout << "The game will start now: " << endl;
            cout << "The race length will be: " << race_length << endl;
            cout << "Time to answer a question will be: " << answer_time << " seconds" << endl;
            in_progress=true;
        }
    } while(!in_progress);

    while (in_progress) {
        // prepare for the first round, create a player object for each player the server send back
        if(round==0){
            int bytes_received2;
            char buffer2[10];
            int tem1=0;
            do{
            bytes_received2= recv(server_socket, buffer2, 1,0);
            if(bytes_received2<0){
                int error_code= WSAGetLastError();
                    if (error_code == WSAEWOULDBLOCK){
                    cout << "Waiting for server to start the first round of the game" << endl;
                    sleep(1);
                }
                else {
                    // cout << "Error recv first round data" << endl;
                    // return 1;
                    }
            }
            else{
                while(tem1<10){
                    Packet p2;
                    read_packet(p2);
                    Player player2(p2.Context);
                    players.push_back(player2);
                    tem1++;
                    }
                }
            }
            while(bytes_received2<0);
        } 
        else{
            // update the point of position of each player before the round
            char buffer3[10];
            int bytes_received3;
            int tem1=0;
            do{
                bytes_received3= recv(server_socket, buffer3, 1,0);
                if(bytes_received3<0){
                    int error_code= WSAGetLastError();
                    if (error_code == WSAEWOULDBLOCK){
                        cout << "Waiting for server to start a new round" << endl;
                        sleep(1);}
                    else {
                        cout << "Error recv data" << endl;
                        // return 1;
                        }
                }
                else{
                    while(tem1<players.size()){
                        Packet p2;
                        read_packet(p2);
                        for (auto x : players){
                            if(p2.Context==x.nickname){
                                x.points=p2.point;
                                x.position=p2.position;
                                cout<<x.nickname<<" ,point: "<<x.points<<"position: "<<x.position<<endl;
                            }
                    }
                        tem1++;
                    }
                }
            }
            while(bytes_received3<0);
            // check for disqualified players and vitors
            for (auto i = players.begin(); i != players.end();i++){
                if(i->position==-1 && i->nickname != nickname)
                    players.erase(i);
                else if(i->position==-1 && i->nickname == nickname){
                    cout << "You have been killed by the cruelty of this race." << endl;
                    break;
                }
                if(i->position>=race_length){
                    if(i->nickname == nickname){
                    cout << "Victory achieved" << endl;
                    in_progress=false;
                    }
                    else{
                    cout << "Victory achieved, but not by you lol" << endl;
                    in_progress=false;
                    }
                }
            }
        }
        if(!in_progress)
            break;
        // where player get question and answer
        int answer;
        round++;
        cout << "Round " << round << " will start now" << endl;
        int bytes_received4;
        do{
            char q_buffer[10];
            bytes_received4=recv(server_socket,q_buffer,1,0);
            if(bytes_received4<0){
                int error_code= WSAGetLastError();
                    if (error_code == WSAEWOULDBLOCK)
                {
                    cout << "Waiting for server to send the question" << endl;
                    sleep(1);
                }
                else {
                    // cout << "Error recv question" << endl;
                    // return 1;
                    }
            }
            else{
                Packet p2;
                read_packet(p2);
                cout << "Question: " << p2.Context << endl;
            }
        }while(bytes_received4<0);

        cout << "Your answer:";
        cin >> answer;
        while (cin.fail()) {
            cout << "Invalid input. Type again" << endl;
            cin.clear(); // clear the error state
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // ignore any remaining characters in the input stream
            cin >> answer;
        }
        char s_answer[10];
        snprintf(s_answer, sizeof(s_answer), "%d", answer);
        send(server_socket, s_answer,strlen(s_answer), 0);
        // show the solution to the players
        int bytes_received5;
        do{
            char s_buffer[10];
            bytes_received5= recv(server_socket, s_buffer, 1,0);
            if(bytes_received5==-1){
                int error_code= WSAGetLastError();
                    if (error_code == WSAEWOULDBLOCK){
                cout << "Waiting for server to send the solution" << endl;
                sleep(1);
                }
            else{
                // cout << "Error recving solution:" << endl;
                // return 1;
                }
            }
            else{
            Packet p2;
            read_packet(p2);
            cout << "Solution: " << p2.Context << " Point earned: "<< p2.point << endl;
            }
        }while(bytes_received5<0);
    }
    // Close socket
    close(server_socket);
    return 0;
}

