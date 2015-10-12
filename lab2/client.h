#pragma once

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "message.h"

using namespace std;

class Client {
public:
    Client(string host, int port);
    ~Client();

    void run();

private:
    virtual void create();
    virtual void close_socket();
    void echo();
    bool send_request(string);
    string get_response();
    void get_value(int, Message &);
    Message parse_response(string);
    bool handle_response(Message);

    string host_;
    int port_;
    int server_;
    int buflen_;
    char* buf_;

    string cache_;
};
