// [[Rcpp::plugins(cpp11)]]
//=================================
// include guard
//=================================
#ifndef __NODE_INCLUDED__
#define __NODE_INCLUDED__
using std::string;
using std::vector;
using std::unordered_set;

//=================================
// What this file declares
//=================================
class  Node;
struct connection_info;

// For a bit of clarity
typedef vector<Node*> NodeList;

//=================================
// Main node class declaration
//=================================
class Node {
  public:
    // Node();
    Node(string, int);       // Constructor function takes ID, node hiearchy level, and assumes default 0 for type
    Node(string, int, int);  // Constructor function takes ID, node hiearchy level, and specification of type as integer
    // ==========================================
    // Attributes
    string               id;          // Unique integer id for node
    NodeList             connections; // Nodes that are connected to this node  
    int                  level;       // What level does this node sit at (0 = data, 1 = cluster, 2 = super-clusters, ...)
    Node*                parent;      // What node contains this node (aka its cluster)
    bool                 has_parent;  // Does this node have a parent or is it the currently highest level?
    unordered_set<Node*> children;    // Nodes that are contained within node (if node is cluster)
    int                  type;        // What type of node is this?

    // ==========================================
    // Methods   
    void             set_parent(Node*);               // Set current node parent/cluster
    void             add_child(Node*);                // Add a node to the children vector
    void             remove_child(Node*);             // Remove a child node 
    void             add_connection(Node*);           // Add connection to another node
    NodeList         get_children_at_level(int);      // Get all member nodes of current node at a given level
    Node*            get_parent_at_level(int);        // Get parent of node at a given level
    NodeList         get_connections_to_level(int);   // Get all nodes connected to Node at a given level
    connection_info  connections_to_node(Node*);      // Get info on connection between any two nodes
    static void      connect_nodes(Node*, Node*);     // Static method to connect two nodes to each other with edge
    double           prob_of_joining_group(Node*, NodeList, int); // Get probability node transitions to a given group
};

struct connection_info {
  int n_between;
  int n_total;
};

#endif