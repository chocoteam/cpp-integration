/// v1.0

#ifndef CONNECTOR
#define CONNECTOR

#include "message.hh"
#include <ostream>

class vector;

namespace Profiling {

  using NodeStatus = cpprofiler::NodeStatus;

class Node;

template<typename T>
class Option {

  bool is_valid{false};
  T value_;

public:

  Option() = default;
  Option(T&& t): is_valid{true}, value_{t} {}
  Option(const Option& other) = default;
  Option& operator=(Option& other) = default;

  const T& value() const { return value_; }
  bool valid() const { return is_valid; }
  void set(const T& v) { is_valid = true; value_ = v; }

};

class Connector {
 private:


  MessageMarshalling marshalling;

  const unsigned int port;

  int sockfd;
  bool _connected;

  void sendOverSocket();

 public:
  void sendRawMsg(const std::vector<char>& buf);

  Connector(unsigned int port, unsigned int wid = -1);

  bool connected() { return _connected; }

  /// connect to a socket via port specified in the construction (6565 by
  /// default)
  void connect();

  // sends START_SENDING message to the Profiler with a model name
  void restart(const std::string& file_path = "", int restart_id = -1,
               const std::string& variableList = "", int execution_id = -1);

  void done();

  /// disconnect from a socket
  void disconnect();

  void sendNode(const Profiling::Node& node);

  Node createNode(int sid, int pid, int alt, int kids, NodeStatus status);
};

std::ostream& operator<<(std::ostream& os, Node& n);

class Node {

  using NodeStatus = cpprofiler::NodeStatus;

  Connector& _c;

  int sid_;
  int pid_;
  int alt_;
  int kids_;

  NodeStatus status_;

  Option<std::string> label_;
  Option<std::string> nogood_;
  Option<std::string> info_;
  Option<std::string> solution_;

  Option<int> tid_;
  Option<int> rid_;

 public:

  Node(int sid, int pid, int alt, int kids, NodeStatus status, Connector& c);

  Node& set_thread_id(int tid) {
    tid_.set(tid);
    return *this;
  }

  Node& set_restart_id(int rid) {
    rid_.set(rid);
    return *this;
  }

  const Option<std::string>& label() const { return label_; }

  Node& set_label(const std::string& label) {
    label_.set(label);
    return *this;
  }

  const Option<std::string>& nogood() const { return nogood_; }

  Node& set_nogood(const std::string& nogood) {
    nogood_.set(nogood);
    return *this;
  }

  const Option<std::string>& info() const { return info_; }

  Node& set_info(const std::string& info) {
    info_.set(info);
    return *this;
  }

  const Option<std::string>& solution() const { return solution_; }

  Node& set_solution(const std::string& solution) {
    solution_.set(solution);
    return *this;
  }

  int sid() const { return sid_; }
  int pid() const { return pid_; }
  int alt() const { return alt_; }
  int kids() const { return kids_; }

  const NodeStatus status() const { return status_; }

  Option<int> tid() const { return tid_; }
  Option<int> rid() const { return rid_; }

  void send();
};
}

#endif
