class Player {
public:
    string nickname;
    int position;
    int points;
    int wrong_answers_count;
    int socketID;

    Player(string n) : nickname(n), position(1), points(0), wrong_answers_count(0) {}
};