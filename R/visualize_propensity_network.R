#' Plot network of nodes connected by pairwise block propensity
#'
#' Takes the results of `mcmc_sweep()` with `track_pairs = TRUE` and plots the
#' data as a network where two nodes have an edge between them if their pairwise
#' propensity to be in the same block is above some threshold.
#'
#' @inheritParams  visualize_propensity_dist
#' @param proportion_threshold Threshold of pairwise propensity to consider two nodes linked. Choose carefully!
#'
#' @return An interactive network plot with force layout.
#' @export
#'
#' @examples
#'
#' set.seed(42)
#' # Simulate network data and initialize model with it
#' my_sbm <- sim_basic_block_network(n_blocks = 3,n_nodes_per_block = 30) %>%
#'   create_sbm()
#'
#' # Run collapsing algorithm
#' collapse_results <- my_sbm %>% collapse_blocks(desired_num_blocks = 1, sigma = 1.1)
#'
#' # Choose best collapse state
#' my_sbm <- choose_best_collapse_state(my_sbm, collapse_results)
#'
#' # Run MCMC sweeps and track pairs
#' sweep_results <- mcmc_sweep(my_sbm, num_sweeps = 100, eps = 0.4, track_pairs = TRUE)
#'
#' # Plot connection propensity network
#' visualize_propensity_network(sweep_results, proportion_threshold = 0.4)
#'
visualize_propensity_network <- function(sweep_results, proportion_threshold = 0.2){

  # Make sure we have propensity counts before proceeding
  if(is.null(sweep_results$pairing_counts)){
    stop("Sweep results do not contain pairwise propensities. Try rerunning MCMC sweep with track_pairs = TRUE.")
  }

  edges <- sweep_results$pairing_counts %>%
    dplyr::filter(proportion_connected > proportion_threshold) %>%
    dplyr::select(from = node_a, to = node_b)

  if (nrow(edges) == 0){
    stop("No node-node propensities exceed threshold. Try lowering threshold and/or making sure your model's MCMC chain has equilibriated.")
  }

  # Get all unique nodes and their avg non-zero pairwise connection proportions
  nodes <- sweep_results$pairing_counts %>%
    tidyr::pivot_longer(c(node_a, node_b), values_to="id") %>%
    dplyr::group_by(id) %>%
    dplyr::summarise(avg_prop_connection = mean(proportion_connected[proportion_connected > 0])) %>%
    dplyr::distinct(id, avg_prop_connection)

  sbmR::visualize_network(edges, nodes, node_color_col = "avg_prop_connection")
}