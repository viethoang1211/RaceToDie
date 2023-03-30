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

// Function prototypes
//bool is_valid_nickname(string nickname);
//void announce(string message, int sockfd, sockaddr_in client);
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
            if(players[i].position<1)
                players[i].position=1;
            else if(players[i].position>race_length) 
                players[i].position=race_length;
        }
}


// 3. each turn 
void playSet(int raceLength, int playerCount, vector<Player>& players, int questionTimeLimit) {
    int currentPlayerIndex = 0;
    int questionCount = 0;
    bool winnerFound = false;

    // Loop until a winner is found
    while (!winnerFound) {
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

        // Calculate the correct answer and determine the fastest player
        int correctAnswer = calculateAnswer(a, b, op);
        int fastestPlayerIndex = -1;
        int maxPoints = -1;
        for (int i = 0; i < playerCount; i++) {
            if (answers[i] == to_string(correctAnswer)) {
                int points = calculatePoints(players, i, playerCount, maxPoints);
                players[i].score += points;
                players[i].position += points;
                if (points > maxPoints) {
                    maxPoints = points;
                    fastestPlayerIndex = i;
                }
            } else {
                players[i].score--;
                players[i].wrongAnswers++;
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

        // Update positions and check for winner
        sort(players.begin(), players.end(), [](Player& a, Player& b) {
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
        }

        // Switch to the next player
        currentPlayerIndex = (currentPlayerIndex + 1) % playerCount;
        questionCount++;
    }
}



int main() {
    // Seed the random number generator
    srand(time(NULL));
    
    // Create the socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        cerr << "Error: could not create socket." << endl;
        return 1;
    }
    
    // Set up the server address
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8888); // Choose a port number
    
    // Bind the socket to the server address
    if (bind(sockfd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error: could not bind socket to server address." << endl;
        return 1;
    }
    
    cout << "Server is running on port 8888..." << endl;
    
    // Initialize the list of players
    vector<Player> players;
    int num_players = 0;
    int race_length = 0;
    
    while (true) {
        // Wait for a client to connect
        sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);
        
        // Receive a request from a client
        string request = receive_string(sockfd, client_addr);
        
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
                    // Generate the race
            	}
            }
        }
    }
}