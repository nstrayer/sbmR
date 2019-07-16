// [[Rcpp::plugins(cpp11)]]
#include <Rcpp.h>
#include <random>
#include "Node.h" 

using namespace Rcpp;
using std::string;
using std::vector;
using std::map;

// Setup random number generation
std::random_device rand_dev;
std::mt19937 generator(rand_dev());


// =======================================================
// Constructor that takes the nodes unique id integer and type
// =======================================================
Node::Node(int node_id, bool type_a){
  id = node_id;
  is_type_a = type_a;
  degree = 0;
}



// =======================================================
// Add connection to edge map
// =======================================================
void Node::add_edge(Node* node_ptr) {
  map<int, Edge>::iterator edge_to_add;
  
  // Try and find the node in edges.
  edge_to_add = edges.find(node_ptr->id);
  
  // Is there already a connection to this node?
  bool already_connected = edge_to_add != edges.end();
  
  if(already_connected){
    // If there is already a connection just iterate the count up
    edge_to_add->second.count++;
  } else {
    // Otherwise add a whole new edge to edges map.
    Edge new_edge;
    new_edge.node = node_ptr;
    new_edge.count = 1;
    edges[node_ptr->id] = new_edge;
  }
  
  // Increment the degree of the node up
  degree++;
}          

// =======================================================
// Remove a connection from edge map
// =======================================================
void Node::remove_edge(Node* node_ptr, bool remove_all){
  int num_edges;
  bool single_edge;
  map<int, Edge>::iterator edge_to_delete;
  
  // Try and find the node in edges.
  edge_to_delete = edges.find(node_ptr->id);
  
  // Is there already a connection to this node?
  bool node_in_edges = edge_to_delete != edges.end();
  
  // If edge exists, remove it.
  if(node_in_edges){
    num_edges = edge_to_delete->second.count;
    single_edge = num_edges == 1;
    
    if(remove_all | single_edge){
      // If we're removing all the edges erase from map
      edges.erase(edge_to_delete);
      degree -= num_edges;
    } else {
      // Otherwise simple remove one edge but keep map entry
      edge_to_delete->second.count--;
      degree--;
    }
  }
}      

// =======================================================
// How many total edges to another node?
// =======================================================
int Node::num_edges_to_node(Node*){
  
  return 1;
} 

// =======================================================
// Methods to implement
// =======================================================
Node*         get_random_neighbor();    // Find a random neighbor node
void          add_member(Node*);        // Add a node to the members map
void          swap_clusters(Node*);     // Swap current cluster with a new one
vector<Edge>  num_edges_to_clusters();  // Get how many edges to all represented neighbor clusters



// [[Rcpp::export]]
List make_node_and_print(
    bool add_edge, 
    bool remove_all_edges
){
  Node node_a(1, true),
       node_b(2, false),
       node_c(3, true);
  
  if(add_edge){
    node_a.add_edge(&node_b);
    node_a.add_edge(&node_b);
  }
  
  if(remove_all_edges){
    node_a.remove_edge(&node_b, true);
  } else{
    node_a.remove_edge(&node_b, false);
  }
  
  return List::create(
    _["id"] = node_a.id,
    _["degree"] = node_a.degree
  );
}



/*** R
make_node_and_print(TRUE, FALSE)
make_node_and_print(TRUE, TRUE)
*/
