#include "server.h"

Server::Server(int port, bool debugFlag) {
    // setup variables
    port_ = port;
    debugFlag_ = debugFlag;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    cache_= "";
}

Server::~Server() {
    delete buf_;
}

void
Server::run() {
    // create and run the server
    create();
    serve();
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

      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {
        handle(client);
    }
    close_socket();
}

Message
Server::parse_request(string request) {
    Message message;  
    stringstream requestParams(request);
    string command, name;
    requestParams >> command >> name;
    message.command = command;
    message.name = name;

    if(command == "put") {
        string length;
        string subject, value;
        requestParams >> subject >> length >> value;
        message.subject = subject;
        message.value = value;
        if(length != "")
            message.length = atoi(length.c_str());
        else
            message.length = -1;
        
        if(message.length > 0)
            message.needed = true;
        else
            message.needed = false;
    }
    
    if(command == "get") {
        string index;
        requestParams >> index;
        message.index = atoi(index.c_str());
    }
    return message;
}

void
Server::get_value(int client, Message &message) {
    int messageLength = message.length;
    int currentSize = cache_.size();
    // read until we get a newline
    while (currentSize < messageLength) {
        int amountLeft = messageLength-currentSize;
        if(amountLeft > 1024)
            amountLeft = 1024;
        int nread = recv(client,buf_,amountLeft,0);
        if(debugFlag_)
            cout << "in get request loop after recieve: " << buf_ << endl;
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return;
        } else if (nread == 0) {
            // the socket is closed
            return;
        }
        // be sure to use append in case we have binary data
        cache_.append(buf_,nread);
        currentSize = cache_.size();
    }
    message.value = cache_;
    cache_ = "";
}

bool
Server::handle_message(int client, Message message) {
    string response = "";
    if(message.command == "put") {
        if(message.name != "" && message.subject != "" && message.length != -1) {
            if(message.length == message.value.size()) {
                messages_.push_back(message);
                response = "OK\n";
            }
            else
                response = "error message not saved correctly\n";
        }
        else
            response = "error invalid message\n";
    }

    if(message.command == "list" && message.name != "") {
        int index = 0;
        for(int i = 0; i < messages_.size(); i++) {
            Message m = messages_[i];
            if(m.name == message.name) {
                index++;
                response += to_string(index) + " " + m.subject + '\n';
            }
        }
        response = "list " + to_string(index) + '\n' + response;
    }

    if(message.command == "get" && message.name != "" && message.index != 0) {
        int index = 0;
        for(int i = 0; i < messages_.size(); i++) {
            Message m = messages_[i];
            if(m.name == message.name) {
                index++;
                if(index == message.index) {
                    response += "message " + m.subject + " " + to_string(m.length) + '\n' + m.value;
                    break;
                }
            }
        }
        if(response == "")
            response = "error no such message for that user\n";
    }
    
    if(message.command == "reset") {
        messages_.clear();
        response = "OK\n";
    }
    //message didn't have recognized command
    if(response == "")
        response = "error I don't recognize that command.\n";

    bool success = send_response(client, response);
    return success;
}

void
Server::handle(int client) {
    // loop to handle all requests
    while (1) {
        // get a request
        string request = get_request(client);
        // break if client is done or an error occurred
        if (request.empty())
            break;
        //parse request
        Message message = parse_request(request);

        //get more characters if needed
        if (message.needed)
            get_value(client, message);

        // send response
        bool success = handle_message(client,message);
        // break if an error occurred
        if (not success)
            break;
    }
    close(client);
}

string
Server::get_request(int client) {
    string request = cache_;
    // read until we get a newline
    while (request.find("\n") == string::npos) {
        int nread = recv(client,buf_,1024,0);
        if(debugFlag_)
            cout << "in get request loop after recieve: " << buf_ << endl;
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } else if (nread == 0) {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        request.append(buf_,nread);
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    size_t found = request.find("\n");
    cache_ = request.substr(found+1);
    return request;
}

bool
Server::send_response(int client, string response) {
    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        if(debugFlag_)
            cout << "what we're sending: " << ptr << endl;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return true;
}
