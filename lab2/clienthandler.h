#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <semaphore.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "message.h"

using namespace std;

class ClientHandler {
public:
    ClientHandler(int,bool,sem_t*);
    ~ClientHandler();
    
    bool handle(vector<Message>*);
    int get_client();

private:
    string get_request();
    Message parse_request(string);
    void get_value(Message &);
    bool handle_message(Message, vector<Message>*);
    bool send_response(string);

    int client_;
    int buflen_;
    char* buf_;
    string cache_;

    bool debugFlag_;
    sem_t* messageLock;
};
