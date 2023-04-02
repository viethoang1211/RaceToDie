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
queue<int> turn_order;
mutex mtx;
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
    // check trong map xem co ai trung ten khong
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
	switch(x):
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


// 3. each turn 
void playSet( int playerCount, vector<Player>& players, int questionTimeLimit) {
    int currentPlayerIndex = 0;
    int questionCount = 0;
    bool winnerFound = false;
    vector<Message> messages;
    race_length = getRandomInt(MIN_LENGTH, MAX_LENGTH);


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
            // not yet implemented
            players[i].socket.send(question);
        }

        // Receive answers from all players
        vector<string> answers(playerCount);
        for (int i = 0; i < playerCount; i++) {
            bool receivedAnswer = players[i].socket.receive(answers[i], questionTimeLimit);
            if (!receivedAnswer) {
                // 3c_i. out of time 
                players[i].score--;
                players[i].socket.send("Out of time! You lost 1 point.");
            }
        }
        //vector<string> answers(playerCount);
        //for (int i = 0; i < playerCount; i++) {
        //    bool receivedAnswer = players[i].socket.receive(answers[i], questionTimeLimit);
        //    if (!receivedAnswer) {
        //    	// 3c_i. out of time 
        //        players[i].score--;
        //        players[i].socket.send("Out of time! You lost 1 point.");
        //    }
        //}


        // Calculate the correct answer and determine the fastest player
        int correctAnswer = calculateAnswer(a, b, op);
        int fastestPlayerIndex = -1;
        int pointForHighest = 0;
    
        for (int i = 0; i < playerCount; i++) {
            if (answers[i] == to_string(correctAnswer)) {
                int points = calculatePoints(players, i, playerCount, maxPoints);
                players[i].score += points;
                players[i].position += points;
                if (points > maxPoints) {
                    maxPoints = points;
                    fastestPlayerIndex = i;
                }
                
                // set lai chuoi thua
                players[i].wrong_answers_count = 0;

            }
            else {
                players[i].points--;
                players[i].wrong_answers_count++;

                players[i].socket.send("Wrong answer! You lost 1 point.");
                if (players[i].wrongAnswers == 3) {
                    playerCount--;
                    players.erase(players.begin() + i);
                    i--;
                    for (int j = 0; j < playerCount; j++) {
                        players[j].socket.send("One player disqualified! Remaining players: " + to_string(playerCount));
                    }
                }
            }
        }

        // ------------ Tinh diem cho moi nguoi -------------------

        vector<Message> message(messages);

        // xoa message nguoi sai
        for (auto x = message.begin(); x != message.end(); ) {
            if (x->text != correctAnswer) {
                message.erase(x);
            }
            else {
                ++x;
            }
        }

        sort(message.begin(), message.end());
        // kiem tra xem ai nhanh nhat  ~ message[message.size()-1] 

        // nguoi nhanh nhat co ton tai khong
        bool fattestExist = false;

        if (message.size() > 0) {
            fattestExist = true;
        }


        // ten cua nguoi nhanh nhat
        string fattestPlayername = "";
        if (fattestExist) {
            fattestPlayername = message.end()->name;
        }
        // tinh diem cho tung nguoi 
        for (auto x : players) {
            x.points = -1;
        }
        int countWrong = 0;
        for (int i = 0; i < players.size(); i++) {
            if (players[i].nickname != fattestPlayer) {
                for (auto j : messages) {
                    if (j.clientID == players[i].socketID) {
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
        for (int i = 0; i < players.length(); i++) {

        }


        // --------------- end update --------------------

        // Switch to the next player
        currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
        questionCount++;
    }
}
void announce(char* message){
     for (int i = 0; i < players.size(); i++) {
        int bytes_sent = send(players[i].socketID,message,strlen(message),0);
        if (bytes_sent==-1){
        cout << "Send error at player:" << i << endl;
        return ;
    }
    }

}


int main() {
    // // Seed the random number generator
    // srand(time(NULL));
    
    // // Create the socket
    // int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (sockfd < 0) {
    //     cerr << "Error: could not create socket." << endl;
    //     return 1;
    // }
    
    // // Set up the server address
    // sockaddr_in server_addr;
    // memset(&server_addr, 0, sizeof(server_addr));
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // server_addr.sin_port = htons(8888); // Choose a port number
    
    // // Bind the socket to the server address
    // if (bind(sockfd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
    //     cerr << "Error: could not bind socket to server address." << endl;
    //     return 1;
    // }
    
    // cout << "Server is running on port 8888..." << endl;
    int opt = true;  
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;  
    int max_sd;  
    struct sockaddr_in address;    
    char buffer[1025];  //data buffer of 1K      

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++)  
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
    puts("Waiting for connections ...");  
    
    while (true) {
        // // Wait for a client to connect
        // sockaddr_in client_addr;
        // memset(&client_addr, 0, sizeof(client_addr));
        // socklen_t client_len = sizeof(client_addr);
        
        // // Receive a request from a client
        // string request = receive_string(sockfd, client_addr);
        //clear the socket set 
        FD_ZERO(&readfds);  
    
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
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
     
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
       
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
             
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
           
            //send new connection greeting message 
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
            {  
                perror("send");  
            }  
            puts("Welcome message sent successfully");  
                 
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++)  
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
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, 1024)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd );  
                    client_socket[i] = 0;  
                } 
            }
            else 
                {  
                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    buffer[valread] = '\0'; 
                    int i;
                    for (i = 0; i < strlen(buffer); i++) {
                        buffer[i] = toupper(buffer[i]);
                    } 
                    send(sd , buffer , strlen(buffer) , 0 );  
                }  
        }
        //
        
        // Check the type of request
        if (request == "REGISTER") {
            // Check if the nickname is valid and not already taken
            string nickname = receive_string(sockfd, client_addr);
            if (is_valid_nickname(nickname) && find_if(players.begin(), players.end(), [nickname](Player p) { return p.name == nickname; }) == players.end()) {
                // Add the player to the list
                Player player;
                player.name = nickname;
                player.score = 0;
                player.position = 1;
                player.wrong_answers = 0;
                players.push_back(player);
                num_players++;
                
                // Send a success message to the client
                announce("Registration Completed Successfully", sockfd, client_addr);
                
                
                // Check if we have enough players to start the game
                if (num_players >= 2 && num_players <= MAX_CLIENTS) {
                    playSet();
            	}
            }
        }
    }
}