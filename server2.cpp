#include <iostream>
#include <cstdlib>
// #include <cstring>
// #include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <algorithm>

// For sockets
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
// .h file
#include "player.cpp"
#include "message.cpp"
using namespace std;

// Constants
const int MAX_CLIENTS = 10; // Maximum number of clients
const int MIN_CLIENTS = 2;
const int MIN_LENGTH = 3; // Minimum length of the race
const int MAX_LENGTH = 26; // Maximum length of the race
const int QUESTION_TIME = 10; // Time in seconds to answer each question
const int MAX_NICKNAME_LENGTH=10; // Lenght of client name


// Global variables
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

// Function prototypes
//bool is_valid_nickname(string nickname);

//void send_int(int n, int sockfd, sockaddr_in client);
//int receive_int(int sockfd, sockaddr_in client);
//void send_string(string s, int sockfd, sockaddr_in client);
//string receive_string(int sockfd, sockaddr_in client);
//int generate_question();
//int calculate_points(int correct_answers, vector<Player>& players, int fastest_player_index);

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
    return rand() % (max - min + 1) + min;
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
}

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
    }
}

//3c iv update position
void update_pos(){
    for (int i = 0; i < players.size(); i++) {
            players[i].position += points;
            if (players[i].position < 1)
                players[i].position = 1;
            else if (players[i].position > race_length)
                players[i].position = race_length;
        }
}

void announce(string message) {
    char* tem_message = new char[message.length() + 1];
    strcpy(tem_message, message.c_str());
    for (int i = 0; i < players.size(); i++) {
        int bytes_sent = send(players[i].socketID, tem_message, strlen(tem_message), 0);
        if (bytes_sent == -1) {
            cout << "Send error at player:" << i << endl;
            return;
        }
    }
    delete[] tem_message;
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
    
    // Loop until a winner is found
    while (!winnerFound) {
        messages.clear();

        // 3a.  Get the two random integers and operator for the question
        int a = getRandomInt(-10000, 10000);
        int b = getRandomInt(-10000, 10000);
        char op = getRandomOperator();

        // Make question and send the question to all players
        string question = to_string(a) + " " + string(1, op) + " " + to_string(b);
        for (int i = 0; i < playerCount; i++) {
            announce(question);
        }
        auto start = std::chrono::high_resolution_clock::now(); // get the start time
        while (true) {
        int activity = select( max_sd + 1 , &readfds , NULL , NULL , &timeout);    
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }        
        for (auto i = players.begin(); i != players.end();)  
        {  
            int sd = i->socketID;           
            if (FD_ISSET(sd, &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message
                char* buffer;  //data buffer of 1K   
                if ((valread = read(sd,buffer,1024)) == 0)  
                {  
                    // getpeername(sd , (struct sockaddr*)&address ,(socklen_t*)&addrlen);  
                    // printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                    //Close the socket and mark as 0 in list for reuse 
                    close(sd);  
                    players.erase(i);
                } 
                else 
                {  
                    Message msg(i->socketID, std::chrono::system_clock::now(), buffer);
                    messages.push_back(msg);
                } 
                delete buffer;
            }
                
        }
        auto now = std::chrono::high_resolution_clock::now(); // get the current time
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count(); // get the elapsed time in milliseconds
        if (duration >= 5000) { // if the elapsed time is greater than or equal to 5000 milliseconds (5 seconds)
            break; // exit the loop
        }
    }        
    }    

        // Calculate the correct answer and determine the fastest player
        int correctAnswer = calculateAnswer(a, b, op);
        int fastestPlayerIndex = -1;
        int pointForHighest = 0;
    
        //for (int i = 0; i < playerCount; i++) {
        //    if (answers[i] == to_string(correctAnswer)) {
        //        int points = calculatePoints(players, i, playerCount, maxPoints);
        //        players[i].score += points;
        //        players[i].position += points;
        //        if (points > maxPoints) {
        //            maxPoints = points;
        //            fastestPlayerIndex = i;
        //        }
        //        
        //        // set lai chuoi thua
        //        players[i].wrong_answers_count = 0;

        //    }
        //    else {
        //        players[i].points--;
        //        players[i].wrong_answers_count++;

        //        players[i].socket.send("Wrong answer! You lost 1 point.");
        //        if (players[i].wrongAnswers == 3) {
        //            playerCount--;
        //            players.erase(players.begin() + i);
        //            i--;
        //            for (int j = 0; j < playerCount; j++) {
        //                players[j].socket.send("One player disqualified! Remaining players: " + to_string(playerCount));
        //            }
        //        }
        //    }
        //}

        // ------------ Tinh diem cho moi nguoi -------------------

        // cop
        vector<Message> message_copy(messages);

        // xoa message nguoi sai
        for (auto x = message_copy.begin(); x != message_copy.end(); ) {
            if (x->text != correctAnswer) {
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
        // ten cua nguoi nhanh nhat
        string fattestPlayername = "";
        if (fattestExist) {
            fattestPlayername = message_copy.end()->name;
        }

        // tinh diem cho tung nguoi 
        for (auto x : players) {
            x.points = -1;
        }
        int countWrong = 0;
        for (int i = 0; i < players.size(); i++) {
            if (players[i].nickname != fattestPlayername) {
                for (auto j : messages) {
                    if (j.clientId == players[i].socketID) {
                        if (to_string(correctAnswer) == j.text) {
                            players[i].points = 1;
                        }
                        else {
                            countWrong++;
                        }
                    }
                }
            }
        }
        
        for (auto x : players) {
            if (x.nickname == fattestPlayername) {
                x.points = countWrong;
            }
        }
        
        // ------------- end check -----------------
        // -------------Update positions and check for winner---------------
       
        /* sort(players.begin(), players.end(), [](Player& a, Player& b) {
            return a.position > b.position;
        });
        for (int i = 0; i < playerCount; i++) {
            if (players[i].position >= raceLength) {
                winnerFound = true;
                for (int j = 0; j < playerCount; j++) {
                    if (j == i) {
                        players[j].socket.send("You won the game!");
                    } else {
                        players[j].socket.send("Player " + players[i].nickname + " won the game!");
                    }
                }
                break;
            } else {
                players[i].socket.send("Current position: " + to_string(players[i].position));
            }
        }*/

        update_pos();
        
        
        
        /*for (int i = 0; i < players.length(); i++) {

        }*/


        // --------------- end update --------------------

        // Switch to the next player
        currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
        questionCount++;
    }



int main() {
    char* hello_message = "Welcome to the RaceToDie \r\n"; 
    char* regsucess_message = "Registration Completed Successfully \r\n"; 
    char* regfail_message = "Registration Failed, Try again \r\n"; 
    char* start_message = "The game begins now \r\n"; 
    int opt = true;  
    int addrlen, new_socket , client_socket[10] , activity, i , valread , sd;  
    struct sockaddr_in address;    
    char buffer[1025];  //data buffer of 1K      

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < MAX_CLIENTS; i++)  
    {  
        client_socket[i] = 0;  
    }   
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
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
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));     
            //send new connection greeting message 
            // if( send(new_socket, hello_message, strlen(hello_message), 0) != strlen(hello_message) )  
            // {  
            //     perror("send");  
            // }            
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
                if ((valread = read( sd , buffer, 1024)) == 0)  
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
                    // string s= buffer;
                    Player player1(buffer);
                    player1.socketID=sd;
                    players.push_back(player1);
                    } 
                    else 
                    send(sd , regfail_message, strlen(regfail_message) , 0 );
                }
            
            }    
            
        }
        // Check if we have enough players to start the game
        if (players.size()>= 3) {
            announce(start_message);
            playSet(players.size(),players, QUESTION_TIME);
            break;
        }
        
    }
    return 0;
}