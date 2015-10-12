#include "clientqueue.h"

ClientQueue::ClientQueue() {
	sem_init(&queueLock, 0, 1);
}

ClientQueue::~ClientQueue() {}

ClientHandler
ClientQueue::pop() {
	sem_wait(&queueLock);
	ClientHandler ch = clients_.front();
	//cout << "before pop, size = " << clients_.size() << endl;
	clients_.pop();
	//cout << "popped" << endl;
	sem_post(&queueLock);
	return ch;
}

void
ClientQueue::push(ClientHandler ch) {
	sem_wait(&queueLock);
	clients_.push(ch);
	//cout << "pushed" << endl;
	sem_post(&queueLock);
}

bool
ClientQueue::is_empty() {
	//cout << "is_empty " << clients_.empty() << endl;
	return clients_.empty();
}

int
ClientQueue::size() {
	return clients_.size();
}

ClientHandler
ClientQueue::front() {
	return clients_.front();
}