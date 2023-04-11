#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cstdlib>
#include <random>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
// For sockets
// #include <sys/socket.h>
// #include <arpa/inet.h>

#ifdef WIN32
    #include <winsock.h>
	#include <winsock2.h>
    typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <sys/un.h>
#endif

#pragma comment(lib, "ws2_32.lib")

#include <unistd.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <fcntl.h> // for non-blocking sockets
// .h file
#include "player.cpp"
#include "message.cpp"
#include "packet.cpp"
using namespace std;

// Constants
const int MAX_CLIENTS = 10; // Maximum number of clients
const int MIN_CLIENTS = 2;
const int MIN_LENGTH = 3; // Minimum length of the race
const int MAX_LENGTH = 26; // Maximum length of the race
const int QUESTION_TIME = 25; // Time in seconds to answer each question
const int MAX_NICKNAME_LENGTH=10; // Lenght of client name

// Global variables
int quesion_time=25;
int race_length;
int player_count = 0;
vector<Player> players;
vector<bool> disqualified;
int master_socket;
int max_sd;  
// queue<int> turn_order;
// mutex mtx;
//set of socket descriptors 
fd_set readfds;  
// 1. register
bool is_valid_nickname(const string& nickname) {
    if (nickname.empty() || nickname.length() > MAX_NICKNAME_LENGTH) {
        return false;
    }
    for (char c : nickname) {
    	// isalnum check whether c is either a decimal digit or an uppercase or lower case letter ( from library)
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    // check xem co ai trung ten khong
    for (auto p : players) {
        if (p.nickname == nickname) {
            return false;
        }
    }
    return true;
}

// 3a. random so ngau nhien trong khoang min,max
int getRandomInt(int min, int max) {
    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(min, max); // define the range
  return distr(gen); // generate the number
}
// 3a. lay random operator
char getRandomOperator(){
	int x = rand()%5;
	switch(x){
		case 0:
			return '%';
			break;
		case 1:
			return '+';
			break;
		case 2:
			return '-';
			break;
		case 3:
			return '*';
			break;
		case 4:
			return '/';
			break;
}}

// 3c. tinh ket qua cua phep tinh
int calculateAnswer(int a, int b, char op) {
    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':	
            if (b == 0) {
                throw runtime_error("Division by zero");
            }
            return a / b;
        case '%':
            if (b == 0) {
                throw runtime_error("Modulo by zero");
            }
            return a % b;
        default:
            throw runtime_error("Invalid operator");
    }}

//3c iv update position
void update_pos(){
    for (int i = 0; i < players.size(); i++) {
            players[i].position += players[i].points;
            if (players[i].position < 1)
                players[i].position = 1;
            else if (players[i].position > race_length)
                players[i].position = race_length;
        }
}

void announce(string message) {

}

void announce_start_game(){
    char type[10];
    char length[10];
    char point[10];
    char position[10];
    // char* buffer1 = reinterpret_cast<char*>(&race_length);
    // char* buffer2 = reinterpret_cast<char*>(&quesion_time);
    // strcpy(msg,buffer1);
    snprintf(type, sizeof(type), "%d", 2);

    snprintf(length, sizeof(length), "0%d", 0);

    if(race_length<9)
        snprintf(point, sizeof(point), "0%d", race_length);
    else
        snprintf(point, sizeof(point), "%d", race_length);

    if(quesion_time<9)
        snprintf(position,sizeof(position), "0%d", quesion_time);
    else
        snprintf(position, sizeof(position), "%d", quesion_time);

    char msg[100];
    strcpy(msg,type);
    strcat(msg,length);
    strcat(msg,point);
    strcat(msg,position);
    cout <<"Start game message: "<< msg << endl;
    for(auto x: players){
        send(x.socketID,msg,strlen(msg),0);
    }
}
void announce_new_round(){
    for(auto x: players){
        for(auto i: players){
            char type[10];
            char length[10];
            char point[10];
            char position[10];
            snprintf(type, sizeof(type), "%d", 1);
            if(strlen(i.nickname.c_str())<9)
            snprintf(length, sizeof(length), "0%d", strlen(i.nickname.c_str()));
            else
            snprintf(length, sizeof(length), "%d", strlen(i.nickname.c_str()));
            snprintf(point, sizeof(point), "0%d",0 );

            if(i.position<9)
            snprintf(position,sizeof(position), "0%d", i.position);
            else
            snprintf(position, sizeof(position), "%d", i.position);
            // cout << type << " " << length << " "<< i.nickname.c_str() << " " << point << " " << position << endl;
            char msg[100];
            strcpy(msg,type);
            strcat(msg,length);
            strcat(msg, i.nickname.c_str());
            strcat(msg,point);
            strcat(msg,position);
            cout <<"New round message: "<< msg << endl;
            int bytes_sent = send(x.socketID, msg, strlen(msg), 0);
            cout<<"Bytes sent: "<< bytes_sent<<endl;
            if (bytes_sent == -1) {
            cout << "Send error at player:" << x.nickname << endl;
            return;
            }
        }
    }
}
void announce_question(string question){
    char type[10];
    char length[10];
    char point[10];
    char position[10];
    snprintf(type, sizeof(type), "%d", 3);
    snprintf(length, sizeof(length), "%d", strlen(question.c_str()));
    if(strlen(question.c_str())<9)
    snprintf(length, sizeof(length), "0%d", strlen(question.c_str()));
    else
    snprintf(length, sizeof(length), "%d", strlen(question.c_str()));
    snprintf(point, sizeof(point), "0%d", 0);
    snprintf(position, sizeof(position), "0%d", 0);
    // cout << type << " " << length << " "<< question.c_str() << " " << point << " " << position << endl;
    char msg[100];
    strcpy(msg,type);
    strcat(msg,length);
    strcat(msg, question.c_str());
    strcat(msg,point);
    strcat(msg,position);
    cout <<"Question or answer message: "<< msg << endl;
    cout <<players.size() << endl;
    for (auto i: players) {
        int bytes_sent = send(i.socketID, msg, strlen(msg), 0);
        if (bytes_sent == -1) {
            cout << "Send error at player:" << i.nickname << endl;
            return;
        }
    }
    // delete msg;
}
void announce_solution(){
    
}
// 3. each turn 
void playSet( int playerCount, vector<Player>& players, int questionTimeLimit) {
    int currentPlayerIndex = 0;
    int questionCount = 0;
    bool winnerFound = false;
    // Toan bo message duoc nhan o server
    vector<Message> messages;
    int valread;
    race_length = getRandomInt(MIN_LENGTH, MAX_LENGTH);
    struct timeval timeout;
    timeout.tv_sec= questionTimeLimit;
    timeout.tv_usec= 0;
    announce_start_game();
    sleep(2);
    // Loop until a winner is found
    while (!winnerFound) {
        announce_new_round();
        sleep(2);
        messages.clear();
        // 3a.  Get the two random integers and operator for the question
        int a = getRandomInt(-10000, 10000);
        int b = getRandomInt(-10000, 10000);
        char op = getRandomOperator();

        // Make question and send the question to all player
        string question = to_string(a) + " " + string(1, op) + " " + to_string(b);
        announce_question(question);
        // auto start = std::chrono::high_resolution_clock::now(); // get the start time
        bool answers_received=false;
        while (!answers_received) 
        {
            cout << "Test point" << endl;
            FD_ZERO(&readfds);    
            //add master socket to set 
            FD_SET(master_socket, &readfds);  
            max_sd = master_socket;  
            //add child sockets to set 
            int sd;
            for ( int i = 0 ; i < players.size() ; i++)  
            {  
                //socket descriptor 
                sd = players[i].socketID;            
                //if valid socket descriptor then add to read list 
                if(sd > 0)  
                    FD_SET( sd , &readfds);    
                //highest file descriptor number, need it for the select function 
                if(sd > max_sd)  
                    max_sd = sd;  
            }     
            int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);    
            if ((activity < 0) && (errno!=EINTR))  
            {  
                cout << "select in receving answers error" << endl; 
            }        
            for (auto i = players.begin(); i != players.end();i++)  
            {  
                int sd = i->socketID;           
                if (FD_ISSET(sd, &readfds))  
                {  
                    //Check if it was for closing , and also read the incoming message
                    char buffer[20];    
                    if ((valread = read(sd,buffer,20)) == 0)  
                    {  
                        close(sd);  
                        players.erase(i);
                    } 
                    else 
                    {  
                        cout << "Answers received:" << endl;
                        Message msg(i->socketID, std::chrono::system_clock::now(), buffer);
                        messages.push_back(msg);
                    } 
                }        
            }
            // auto now = std::chrono::high_resolution_clock::now(); // get the current time
            // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count(); // get the elapsed time in milliseconds
            // if (duration >= 5000) { // if the elapsed time is greater than or equal to 5000 milliseconds (5 seconds)
            //     break; // exit the loop
            //}
            if(messages.size()>=players.size())
                answers_received=true;
        }
        int correctAnswer = calculateAnswer(a, b, op);
        int fastestPlayerIndex = -1;
        int pointForHighest = 0;
        char str[10];
        // Convert integer to string
        // itoa(correctAnswer, str, 10);
        string correctA= to_string(correctAnswer);
        announce_question(correctA);
        vector<Message> message_copy(messages);
        // xoa message nguoi sai
        for (auto x = message_copy.begin(); x != message_copy.end(); ) {
            if (x->text != to_string(correctAnswer)) {
                message_copy.erase(x);
            }
            else {
                ++x;
            }
        }

        sort(message_copy.begin(), message_copy.end());
        // kiem tra xem ai nhanh nhat  ~ message[message.size()-1] 

        // nguoi nhanh nhat co ton tai khong
        bool fattestExist = false;
        if (message_copy.size() > 0) {
            fattestExist = true;
        }
        // ID cua nguoi nhanh nhat
        int fattestPlayerID=-1;
        if (fattestExist) {
            fattestPlayerID = message_copy.end()->clientId;
        }
        // tinh diem cho tung nguoi 
        for (auto x : players) {
            x.points = -1;
        }
        int countWrong = 0;
        for (int i = 0; i < players.size(); i++) {
            if (players[i].socketID != fattestPlayerID) {
                for (auto j : messages) {
                    if (j.clientId == players[i].socketID) {
                        if (to_string(correctAnswer) == j.text) {
                            players[i].points = 1;
                        }
                        else {
                            countWrong++;}
                    }
                }
            }
        }
        
        for (auto x : players) {
            if (x.socketID == fattestPlayerID) {
                x.points = countWrong;
            }
        }
        update_pos();
        for (auto x : players) {
            if (x.position==race_length) {
                winnerFound=true;
            }
        }
        
    }    

    // Switch to the next player
    currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
    questionCount++;
}

int main() {
    char* hello_message = "Welcome to the RaceToDie"; 
    char* regsucess_message = "Registration Completed Successfully"; 
    char* regfail_message = "Registration Failed, Try again"; 
    char* start_message = "The game begins now"; 
    int opt = true;  
    int addrlen, new_socket , client_socket[10] , activity, i , valread , sd;  
    struct sockaddr_in address;    
    char buffer[100];  //data buffer of 1K      

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < MAX_CLIENTS; i++)  
    {  
        client_socket[i] = 0;  
    }   
    //create a master socket 
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    // int flags = fcntl(master_socket, F_GETFL, 0);
    // fcntl(master_socket, F_SETFL,flags | O_NONBLOCK);
    u_long mode =1;
    int result = ioctlsocket(master_socket, FIONBIO, &mode);
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    //type of socket created 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons(8888);  
         
    //bind the socket to localhost port 8888 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", 8888);  
         
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }     
    //accept the incoming connection 
    addrlen = sizeof(address);   
    while (true) {
        //clear the socket set 
        FD_ZERO(&readfds);    
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
        //add child sockets to set 
        for ( i = 0 ; i < MAX_CLIENTS ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];             
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);    
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }     
        //wait for an activity on one of the sockets , timeout is NULL , //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);    
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
            continue;
        }        
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            } 
            // int flags = fcntl(new_socket, F_GETFL, 0);
            // fcntl(new_socket, F_SETFL,flags | O_NONBLOCK); 
            u_long mode1 =1;
            int result = ioctlsocket(master_socket, FIONBIO, &mode1);
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));         
            //add new socket to array of sockets 
            for (i = 0; i < MAX_CLIENTS; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);                          
                    break;  
                }  
            } 
        }  
        for (i = 0; i < MAX_CLIENTS; i++)  
        {  
            sd = client_socket[i];              
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the incoming message 
                if ((valread = recv( sd ,buffer, sizeof(buffer),0)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address ,(socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                    close(sd);  
                    client_socket[i] = 0;  
                } 
                else 
                {  
                    if(is_valid_nickname(buffer)){
                    //set the string terminating NULL byte on the end of the data read 
                    send(sd , regsucess_message , strlen(regsucess_message) , 0 );  
                    cout << "regsuccess send on " << sd << endl;
                    Player player1(buffer);
                    player1.socketID=sd;
                    players.push_back(player1);
                    } 
                    else {
                    cout << "Reg failed on " << sd << endl;
                    send(sd , regfail_message, strlen(regfail_message) , 0 );
                    }
                }
            
            }          
        }
        // Check if we have enough players to start the game
        if (players.size()>= 2) {
            // announce(start_message);
            break;
        }   
    }
    playSet(players.size(),players, QUESTION_TIME);
    return 0;
}