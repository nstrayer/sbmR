#include <Rcpp.h>
#include "../SBM.h"

using namespace Rcpp;

inline DataFrame state_to_df(State_Dump state)
{
  // Create and return dump of state as dataframe
  return DataFrame::create(
      _["id"] = state.id,
      _["parent"] = state.parent,
      _["type"] = state.type,
      _["level"] = state.level,
      _["stringsAsFactors"] = false);
}

class Rcpp_SBM : public SBM
{
public:
  // Rcpp_SBM(int seed):sampler(seed) {};
  void add_node(const std::string id,
                const int type,
                const int level)
  {
    SBM::add_node(id, type, level);
  }

  void add_connection(const std::string node_a_id, const std::string node_b_id)
  {
    SBM::add_connection(node_a_id, node_b_id);
  }

  DataFrame get_state()
  {
    // Grab state dump struct return as dataframe
    return state_to_df(SBM::get_state());
  }

  void set_node_parent(const std::string child_id,
                       const std::string parent_id,
                       const int level = 0,
                       const bool building_network = false)
  {
    get_node_by_id(child_id, level)->set_parent(get_node_by_id(parent_id, level + 1));
  }

  double compute_entropy(const int level)
  {
    return SBM::compute_entropy(level);
  }

  int mcmc_sweep(int level, bool variable_num_groups)
  {
    std::cout << "Running rcpp_SBM mcmc function" << std::endl;
    return SBM::mcmc_sweep(level, variable_num_groups);
  }

  List collapse_groups(const int node_level, const int num_mcmc_steps)
  {

    auto init_results = SBM::collapse_groups(node_level, num_mcmc_steps);

    List entropy_results;

    for (auto step = init_results.begin();
         step != init_results.end();
         step++)
    {

      entropy_results.push_back(
          List::create(
              _["entropy"] = step->entropy,
              _["state"] = state_to_df(step->state)));
    }

    return entropy_results;
  }

  void load_from_state(std::vector<string> id,
                       std::vector<string> parent,
                       std::vector<int> level,
                       std::vector<int> type)
  {
    // Construct a state dump from vectors and
    // pass the constructed state to load_state function
    SBM::load_from_state(State_Dump(id, parent, level, type));
  }

  // Getters and setters for inhereted fields
  void set_beta(const double beta) { BETA = beta; }
  double get_beta() { return BETA; }

  void set_epsilon(const double eps) { EPS = eps; }
  double get_epsilon() { return EPS; }

  void set_sigma(const double sigma) { SIGMA = sigma; }
  double get_sigma() { return SIGMA; }

  void set_greedy(const bool greedy) { GREEDY = greedy; }
  bool get_greedy() { return GREEDY; }

  void set_n_checks_per_group(const int n) { N_CHECKS_PER_GROUP = n; }
  int get_n_checks_per_group() { return N_CHECKS_PER_GROUP; }
};

RCPP_MODULE(sbm_module)
{
  class_<Rcpp_SBM>("Rcpp_SBM")

      .constructor()

      .property("EPS",
                &Rcpp_SBM::get_epsilon, &Rcpp_SBM::set_epsilon,
                "Epsilon value for ergodicity")

      .property("BETA",
                &Rcpp_SBM::get_beta, &Rcpp_SBM::set_beta,
                "Beta value for MCMC acceptance probability")

      .property("GREEDY",
                &Rcpp_SBM::get_greedy, &Rcpp_SBM::set_greedy,
                "Perform merging and sweeps in greedy way?")

      .property("SIGMA",
                &Rcpp_SBM::get_sigma, &Rcpp_SBM::set_sigma,
                "Sigma value for determining rate of agglomerative merging")

      .property("N_CHECKS_PER_GROUP",
                &Rcpp_SBM::get_n_checks_per_group, &Rcpp_SBM::set_n_checks_per_group,
                "If not in greedy mode, how many options do we check per node for moves in agglomerative merging?")

      .method("add_node",
              &Rcpp_SBM::add_node) 
      .method("add_connection",
              &Rcpp_SBM::add_connection)
      .method("set_node_parent",
              &Rcpp_SBM::set_node_parent)
      .method("get_state",
              &Rcpp_SBM::get_state)
      .method("load_from_state",
              &Rcpp_SBM::load_from_state)
      .method("compute_entropy",
              &Rcpp_SBM::compute_entropy)
      .method("mcmc_sweep",
              &Rcpp_SBM::mcmc_sweep)
      .method("collapse_groups",
              &Rcpp_SBM::collapse_groups);
}

/*** R
sbm <- new(Rcpp_SBM)

sbm$add_node("a1", 0L, 0L)
sbm$add_node("a2", 0L, 0L)
sbm$add_node("a3", 0L, 0L)
sbm$add_node("b1", 1L, 0L)
sbm$add_node("b2", 1L, 0L)
sbm$add_node("b3", 1L, 0L)

sbm$add_node("a11", 0L, 1L)
sbm$add_node("a12", 0L, 1L)
sbm$add_node("b11", 1L, 1L)
sbm$add_node("b12", 1L, 1L)

sbm$add_connection("a1", "b1")
sbm$add_connection("a1", "b2")
sbm$add_connection("a1", "b3")
sbm$add_connection("a2", "b3")
sbm$add_connection("a3", "b2")


sbm$set_node_parent("a1", "a11", 0, TRUE)
sbm$set_node_parent("a2", "a11", 0, TRUE)
sbm$set_node_parent("a3", "a12", 0, TRUE)
sbm$set_node_parent("b1", "b11", 0, TRUE)
sbm$set_node_parent("b2", "b11", 0, TRUE)
sbm$set_node_parent("b3", "b12", 0, TRUE)


load_state <- function(sbm, state_dump){
  sbm$load_from_state(
    state_dump$id, 
    state_dump$parent, 
    state_dump$level, 
    state_dump$type)
}

# Set some model parameters
sbm$GREEDY <- TRUE
sbm$BETA <- 1.5
sbm$EPS <- 0.1
sbm$N_CHECKS_PER_GROUP <- 5

original_state <- sbm$get_state()

# for(i in 1:10){
#   entro_pre <- sbm$compute_entropy(0L)
#   groups_moved <- sbm$mcmc_sweep(0L,FALSE)
#   print(paste("started with entropy of", entro_pre, "and moved", groups_moved))
# }
# 
# new_state <- sbm$get_state()
# 
# # Bring me back to original state
# load_state(sbm, original_state)

library(tidyverse)
merge_results <- sbm$collapse_groups(0, 15)
load_state(sbm, original_state)
new_state <- sbm$get_state()

sort_state <- . %>%  arrange(level, type, id)
original_state %>% sort_state()
new_state %>% sort_state()

# 
# 
# desired_state <- init_results[[1]]$state
# 
# sbm$get_state()
# load_state(sbm, original_state)
# sbm$get_state()
# sbm$mcmc_sweep(0L,FALSE)
# sbm$mcmc_sweep(0L,FALSE)
# sbm$compute_entropy(0L)


*/