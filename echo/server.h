#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "message.h"

using namespace std;

class Server {
public:
    Server(int port, bool debugFlag);
    ~Server();

    void run();
    
private:
    void create();
    void close_socket();
    void serve();
    void handle(int);
    string get_request(int);
    bool send_response(int, string);
    Message parse_request(string);
    void get_value(int, Message &);
    bool handle_message(int, Message);

    int port_;
    int server_;
    int buflen_;
    char* buf_;

    string cache_;
    vector<Message> messages_;

    bool debugFlag_;
};
