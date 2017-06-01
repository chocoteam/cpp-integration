#include "connector.hh"

#include <iostream>
#include <ctime>
#include <sstream>
#include <cstdint>
#include <vector>
#include <cstring>

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#else

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#endif

using std::vector;

using cpprofiler::NodeStatus;

namespace Profiling {

Node::Node(int sid, int pid, int alt, int kids, NodeStatus status, Connector& c)
    : _c(c), sid_(sid), pid_(pid), alt_(alt), kids_(kids), status_(status) {}

void Node::send() { this->_c.sendNode(*this); }

Connector::Connector(unsigned int port, unsigned int wid) : port(port), _connected(false) {}

// From http://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall
static int sendall(int s, const char* buf, int* len) {
  int total = 0;         // how many bytes we've sent
  int bytesleft = *len;  // how many we have left to send
  int n;

  while (total < *len) {
    n = send(s, buf + total, bytesleft, 0);
    if (n == -1) {
      break;
    }
    total += n;
    bytesleft -= n;
  }

  *len = total;  // return number actually sent here

  return n == -1 ? -1 : 0;  // return -1 on failure, 0 on success
}

void Connector::sendOverSocket() {
  if (!_connected) return;

  vector<char> buf = marshalling.serialize();

  sendRawMsg(buf);
}

void Connector::sendRawMsg(const vector<char>& buf) {
  uint32_t bufSize = buf.size();
  int bufSizeLen = sizeof(uint32_t);
  sendall(sockfd, reinterpret_cast<char*>(&bufSize), &bufSizeLen);
  int bufSizeInt = bufSize;
  sendall(sockfd, reinterpret_cast<const char*>(buf.data()), &bufSizeInt);
}


Node Connector::createNode(int sid, int pid, int alt, int kids,
                           NodeStatus status) {
  return Node(sid, pid, alt, kids, status, *this);
}

void Connector::sendNode(const Profiling::Node& node) {

  if (!_connected) return;

  auto& msg = marshalling.makeNode(node.sid(), node.pid(), node.alt(), node.kids(), node.status());

  if (node.label().valid()) msg.set_label(node.label().value());

  if (node.nogood().valid()) msg.set_nogood(node.nogood().value());

  if (node.info().valid()) msg.set_info(node.info().value());

  if (node.solution().valid()) msg.set_solution(node.solution().value());

  if (node.tid().valid()) msg.set_tid(node.tid().value());

  if (node.rid().valid()) msg.set_rid(node.rid().value());

  sendOverSocket();
}

void Connector::restart(const std::string& file_path, int rid,
                        const std::string& variableList, int ex_id) {
  /// extract fzn file name
  std::string label(file_path);
  {
    int pos = label.find_last_of('/');
    if (pos > 0) {
      label = label.substr(pos + 1, label.length() - pos - 1);
    }
  }

  std::string info{""};
  {
    std::stringstream ss;
    ss << "{";
    if (ex_id != -1) {
      ss << "\"execution_id\": " << ex_id;
    }
    if (variableList.size() > 0) {
      if (ex_id != -1) ss << ", ";
      ss << "\"variable_list\": \"" << variableList << "\"";
    }
    ss << "}";
    info = ss.str();
  }

  marshalling.makeStart(rid, label, info);
  sendOverSocket();
}

void Connector::connect() {
  struct addrinfo hints, *servinfo, *p;
  int rv;

#ifdef WIN32
  // Initialise Winsock.
  WSADATA wsaData;
  int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (startupResult != 0) {
    printf("WSAStartup failed with error: %d\n", startupResult);
  }
#endif

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo("localhost", std::to_string(port).c_str(), &hints,
                        &servinfo)) != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(rv) << "\n";
    goto giveup;
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      // errno is set here, but we don't examine it.
      continue;
    }

    if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
#ifdef WIN32
      closesocket(sockfd);
#else
      close(sockfd);
#endif
      // errno is set here, but we don't examine it.
      continue;
    }

    break;
  }

  // Connection failed; give up.
  if (p == NULL) {
    goto giveup;
  }

  freeaddrinfo(servinfo);  // all done with this structure

  _connected = true;

  return;
giveup:
  _connected = false;
  return;
}

void Connector::done() {
  marshalling.makeDone();
  sendOverSocket();
}

void Connector::disconnect() {}

std::ostream& operator<<(std::ostream& os, Node& n) {
  return os << "print node here";
}


}
