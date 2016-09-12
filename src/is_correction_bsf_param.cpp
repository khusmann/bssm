// #include "bssm.h"
// #include "ngssm.h"
// #include "ng_bsm.h"
// #include "svm.h"
// 
// 
// // [[Rcpp::plugins(openmp)]]
// template <typename T>
// void is_correction_bsf_param(T mod, const arma::mat& theta, const arma::vec& ll_store,
//   const arma::uvec& counts, unsigned int nsim_states,
//   unsigned int n_threads, arma::vec& weights_store) {
// 
//   unsigned n_iter = theta.n_cols;
// 
//   arma::uvec cum_counts = arma::cumsum(counts);
// #pragma omp parallel num_threads(n_threads) default(none)           \
//   shared(n_threads, ll_store, n_iter, nsim_states, theta, ess_treshold,                    \
//     weights_store, counts, cum_counts) firstprivate(mod)
//     {
// #ifdef _OPENMP
// if (n_threads > 1) {
//   mod.engine = std::mt19937(omp_get_thread_num() + 1);
// }
// #endif
// #pragma omp for schedule(static)
//       for (unsigned int i = 0; i < n_iter; i++) {
// 
//         arma::vec theta_i = theta.col(i);
//         mod.update_model(theta_i);
//         mod.particle_filter(nsim_states * counts(i), arma::cube&, arma::mat&, arma::umat&);)
//         weights_store(i) = exp(mod.particle_loglik(nsim_states * counts(i)) - ll_store(i));
// 
//       }
//     }
// }
// template void is_correction_bsf_param<svm>(svm mod, const arma::mat& theta, const arma::vec& ll_store,
//   const arma::uvec& counts,
//   unsigned int nsim_states, unsigned int n_threads,
//   arma::vec& weights_store, double ess_treshold);