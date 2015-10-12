#include "server.h"

void* handle(void* ptr) {
    Server* server;
    server = (Server*) ptr;
    ClientQueue* queue = server->get_queue();

    while(1) {
        //cout << "before wait on numClients" << endl;
        sem_wait(&server->numClients);
        //cout << "after wait on numClients" << endl;
        ClientHandler handler = queue->pop();
        //cout << "after popped, before post numSpaces" << endl;
        sem_post(&server->numSpaces);
        //cout << "after post numSpaces" << endl;

        //cout << "after pop in handle" << endl;
        bool isClosed = handler.handle(server->get_messages());
        //cout << "after handle called on handler, isCLosed: " << isClosed << endl;
        
        if(!isClosed) {
            //cout << "before wait numSpaces" << endl;
            sem_wait(&server->numSpaces);
            //cout << "after wait numSpaces, before push" << endl;
            queue->push(handler);
            //cout << "after push, before post numClients" << endl;
            sem_post(&server->numClients);
            //cout << "pushed" << endl;
        }
    }
    //delete server;
    
    pthread_exit(0);
}

Server::Server(int port, bool debugFlag) {
    // setup variables
    port_ = port;
    debugFlag_ = debugFlag;

    sem_init(&messageLock, 0, 1);
    sem_init(&numClients, 0, 0);
    sem_init(&numSpaces, 0, 100);
    
    queue_ = new ClientQueue();
    messages_ = new vector<Message>();
}

Server::~Server() {
    delete queue_;
}

void
Server::run() {
    // create and run the server
    create();
    serve();
}

void
Server::initialize_thread_pool() {
    for (int i=0; i<10; i++) {
        pthread_t* thread = new pthread_t;
        pthread_create(thread, NULL, &handle, (void *) this);
        threads_.push_back(thread);
    }
}

void
Server::create() {
    struct sockaddr_in server_addr;

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // create socket
    server_ = socket(PF_INET,SOCK_STREAM,0);
    if (!server_) {
        perror("socket");
        exit(-1);
    }

    // set socket to immediately reuse port when the application closes
    int reuse = 1;
    if (setsockopt(server_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(-1);
    }

    // call bind to associate the socket with our local address and
    // port
    if (bind(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("bind");
        exit(-1);
    }

    // convert the socket to listen for incoming connections
    if (listen(server_,SOMAXCONN) < 0) {
        perror("listen");
        exit(-1);
    }
}

void
Server::close_socket() {
    close(server_);
}

void
Server::serve() {
    // setup client
    int client;
    struct sockaddr_in client_addr;
    socklen_t clientlen = sizeof(client_addr);

    initialize_thread_pool();
      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {
        //cout << "before push" << endl;
        sem_wait(&numSpaces);
        queue_->push(ClientHandler(client, debugFlag_, &messageLock));
        sem_post(&numClients);
    }

    // wait for threads to terminate.
    for (int i=0; i<10; i++) {
        pthread_join(*threads_[i], NULL);
        delete threads_[i];
    }

    close_socket();
}

ClientQueue*
Server::get_queue() {
    return queue_;
}

vector<Message>*
Server::get_messages() {
    return messages_;
}