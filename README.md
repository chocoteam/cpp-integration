
#### 1. Create a connector instance

```c++
unsigned int port = 6565;
```

```c++
Connector c(port);
```

#### 2. Establish a connection and start a new search tree

```c++
/// Establishes a socket connection using the port specified above
c.connect();

/// Tells the profiler to start a new tree
c.restart("example");

/// Also used in case of a restart with restart id specified
c.restart("example", 1);
```

#### 3. Send data every time the solver branches/fails/finds a solution

##### The old way

```c++
c.sendNode(node_id, parent_id, alt, kids, status, label);
```

where

field   | type | description
------  | ---- | -----------
node_id   | int | current node's identifier
parent_id | int | identifier of node's parent
alt       | int | which of its siblings the node is (0 for the left-most)
kids      | int | number of children
status    | Profiling::NodeStatus | determines the node's type (solution, failure, branching etc)
label     | std::string | some text-based information to go along with the node (ie branching decision

##### An alternative way

```c++
// Create a node on a stack with mandatory fields
Node node(2, 0, 1, 0, NodeStatus::SOLVED);
```

```c++
// Separately specify optional fields (whichever available)
node.set_label("b");
```

```c++
// Send the node
c.sendNode(node);
```

#### 4. Finish the tree and release the socket

```c++
c.done();
c.disconnect();
```
