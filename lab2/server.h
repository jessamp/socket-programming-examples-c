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
#include "clienthandler.h"
#include "clientqueue.h"

using namespace std;

void* handle(void* ptr);

class Server {
public:
    Server(int port, bool debugFlag);
    ~Server();

    void run();
    ClientQueue* get_queue();
    vector<Message>* get_messages();
    sem_t messageLock, numClients, numSpaces;
    
private:
    void initialize_thread_pool();
    void create();
    void close_socket();
    void serve();

    int port_;
    int server_;

    vector<Message>* messages_;
    vector<pthread_t*> threads_;
    ClientQueue* queue_;

    bool debugFlag_;
};
