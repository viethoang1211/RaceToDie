class Player {
public:
    
    string nickname;
    // vi tri hien tai
    int position;
    // point of this round 
    int points;
    // 3 lan lien tiep la tach
    
    // current answer
    int currentAnswer;
    int wrong_answers_count;
    // bang voi clientID cua message
    int socketID;

    Player(string n) : nickname(n), position(1), points(0), wrong_answers_count(0) {}
};