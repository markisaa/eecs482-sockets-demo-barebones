
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

using std::cerr;
using std::endl;
using std::string;
using std::to_string;
using std::memset;
using std::fill_n;
using std::runtime_error;
using std::thread;


const int kBufferSize = 20;
const int kBacklogSize = 15;

void clientStart(int fd);
void serverStart(int myFd);
int createServerSocket();
int createClientSocket();
int getSocket(addrinfo& info);
addrinfo* populateAddrInfo();

int main() {

  int serverFd = createServerSocket();
  int clientFd = createClientSocket();

  thread server(serverStart, serverFd);
  server.detach();
  clientStart(clientFd);

  return 0;
}

void serverStart(int myFd) {
  //Wait for someone to connect:
  sockaddr_storage theirAddr;
  socklen_t connAddr = sizeof(sockaddr_storage);
  int acceptedFd = ::accept(myFd, reinterpret_cast<sockaddr*>(&theirAddr), &connAddr);

  //Wait for them to send us a message:
  char buf[kBufferSize];
  fill_n(buf, kBufferSize, '\0');
  ::recv(acceptedFd, reinterpret_cast<void*>(&buf), kBufferSize, 0);

  cerr << "Server received message: " << buf << endl;

  //Send response:
  string reply("Hello, internet.");
  ::send(acceptedFd, reply.data(), reply.size(), MSG_NOSIGNAL);

  //Clean up:
  ::close(acceptedFd);
  ::close(myFd);
}

void clientStart(int fd) {

  //Say hello:
  string message("Hello server!");
  ::send(fd, message.data(), message.size(), MSG_NOSIGNAL);

  //Get a response:
  char buf[kBufferSize];
  fill_n(buf, kBufferSize, '\0');
  ::recv(fd, reinterpret_cast<void*>(&buf), kBufferSize, 0);

  cerr << "Client received response: " << buf << endl;

  //Clean up:
  ::close(fd);
}


int createServerSocket() {
  addrinfo* info = populateAddrInfo();
  int fd = getSocket(*info);

  //Allow us to reuse addresses/ports from run to run:
  int yes = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  //Tell the socket what port we are listening on:
  ::bind(fd, info->ai_addr, info->ai_addrlen);
  ::freeaddrinfo(info);

  //Tell the outside world that we're ready to be connected to:
  ::listen(fd, kBacklogSize);

  return fd;
}

int createClientSocket() {
  //Tell it who we want to talk to:
  addrinfo* info = populateAddrInfo();
  int fd = getSocket(*info);

  //Actually connect to the server:
  ::connect(fd, info->ai_addr, info->ai_addrlen);
  ::freeaddrinfo(info);

  return fd;
}

int getSocket(addrinfo& info) {
  return ::socket(info.ai_family, info.ai_socktype, info.ai_protocol);
}

addrinfo* populateAddrInfo() {
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; //Use IPv4
  hints.ai_socktype = SOCK_STREAM; //Use TCP
  hints.ai_flags = AI_PASSIVE;
  const char* const host = "localhost";
  const char* const service = "20345"; //Use port 20345, set to 0 to let the OS pick

  addrinfo* info;
  int rc = ::getaddrinfo(host, service, &hints, &info);
  if(rc != 0) {
    throw runtime_error{string{"getaddrinfo error: "} + ::gai_strerror(rc)};
  }

  return info;

  //Later call: '::freeaddrinfo(info);' to clean up the memory. Don't use delete!
}
