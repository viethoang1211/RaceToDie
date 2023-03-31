#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
using namespace std;
// A struct to hold a message, with the sending client's identifier and the time of sending
class Message {
public:
    int clientId;
    chrono::system_clock::time_point timestamp;
    string text;

    // A comparison operator to allow sorting messages based on their timestamps
    bool operator<(const Message& other) const {
        return timestamp < other.timestamp;
    }
    Message(int id, chrono::system_clock::time_point t, string message) {
        clientId = id;
        timestamp = t;
        text = message;
    }
    // example : Message msg(i, std::chrono::system_clock::now(), text);
    // messages.push_back(msg);
    // sort(messages.begin(), messages.end());

};