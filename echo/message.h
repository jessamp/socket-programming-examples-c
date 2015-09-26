#include <string>

using namespace std;

class Message {
public:
    Message();
    ~Message();

    string command;
    string name;
    string subject;
    int length;
    int index;
    string value;
    bool needed;
};