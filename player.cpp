class Player {
    int id;
    string nickname;
    int points;
    int position;
    int consecutive_wrong_answers;
    bool disqualified;
    
    
    chrono::steady_clock::time_point last_answer_time;
};
