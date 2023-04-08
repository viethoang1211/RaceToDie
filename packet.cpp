#include <cstring>
#include <string>
using namespace std;
class Packet {
public:
    // the type of message, defined at server
    int type;
    // message to other side, maybe a annoucement or a name of player
    char Context[50]="";
    // point of this round 
    int point;
    // position of the player
    int position;

    Packet(int t, char* context, int p1, int p2) {
        type=t;
        point=p1;
        position=p2;
        strcpy(Context,context);
    }
    Packet(){
        type=0;
        point=0;
        position=0;
    }
};