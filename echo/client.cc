#include "client.h"

Client::Client(string host, int port) {
    // setup variables
    host_ = host;
    port_ = port;
    buflen_ = 1024;
    buf_ = new char[buflen_+1];
}

Client::~Client() {
}

void Client::run() {
    // connect to the server and run echo program
    create();
    echo();
}

void
Client::create() {
    struct sockaddr_in server_addr;

    // use DNS to get IP address
    struct hostent *hostEntry;
    hostEntry = gethostbyname(host_.c_str());
    if (!hostEntry) {
        cout << "No such host name: " << host_ << endl;
        exit(-1);
    }

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

    // create socket
    server_ = socket(PF_INET,SOCK_STREAM,0);
    if (!server_) {
        perror("socket");
        exit(-1);
    }

    // connect to server
    if (connect(server_,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-1);
    }
}

void
Client::close_socket() {
    close(server_);
}

void
Client::echo() {
    string line;
    
    // loop to handle user interface

    cout << "% ";
    while (getline(cin,line)) {
        // append a newline
        line += "\n";

        stringstream request(line);
        string command;
        request >> command;

        if (command == "send") {
            string name, subject, message;
            request >> name >> subject;

            if(name != "" && subject != "") {
                cout << "- Type your message. End with a blank line -" << endl;
                string currentLine;
                while (getline(cin, currentLine)) {
                    if(currentLine != "")
                        message += currentLine + '\n';
                    else
                        break;
                }
                //remove newline at the end of message
                if (message != "")
                    message.pop_back();
                else
                    message = "";
                
                stringstream reformat_request;
                reformat_request << "put " << name << " " << subject << " " << message.length() << "\n" << message;
                line = reformat_request.str();
            }
        }
        else if (command == "read") {
            line.erase(line.begin(), line.begin()+4);
            line = "get" + line;
        }
        else if (command == "quit")
            break;

        // send request
        //cout << "Client: before request: " << line;
        bool success = send_request(line);
        // break if an error occurred
        if (not success)
            break;
        // get a response
        string response = get_response();
        //parse request
        Message message = parse_response(response);
        if (message.needed)
            get_value(server_,message);
        //do something
        success = handle_response(message);
        // break if an error occurred
        if (not success)
            break;
        cout << "% ";
    }
    close_socket();
}

bool
Client::send_request(string request) {
    // prepare to send request
    const char* ptr = request.c_str();
    int nleft = request.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(server_, ptr, nleft, 0)) < 0) {
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
        nleft -= nwritten;
        ptr += nwritten;
    }
    return true;
}

Message
Client::parse_response(string response) {
    Message message;
    stringstream responseParams(response);
    string command;
    responseParams >> command;
    message.command = command;

    if(command == "message") {
        int length;
        string subject, value;
        responseParams >> subject >> length >> value;
        message.subject = subject;
        message.length = length;
        message.value = value;
        
        if(length > 0)
            message.needed = true;
        else
            message.needed = false;
    }
    
    else if(command == "list") {
        int index;
        string list;
        responseParams >> index >> list;
        message.index = index;
        message.value = list;

        if(index > 0)
            message.needed = true;
        else
            message.needed = false;
    }

    else if(command == "error") {
        message.value = response.erase(0, command.size()+1);
    }
    else
        message.value = response;
    return message;
}

void
Client::get_value(int server, Message &message) {
    int messageLength = message.length;
    int currentSize = cache_.size();
    // read until we get a newline
    while (currentSize < messageLength) {
        int amountLeft = messageLength-currentSize;
        if(amountLeft > 1024)
            amountLeft = 1024;
        //cout << "in get response loop before recieve: " << buf_ << endl;
        int nread = recv(server,buf_,amountLeft,0);
        //cout << "in get response loop after: " << buf_ << endl;
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
Client::handle_response(Message message) {
    if(message.command == "message")
        cout << message.subject << endl << message.value << endl;
    else if(message.command == "list")
        cout << message.value;
    else if(message.command == "error") {
        if(message.value[0] == 'I')
            cout << message.value;
        else
            cout << "Server error: " << message.value;
    }
    return true;
}

string
Client::get_response() {
    string response = "";
    // read until we get a newline
    while (response.find("\n") == string::npos) {
        int nread = recv(server_,buf_,1024,0);
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
        response.append(buf_,nread);
    }
    // a better client would cut off anything after the newline and
    // save it in a cache
    size_t found = response.find("\n");
    cache_ = response.substr(found+1);
    return response;
}
