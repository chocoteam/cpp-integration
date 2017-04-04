#include "connector.hh"
#include <iostream>
#include <ctime>
#include <sstream>
#include <cstdint>

#include <cstdio>
#include <cstring>

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#else

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#endif

template <typename T>
std::string NumberToString ( T Number )
{
  std::ostringstream ss;
  ss << Number;
    return ss.str();
}

namespace Profiling {

  Node::Node(int sid, int pid, int alt, int kids, NodeStatus status, Connector& c) : _c(c) {

    _node.set_type(message::Node::NODE);

    _node.set_sid(sid);
    _node.set_pid(pid);
    _node.set_alt(alt);
    _node.set_kids(kids);
    _node.set_status(static_cast<message::Node::NodeStatus>(status));
    _node.set_thread_id(-1); /// -1 is default for thread id
  }

  void Node::send() {
    this->_c.sendNode(*this);
  }

  Node::~Node() {

  }

}

namespace Profiling {

  // get sockaddr, IPv4 only!
  void *get_in_addr(struct sockaddr *sa)
  {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return NULL;
  }

  Connector::Connector(unsigned int port, unsigned int tid)
    : port(port), _thread_id(tid),
      // socket(service),
      _connected(false)
  {}

  // From http://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall
  int sendall(int s, const char *buf, int *len)
  {
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
        
    while(total < *len) {
      n = send(s, buf+total, bytesleft, 0);
      if (n == -1) { break; }
      total += n;
      bytesleft -= n;
    }
        
    *len = total; // return number actually sent here
        
    return n==-1?-1:0; // return -1 on failure, 0 on success
  } 
    
  void Connector::sendOverSocket(const message::Node &msg) {
    if (!_connected) return;
    std::string msg_str;
    msg.SerializeToString(&msg_str);

    char *buf = new char[msg_str.size()];
    memcpy(buf, msg_str.c_str(), msg_str.size());

    sendRawMsg(buf, msg_str.size());
  }

  void Connector::sendRawMsg(const char* buf, int len) {
    uint32_t bufSize = len;
    int bufSizeLen = sizeof(uint32_t);
    sendall(sockfd, reinterpret_cast<char*>(&bufSize), &bufSizeLen);
    int bufSizeInt = bufSize;
    sendall(sockfd, buf, &bufSizeInt);
  }

  void Connector::sendNode(int sid, int pid, int alt, int kids,
                           NodeStatus status, const char* label, unsigned int thread, int restart,
                           float domain, const std::string& nogood, int nogood_bld, bool uses_assumptions, const std::string& info) {
    if (!_connected) return;

    message::Node node;

    node.set_type(message::Node::NODE);

    node.set_sid(sid);
    node.set_pid(pid);
    node.set_alt(alt);
    node.set_kids(kids);

    node.set_status(static_cast<message::Node::NodeStatus>(status));
    node.set_label(label);
    node.set_thread_id(thread);
    node.set_restart_id(restart);
    node.set_domain_size(domain);
    // node.set_solution(solution);
    node.set_nogood(nogood);
    node.set_nogood_bld(nogood_bld);
    node.set_uses_assumptions(uses_assumptions);
    node.set_info(info);

    sendOverSocket(node);
  }

  Node Connector::createNode(int sid, int pid, int alt, int kids, NodeStatus status) {
    return Node(sid, pid, alt, kids, status, *this);
  }

  void Connector::sendNode(const Profiling::Node& node) {
    sendOverSocket(node.get_node());
  }

  void Connector::restart(const std::string& file_path, int restart_id, const std::string& variableList, int execution_id) {
    // std::cerr << "restarting gist, restart_id: " << restart_id << "\n";

    message::Node dummy_node;
    dummy_node.set_type(message::Node::START);

    /// should it be restart_id instead?
    dummy_node.set_restart_id(restart_id);

    /// extract fzn file name
    std::string name(file_path);
    int pos = name.find_last_of('/');
    if (pos > 0) {
      name = name.substr(pos + 1, name.length() - pos - 1);
    }

    dummy_node.set_label(name);

    std::stringstream infoStream;
    if(execution_id != -1) {
      infoStream << "\"execution_id\": " << execution_id;
    }
    if (variableList.size() > 0) {
      infoStream << "\"variable_list\": " << variableList << ","
    }
    dummy_node.set_info(infoStream.str());

    sendOverSocket(dummy_node);
  }

  void Connector::connect() {
    struct addrinfo hints, *servinfo, *p;
    int rv;

    #ifdef WIN32
    // Initialise Winsock.
    WSADATA wsaData;
    int startupResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (startupResult != 0) {
        printf("WSAStartup failed with error: %d\n", startupResult);
    }
    #endif

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("localhost", std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
      std::cerr << "getaddrinfo: " << gai_strerror(rv) << "\n";
      goto giveup;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
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
    
    freeaddrinfo(servinfo); // all done with this structure

    _connected = true;

    return;
  giveup:
    _connected = false;
    return;
  }

  void Connector::done() {
    message::Node dummy_node;
    dummy_node.set_type(message::Node::DONE);
    sendOverSocket(dummy_node);
  }

  void Connector::disconnect() {
  }

}
