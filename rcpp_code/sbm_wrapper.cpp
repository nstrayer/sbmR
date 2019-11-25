#include <Rcpp.h>
#include "../SBM.h"

using namespace Rcpp;

class Rcpp_SBM: public SBM {
public:
  // Rcpp_SBM(int seed):sampler(seed) {};
  void add_node_rcpp(
      std::string id,
      int type,
      int level)
  {
    add_node(id, type, level);
  }
  
  void add_connections_rcpp(std::string node_a_id, std::string node_b_id)
  {
    add_connection(node_a_id, node_b_id);
  }

  DataFrame get_state_rcpp()
  {
    // Grab state dump struct
    State_Dump state = get_state();

    // Create and return dump of state as dataframe
    return DataFrame::create(
        _("id") = state.id,
        _("parent") = state.parent,
        _("type") = state.type,
        _("level") = state.level);
  }

  void set_node_parent(std::string child_id, std::string parent_id, int level = 0)
  {
    NodePtr child_node = get_node_by_id(child_id, level);
    NodePtr parent_node = get_node_by_id(parent_id, level + 1);

    child_node->set_parent(parent_node);
  }
};



RCPP_MODULE(sbm_module)
{
  class_<Rcpp_SBM>("Rcpp_SBM")
  
  .constructor()
  
  .method("add_node_rcpp", &Rcpp_SBM::add_node_rcpp)
  .method("add_connections_rcpp", &Rcpp_SBM::add_connections_rcpp)
  .method("get_state_rcpp", &Rcpp_SBM::get_state_rcpp)
  .method("set_node_parent", &Rcpp_SBM::set_node_parent)
  ;
}

/*** R
sbm <- new(Rcpp_SBM)

sbm$add_node_rcpp("a1", 0L, 0L)
sbm$add_node_rcpp("a2", 0L, 0L)
sbm$add_node_rcpp("a3", 0L, 0L)
sbm$add_node_rcpp("b1", 1L, 0L)
sbm$add_node_rcpp("b2", 1L, 0L)
sbm$add_node_rcpp("b3", 1L, 0L)

sbm$add_node_rcpp("a11", 0L, 1L)
sbm$add_node_rcpp("a12", 0L, 1L)
sbm$add_node_rcpp("b11", 1L, 1L)
sbm$add_node_rcpp("b12", 1L, 1L)


sbm$add_connections_rcpp("a1", "b1")
sbm$add_connections_rcpp("a1", "b2")
sbm$add_connections_rcpp("a1", "b3")
sbm$add_connections_rcpp("a2", "b3")
sbm$add_connections_rcpp("a3", "b2")

sbm$set_node_parent("a1", "a11", 0)
sbm$set_node_parent("a2", "a11", 0)
sbm$set_node_parent("a3", "a12", 0)
sbm$set_node_parent("b1", "b11", 0)
sbm$set_node_parent("b2", "b11", 0)
sbm$set_node_parent("b3", "b12", 0)


sbm$get_state_rcpp()

*/