#include "clienthandler.h"

ClientHandler::ClientHandler(int client, bool debugFlag, sem_t* msgLock) {
    // setup variables
    client_ = client;
    debugFlag_ = false;//debugFlag;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
    cache_= "";
    messageLock = msgLock;
}

ClientHandler::~ClientHandler() {
    //delete buf_;
}

bool
ClientHandler::handle(vector<Message>* messages) {
    // get a request
    string request = get_request();
    // break if client is done or an error occurred
    if (request.empty()) {
        close(client_);
        return true;
    }
    //parse request
    Message message = parse_request(request);

    //get more characters if needed
    if (message.needed)
        get_value(message);

    // send response
    bool success = handle_message(message, messages);
    // break if an error occurred
    if (not success) {
        close(client_);
        return true;
    }

    if(debugFlag_)
        cout << "End of one request in handle. Success = " << success << endl;
    return false;
}

string
ClientHandler::get_request() {
    string request = cache_;
    // read until we get a newline
    while (request.find("\n") == string::npos) {
        int nread = recv(client_,buf_,1024,0);
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

Message
ClientHandler::parse_request(string request) {
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
ClientHandler::get_value(Message &message) {
    int messageLength = message.length;
    int currentSize = cache_.size();
    // read until we get a newline
    while (currentSize < messageLength) {
        int amountLeft = messageLength-currentSize;
        if(amountLeft > 1024)
            amountLeft = 1024;
        int nread = recv(client_,buf_,amountLeft,0);
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
ClientHandler::handle_message(Message message, vector<Message>* messages) {
    string response = "";
    if(message.command == "put") {
        sem_wait(messageLock);
        if(message.name != "" && message.subject != "" && message.length != -1) {
            if(message.length == message.value.size()) {
                messages->push_back(message);
                response = "OK\n";
            }
            else
                response = "error message not saved correctly\n";
        }
        else
            response = "error invalid message\n";

        sem_post(messageLock);
    }

    if(message.command == "list" && message.name != "") {
        sem_wait(messageLock);
        int index = 0;
        for(int i = 0; i < messages->size(); i++) {
            Message m = (*messages)[i];
            if(m.name == message.name) {
                index++;
                response += to_string(index) + " " + m.subject + '\n';
            }
        }
        response = "list " + to_string(index) + '\n' + response;
        if(debugFlag_)
            cout << "in handle message in list...response: " << response << endl;    
        sem_post(messageLock);
    }

    if(message.command == "get" && message.name != "" && message.index != 0) {
        sem_wait(messageLock);
        int index = 0;
        for(int i = 0; i < messages->size(); i++) {
            Message m = (*messages)[i];
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
        sem_post(messageLock);
    }
    
    if(message.command == "reset") {
        sem_wait(messageLock);
        messages->clear();
        sem_post(messageLock);

        response = "OK\n";
    }
    //message didn't have recognized command
    if(response == "")
        response = "error I don't recognize that command.\n";

    bool success = send_response(response);
    return success;
}

bool
ClientHandler::send_response(string response) {
    // prepare to send response
    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client_, ptr, nleft, 0)) < 0) {
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

int
ClientHandler::get_client() {
    return client_;
}
