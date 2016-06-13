/// v1.0

#ifndef CONNECTOR
#define CONNECTOR

#include "message.pb.hh"
// #include "nn.hpp"
// #include <nanomsg/pipeline.h>
// #include <nanomsg/tcp.h>

// #include <boost/asio.hpp>

// using namespace boost::asio;
// using namespace boost::asio::ip;

#include <iostream>

namespace message {
  class Node;
}

namespace Profiling {

  class Node;

  enum NodeStatus {
    SOLVED = 0,
    FAILED = 1,
    BRANCH = 2,
    SKIPPED = 6,
    MERGING = 7
  };

  class Connector {

  private:

    const unsigned int port;
    unsigned int _thread_id;

    // nn::socket nanosocket;
      // int socketfd;
      // io_service service;
      // tcp::socket socket;
      int sockfd;
      bool _connected;
    // int endpoint;

    void sendOverSocket(const message::Node& msg);

  public:
    void sendRawMsg(const char* buf, int len);

    Connector(unsigned int port, unsigned int tid = 0);

      bool connected() { return _connected; }

    /// connect to a socket via port specified in the construction (6565 by default)
    void connect();

    // sends START_SENDING message to the Profiler with a model name
    void restart(const std::string& file_path = "", int restart_id = -1, const std::string& variableList = "");

    void done();

    /// disconnect from a socket
    void disconnect();

    void sendNode(int sid,
                  int pid,
                  int alt,
                  int kids,
                  NodeStatus status,
                  const char* label,
                  unsigned int thread = -1,
                  int restart = -1,
                  float domain = -1,
                  const std::string& nogood = "",
                  int nogood_bld = -1,
                  bool uses_assumptions = false,
                  const std::string& info = "");

    void sendNode(const Profiling::Node& node);

    Node createNode(int sid, int pid, int alt, int kids, NodeStatus status);

  };

  class Node {

  private:

    message::Node _node;

    const message::Node& get_node() const {
      return _node;
    }

    Node(const Node& node); /// no copying allowed

    Connector& _c;

  public:

    friend class Connector;

    Node(int sid, int pid, int alt, int kids, NodeStatus status, Connector& c);

    ~Node();

    void send();

    inline Node& set_label(const std::string& label) {
      _node.set_label(label);
      return *this;
    }

    inline Node& set_thread_id(unsigned int thread_id) {
      _node.set_thread_id(thread_id);
      return *this;
    }

    inline Node& set_restart_id(int restart_id) {
      _node.set_restart_id(restart_id);
      return *this;
    }

    inline Node& set_domain_size(float domain_size) {
      _node.set_domain_size(domain_size);
      return *this;
    }

    inline Node& set_nogood(const std::string& nogood) {
      _node.set_nogood(nogood);
      return *this;
    }

    inline Node& set_nogood_bld(int nogood_bld) {
      _node.set_nogood_bld(nogood_bld);
      return *this;
    }

    inline Node& set_uses_assumptions(bool uses_assumptions) {
      _node.set_uses_assumptions(uses_assumptions);
      return *this;
    }

    inline Node& set_backjump_distance(int backjump_distance) {
      _node.set_backjump_distance(backjump_distance);
      return *this;
    }

    inline Node& set_decision_level(int decision_level) {
      _node.set_decision_level(decision_level);
      return *this;
    }

    inline Node& set_info(const std::string& nogood) {
      _node.set_info(nogood);
      return *this;
    }

    inline Node& set_solution(const std::string& solution) {
      _node.set_solution(solution);
      return *this;
    }

    inline Node& set_time(unsigned long long time) {
      _node.set_time(time);
      return *this;
    }

    void print(std::ostream& out) {
      out << _node.type() << ","
          << _node.sid() << ","
          << _node.restart_id() << ","
          << _node.pid() << ","
          << _node.alt() << ","
          << _node.kids() << ","
          << _node.status() << ","
          << _node.time() << ","
          << _node.label() << ","
          << _node.nogood() << ","
          << _node.nogood_bld() << ","
          << _node.uses_assumptions() << ","
          << _node.backjump_distance() << ","
          << _node.decision_level() << ","
          << _node.info() << "\n";
    }
  };
}

#endif
