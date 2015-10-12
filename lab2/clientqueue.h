#pragma once

#include <string>
#include <queue>
#include <semaphore.h>
#include "clienthandler.h"

using namespace std;

class ClientQueue {
public:
    ClientQueue();
    ~ClientQueue();

    ClientHandler pop();
    void push(ClientHandler);
    bool is_empty();
    int size();
    ClientHandler front();

private:
    queue<ClientHandler> clients_;
    sem_t queueLock;
};