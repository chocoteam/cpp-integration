
#### 1. Create a connector instance

```java
int port = 6565;
```

```java
Connector c = new Connector(port);
```

#### 2. Establish a connection and start a new search tree

```java
/// Establishes a socket connection using the port specified above
c.connect();

/// Tells the profiler to start a new tree
c.restart("example");

/// Also used in case of a restart with restart id specified
c.restart("example", 1);
```

#### 3. Send data every time the solver branches/fails/finds a solution

```java
/// Create a node on a stack with mandatory fields
Node node = c.createNode(node_id, parent_id, alt, kids, status);
```

```java
// Specify optional fields (whichever available)
node.set_label("b");
```

```java
// Send the node
c.sendNode(node);
```

Or all in one line:

```java
c.createNode(node_id, parent_id, alt, kids, status).set_label("b").send();
```


The parameters are:

field   | type | description
------  | ---- | -----------
node_id   | int | current node's identifier
parent_id | int | identifier of node's parent
alt       | int | which of its siblings the node is (0 for the left-most)
kids      | int | number of children
status    | Profiling::NodeStatus | determines the node's type (solution, failure, branching etc)
label     | std::string | some text-based information to go along with the node (ie branching decision

#### 4. Finish the tree and release the socket

```java
c.done();
c.disconnect();
```
