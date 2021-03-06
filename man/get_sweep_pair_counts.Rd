% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/get_sweep_pair_counts.R
\name{get_sweep_pair_counts}
\alias{get_sweep_pair_counts}
\title{Get pairwise group sharing counts from model}
\usage{
get_sweep_pair_counts(sbm)
}
\arguments{
\item{sbm}{Object of class \code{sbm_network}.}
}
\value{
pairwise connection propensity counts from latest MCMC sweep
}
\description{
Retrieves the pairwise connection propensity counts from latest MCMC sweep on an \code{sbm_network} object. Needs to have
\code{\link{mcmc_sweep}}\code{(track_pairs = TRUE)} run on the object before.
}
\examples{

# Start with a small random network
net <- sim_basic_block_network(n_blocks = 3, n_nodes_per_block = 15) \%>\%
  initialize_blocks(num_blocks = 4) \%>\%
  mcmc_sweep(num_sweeps = 4, variable_num_blocks = FALSE)

# Retrieve the sweep results from network
get_sweep_results(net)

}
\seealso{
Other helpers: 
\code{\link{build_score_fn}()},
\code{\link{get_combination_indices}()},
\code{\link{get_sweep_results}()},
\code{\link{print.sbm_network}()},
\code{\link{rolling_mean}()},
\code{\link{verify_model}()}
}
\concept{helpers}
