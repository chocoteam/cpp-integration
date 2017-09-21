#ifndef MESSAGE_HH
#define MESSAGE_HH

#include <vector>
#include <string>
#include <cassert>

namespace Profiling {

static const int32_t PROFILER_PROTOCOL_VERSION = 2;

enum NodeStatus {
  SOLVED = 0,        ///< Node representing a solution
  FAILED = 1,        ///< Node representing failure
  BRANCH = 2,        ///< Node representing a branch
  UNDETERMINED = 3,  ///< Node that has not been explored yet
  STOP = 4,          ///< Node representing stop point
  UNSTOP = 5,        ///< Node representing ignored stop point
  SKIPPED = 6,       ///< Skipped by backjumping
  MERGING = 7
};

enum class MsgType {
  NODE = 0,
  DONE = 1,
  START = 2,
};

template <typename T>
class Option {
  T value_;
  bool present{false};

public:
  bool valid() const { return present; }
  void set(const T& t) { present = true; value_ = t; }
  void unset() { present = false; }
  const T& value() const { assert(present); return value_; }
  T& value() { assert(present); return value_; }
};

// Unique identifier for a node
struct NodeUID {
  // Node number
  int nid;
  // Restart id
  int rid;
  // Thread id
  int tid;
};


class Message {
  MsgType _type;

  NodeUID _node;
  NodeUID _parent;
  int32_t _alt;
  int32_t _kids;
  NodeStatus _status;

  bool _have_label{false};
  std::string _label;

  bool _have_nogood{false};
  std::string _nogood;

  bool _have_info{false};
  std::string _info;

  bool _have_version{false};
  int32_t _version; // PROFILER_PROTOCOL_VERSION;

public:
  bool isNode(void) const { return _type == MsgType::NODE; }
  bool isDone(void) const { return _type == MsgType::DONE; }
  bool isStart(void) const { return _type == MsgType::START; }

  NodeUID nodeUID(void) const { return _node; }
  void set_nodeUID(const NodeUID& n) { _node = n; }

  NodeUID parentUID(void) const { return _parent; }
  void set_parentUID(const NodeUID& p) { _parent = p; }

  // required fields for node messages
  int32_t node_id(void) const { return _node.nid; }
  void set_node_id(int32_t id) { _node.nid = id; }

  int32_t parent_id(void) const { return _parent.nid; }
  void set_parent_id(int32_t pid) { _parent.nid = pid; }

  int32_t alt(void) const { return _alt; }
  void set_alt(int32_t alt) { _alt = alt; }

  int32_t kids(void) const { return _kids; }
  void set_kids(int32_t kids) { _kids = kids; }

  NodeStatus status(void) const { return _status; }
  void set_status(NodeStatus status) { _status = status; }

  // optional fields for node messages
  int32_t node_restart_id(void) const { return _node.rid; }
  int32_t node_thread_id(void) const {  return _node.tid; }

  int32_t parent_restart_id(void) const { return _parent.rid; }
  int32_t parent_thread_id(void) const {  return _parent.tid; }

  void set_label(const std::string& label) {
    _have_label = true;
    _label = label;
  }

  void set_info(const std::string& info) {
    _have_info = true;
    _info = info;
  }

  void set_nogood(const std::string& nogood) {
    _have_nogood = true;
    _nogood = nogood;
  }

  void set_version(int32_t v) {
    _have_version = true;
    _version = v;
  }

  void set_node_restart_id(int32_t rid) { _node.rid = rid; }
  void set_node_thread_id(int32_t tid) { _node.tid = tid; }

  void set_parent_restart_id(int32_t rid) { _parent.rid = rid; }
  void set_parent_thread_id(int32_t tid) { _parent.tid = tid; }

  bool has_version(void) const { return _have_version; }
  int32_t version(void) const { return _version; }

  bool has_label(void) const { return _have_label; }
  const std::string& label() const { return _label; }

  bool has_nogood(void) const { return _have_nogood; }
  const std::string& nogood(void) const { return _nogood; }

  // generic optional fields
  bool has_info(void) const { return _have_info; }
  const std::string& info(void) const { return _info; }

  void set_type(MsgType type) { _type = type; }
  MsgType type(void) const { return _type; }

  void reset(void) {
    _have_label = false;
    _have_nogood = false;
    _have_info = false;
    _have_version = false;
  }
};


class MessageMarshalling {

private:
  enum Field {
    ID = 0, // Not used
    PID = 1, // Not used
    ALT = 2,
    KIDS = 3,
    STATUS = 4,
    RESTART_ID = 5, // Not used
    THREAD_ID = 6, // Not used
    LABEL = 7,
    SOLUTION = 8, // Removed
    NOGOOD = 9,
    INFO = 10,
    VERSION = 11
  };

  Message msg;

  typedef char* iter;

  static void serializeType(std::vector<char>& data, MsgType f) {
    data.push_back(static_cast<char>(f));
  }

  static void serializeField(std::vector<char>& data, Field f) {
    data.push_back(static_cast<char>(f));
  }

  static void serialize(std::vector<char>& data, int32_t i) {
    data.push_back(static_cast<char>((i & 0xFF000000) >> 24));
    data.push_back(static_cast<char>((i & 0xFF0000) >> 16));
    data.push_back(static_cast<char>((i & 0xFF00) >> 8));
    data.push_back(static_cast<char>((i & 0xFF)));
  }

  static void serialize(std::vector<char>& data, NodeStatus s) {
    data.push_back(static_cast<char>(s));
  }

  static void serialize(std::vector<char>& data, const std::string& s) {
    serialize(data, static_cast<int32_t>(s.size()));
    for (char c : s) {
      data.push_back(c);
    }
  }

  static MsgType deserializeMsgType(iter& it) {
    auto m = static_cast<MsgType>(*it);
    ++it;
    return m;
  }

  static Field deserializeField(iter& it) {
    auto f = static_cast<Field>(*it);
    ++it;
    return f;
  }

  static int32_t deserializeInt(iter& it) {
    auto b1 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));
    auto b2 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));
    auto b3 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));
    auto b4 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));

    return static_cast<int32_t>(b1 << 24 | b2 << 16 | b3 << 8 | b4);
  }

  static NodeStatus deserializeStatus(iter& it) {
    auto f = static_cast<NodeStatus>(*it);
    ++it;
    return f;
  }

  static std::string deserializeString(iter& it) {
    std::string result;
    // std::cerr << "string size:\n";
    int32_t size = deserializeInt(it);
    result.reserve(static_cast<size_t>(size));
    for (int32_t i = 0; i < size; i++) {
      result += *it;
      ++it;
    }
    return result;
  }

public:
  Message& makeNode(NodeUID node, NodeUID parent,
                    int32_t alt, int32_t kids, NodeStatus status) {
    msg.reset();
    msg.set_type(MsgType::NODE);

    msg.set_nodeUID(node);
    msg.set_parentUID(parent);

    msg.set_alt(alt);
    msg.set_kids(kids);
    msg.set_status(status);

    return msg;
  }

  void makeStart(int rid, const std::string& label, const std::string& info) {
    msg.reset();
    msg.set_type(MsgType::START);
    msg.set_version(Profiling::PROFILER_PROTOCOL_VERSION);
    msg.set_node_restart_id(rid);
    msg.set_label(label);
    msg.set_info(info);
  }

  void makeDone(void) {
    msg.reset();
    msg.set_type(MsgType::DONE);
  }

  const Message& get_msg(void) { return msg; }

  std::vector<char> serialize(void) const {
    std::vector<char> data;
    size_t dataSize = 1 + (msg.isNode() ? 4 * 8 + 1 : 0) +
        (msg.has_label() ? 1 + 4 + msg.label().size() : 0) +
        (msg.has_nogood() ? 1 + 4 + msg.nogood().size() : 0) +
        (msg.has_info() ? 1 + 4 + msg.info().size() : 0);
    data.reserve(dataSize);

    serializeType(data, msg.type());
    if (msg.isNode()) {
      // serialize NodeId node
      serialize(data, msg.node_id());
      serialize(data, msg.node_restart_id());
      serialize(data, msg.node_thread_id());
      // serialize NodeId parent
      serialize(data, msg.parent_id());
      serialize(data, msg.parent_restart_id());
      serialize(data, msg.parent_thread_id());
      // Other Data
      serialize(data, msg.alt());
      serialize(data, msg.kids());
      serialize(data, msg.status());
    }

    if(msg.has_version()) {
      serializeField(data, VERSION);
      serialize(data, msg.version());
    }
    if (msg.has_label()) {
      serializeField(data, LABEL);
      serialize(data, msg.label());
    }
    if (msg.has_nogood()) {
      serializeField(data, NOGOOD);
      serialize(data, msg.nogood());
    }
    if (msg.has_info()) {
      serializeField(data, INFO);
      serialize(data, msg.info());
    }
    return data;
  }

  void deserialize(char* data, size_t size) {
    char *end = data + size;
    msg.set_type(deserializeMsgType(data));
    if (msg.isNode()) {
      msg.set_node_id(deserializeInt(data));
      msg.set_node_restart_id(deserializeInt(data));
      msg.set_node_thread_id(deserializeInt(data));

      msg.set_parent_id(deserializeInt(data));
      msg.set_parent_restart_id(deserializeInt(data));
      msg.set_parent_thread_id(deserializeInt(data));

      msg.set_alt(deserializeInt(data));
      msg.set_kids(deserializeInt(data));
      msg.set_status(deserializeStatus(data));
    }

    msg.reset();

    while (data != end) {
      MessageMarshalling::Field f = deserializeField(data);
      switch (f) {
      case VERSION:
        msg.set_version(deserializeInt(data)); break;
      case LABEL:
        msg.set_label(deserializeString(data)); break;
      case NOGOOD:
        msg.set_nogood(deserializeString(data)); break;
      case INFO:
        msg.set_info(deserializeString(data)); break;
      default:
        break;
      }
    }
  }
};

}

#endif  // MESSAGE_HH
