#ifdef _OPENMP
#include <omp.h>
#endif
#include <ramcmc.h>
#include "ung_amcmc.h"
#include "ugg_ssm.h"
#include "ung_ssm.h"
#include "ugg_bsm.h"
#include "ung_bsm.h"
#include "ung_svm.h"

#include "distr_consts.h"
#include "filter_smoother.h"

ung_amcmc::ung_amcmc(const arma::uvec& prior_distributions, 
  const arma::mat& prior_parameters, const unsigned int n_iter, 
  const unsigned int n_burnin, const unsigned int n_thin, const unsigned int n, 
  const unsigned int m, const double target_acceptance, const double gamma, 
  const arma::mat& S, const bool store_states) :
  mcmc(prior_distributions, prior_parameters, n_iter, n_burnin, n_thin, n, m,
    target_acceptance, gamma, S, store_states),
    weight_storage(arma::vec(n_samples, arma::fill::zeros)),
    scales_storage(arma::mat(n, n_samples)),
    y_storage(arma::mat(n, n_samples)), H_storage(arma::mat(n, n_samples)),
    approx_loglik_storage(arma::vec(n_samples)), 
    prior_storage(arma::vec(n_samples)){
}

void ung_amcmc::trim_storage() {
  theta_storage.resize(n_par, n_stored);
  posterior_storage.resize(n_stored);
  count_storage.resize(n_stored);
  alpha_storage.resize(alpha_storage.n_rows, alpha_storage.n_cols, n_stored);
  weight_storage.resize(n_stored);
  scales_storage.resize(y_storage.n_rows, n_stored);
  y_storage.resize(y_storage.n_rows, n_stored);
  H_storage.resize(H_storage.n_rows, n_stored);
  approx_loglik_storage.resize(n_stored);
  prior_storage.resize(n_stored);
}

// run approximate MCMC for
// non-linear and/or non-Gaussian state space model
template void ung_amcmc::approx_mcmc(ung_ssm model, const bool end_ram,
  const bool local_approx, const arma::vec& initial_mode,
  const unsigned int max_iter, const double conv_tol);
template void ung_amcmc::approx_mcmc(ung_bsm model, const bool end_ram,
  const bool local_approx, const arma::vec& initial_mode,
  const unsigned int max_iter, const double conv_tol);
template void ung_amcmc::approx_mcmc(ung_svm model, const bool end_ram,
  const bool local_approx, const arma::vec& initial_mode,
  const unsigned int max_iter, const double conv_tol);

template<class T>
void ung_amcmc::approx_mcmc(T model, const bool end_ram, const bool local_approx,
  const arma::vec& initial_mode, const unsigned int max_iter, const double conv_tol) {
  
  unsigned int m = model.m;
  unsigned n = model.n;
  
  // get the current values of theta
  arma::vec theta = model.get_theta();
  // compute the log[p(theta)]
  double logprior = log_prior_pdf(theta);
  
  // construct the approximate Gaussian model
  arma::vec mode_estimate = initial_mode;
  ugg_ssm approx_model = model.approximate(mode_estimate, max_iter, conv_tol);
  
  // compute the log-likelihood of the approximate model
  double gaussian_loglik = approx_model.log_likelihood();
  
  // compute unnormalized mode-based correction terms
  // log[g(y_t | ^alpha_t) / ~g(y_t | ^alpha_t)]
  arma::vec scales = model.scaling_factors(approx_model, mode_estimate);
  double sum_scales = arma::accu(scales);
  // compute the constant term
  double const_term = compute_const_term(model, approx_model);
  // log-likelihood approximation
  double approx_loglik = gaussian_loglik + const_term + sum_scales;
  
  double acceptance_prob = 0.0;
  unsigned int counts = 0;
  std::normal_distribution<> normal(0.0, 1.0);
  std::uniform_real_distribution<> unif(0.0, 1.0);
  arma::vec scales_prop = scales;
  arma::vec approx_y = approx_model.y;
  arma::vec approx_H = approx_model.H;
  for (unsigned int i = 1; i <= n_iter; i++) {
    
    if (i % 16 == 0) {
      Rcpp::checkUserInterrupt();
    }
    
    // sample from standard normal distribution
    arma::vec u(n_par);
    for(unsigned int j = 0; j < n_par; j++) {
      u(j) = normal(model.engine);
    }
    
    // propose new theta
    arma::vec theta_prop = theta + S * u;
    // compute prior
    double logprior_prop = log_prior_pdf(theta_prop);
    
    if (logprior_prop > -arma::datum::inf) {
      // update parameters
      model.set_theta(theta_prop);
      
      if (local_approx) {
        // construct the approximate Gaussian model
        mode_estimate = initial_mode;
        model.approximate(approx_model, mode_estimate, max_iter, conv_tol);
        // compute unnormalized mode-based correction terms
        // log[g(y_t | ^alpha_t) / ~g(y_t | ^alpha_t)]
        scales_prop = model.scaling_factors(approx_model, mode_estimate);
        sum_scales = arma::accu(scales_prop);
        // compute the constant term
        const_term = compute_const_term(model, approx_model);
      } else {
        model.approximate(approx_model, mode_estimate, 0, conv_tol);
      }
      // compute the log-likelihood of the approximate model
      gaussian_loglik = approx_model.log_likelihood();
      double approx_loglik_prop = gaussian_loglik + const_term + sum_scales;
      
      acceptance_prob = std::min(1.0, exp(approx_loglik_prop - approx_loglik +
        logprior_prop - logprior));
      
      if (unif(model.engine) < acceptance_prob) {
        if (i > n_burnin) acceptance_rate++;
        approx_loglik = approx_loglik_prop;
        logprior = logprior_prop;
        theta = theta_prop;
        scales = scales_prop;
        approx_y = approx_model.y;
        approx_H = approx_model.H;
        counts = 0;
      }
    } else acceptance_prob = 0.0;
    
    if (i > n_burnin) {
      counts++;
      if ((i - n_burnin - 1) % n_thin == 0) {
        if (counts <= n_thin) {
          approx_loglik_storage(n_stored) = approx_loglik;
          theta_storage.col(n_stored) = theta;
          y_storage.col(n_stored) = approx_y;
          H_storage.col(n_stored) = approx_H;
          prior_storage(n_stored) = logprior;
          scales_storage.col(n_stored) = scales;
          count_storage(n_stored) = 1;
          n_stored++;
        } else {
          count_storage(n_stored - 1)++;
        }
      }
    }
    
    if (!end_ram || i <= n_burnin) {
      ramcmc::adapt_S(S, u, acceptance_prob, target_acceptance, i, gamma);
    }
  }
  
  trim_storage();
  acceptance_rate /= (n_iter - n_burnin);
}


template void ung_amcmc::is_correction_psi(ung_ssm model, const unsigned int nsim_states, 
  const bool const_sim, const unsigned int n_threads);
template void ung_amcmc::is_correction_psi(ung_bsm model, const unsigned int nsim_states, 
  const bool const_sim, const unsigned int n_threads);
template void ung_amcmc::is_correction_psi(ung_svm model, const unsigned int nsim_states, 
  const bool const_sim, const unsigned int n_threads);

template <class T>
void ung_amcmc::is_correction_psi(T model, const unsigned int nsim_states, 
  const bool const_sim, const unsigned int n_threads) {
  
  if(n_threads > 1) {
#ifdef _OPENMP
#pragma omp parallel num_threads(n_threads) default(none) firstprivate(model)
{
  model.engine = std::mt19937(omp_get_thread_num() + 1);
  unsigned thread_size = floor(n_stored / n_threads);
  unsigned int start = omp_get_thread_num() * thread_size;
  unsigned int end = (omp_get_thread_num() + 1) * thread_size - 1;
  if(omp_get_thread_num() == (n_threads - 1)) {
    end = n_stored - 1;
  }
  
  arma::mat theta_piece = theta_storage(arma::span::all, arma::span(start, end));
  arma::cube alpha_piece(model.n, model.m, thread_size);
  arma::vec weights_piece(thread_size);
  arma::mat y_piece = y_storage(arma::span::all, arma::span(start, end));
  arma::mat H_piece = H_storage(arma::span::all, arma::span(start, end));
  arma::mat scales_piece = scales_storage(arma::span::all, arma::span(start, end));
  if (const_sim) {
    state_sampler_psi_is2(model, nsim_states, theta_piece, alpha_piece, weights_piece,
      y_piece, H_piece, scales_piece);
  } else {
    arma::uvec count_piece = count_storage(arma::span(start, end));
    state_sampler_psi_is1(model, nsim_states, theta_piece, alpha_piece, weights_piece,
      y_piece, H_piece, scales_piece, count_piece);
  }
  alpha_storage.slices(start, end) = alpha_piece;
  weight_storage.subvec(start, end) = weights_piece;
}
#else
    if (const_sim) {
      state_sampler_psi_is2(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage);
    } else {
      state_sampler_psi_is1(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage, count_storage);
    }
    
#endif
  } else {
    if (const_sim) {
      state_sampler_psi_is2(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage);
    } else {
      state_sampler_psi_is1(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage, count_storage);
    }
  }
  posterior_storage = prior_storage + approx_loglik_storage + 
    log(weight_storage);
}

template void ung_amcmc::state_sampler_psi_is2(ung_ssm model, 
  const unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::mat& scales);
template void ung_amcmc::state_sampler_psi_is2(ung_bsm model, 
  const unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::mat& scales);
template void ung_amcmc::state_sampler_psi_is2(ung_svm model, 
  const unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::mat& scales);

template <class T>
void ung_amcmc::state_sampler_psi_is2(T model, const unsigned int nsim_states, 
  const arma::mat& theta, arma::cube& alpha, arma::vec& weights, 
  const arma::mat& y, const arma::mat& H, const arma::mat& scales) {
  
  arma::vec tmp(1);
  ugg_ssm approx_model = model.approximate(tmp, 0, 0);
  for (unsigned int i = 0; i < theta.n_cols; i++) {
    
    arma::vec theta_i = theta.col(i);
    model.set_theta(theta_i);
    model.approximate(approx_model, tmp, 0, 0);
    approx_model.y = y.col(i);
    approx_model.H = H.col(i);
    approx_model.compute_HH();
    
    arma::cube alpha_i(model.m, model.n, nsim_states);
    arma::mat weights_i(nsim_states, model.n);
    arma::umat indices(nsim_states, model.n - 1);
    weights(i) = exp(model.psi_filter(approx_model, 0, scales.col(i),
      nsim_states, alpha_i, weights_i, indices));
    filter_smoother(alpha_i, indices);
    arma::vec w = weights_i.col(model.n - 1);
    std::discrete_distribution<> sample(w.begin(), w.end());
    alpha.slice(i) = alpha_i.slice(sample(model.engine)).t();
  }
}


template void ung_amcmc::state_sampler_psi_is1(ung_ssm model, 
  const unsigned int nsim_states, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::mat& scales, const arma::uvec& counts);
template void ung_amcmc::state_sampler_psi_is1(ung_bsm model, 
  const unsigned int nsim_states, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::mat& scales, const arma::uvec& counts);
template void ung_amcmc::state_sampler_psi_is1(ung_svm model, 
  const unsigned int nsim_states, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::mat& scales, const arma::uvec& counts);

template <class T>
void ung_amcmc::state_sampler_psi_is1(T model, const unsigned int nsim_states, 
  const arma::mat& theta, arma::cube& alpha, arma::vec& weights, 
  const arma::mat& y, const arma::mat& H, const arma::mat& scales, 
  const arma::uvec& counts) {
  
  arma::vec tmp(1);
  ugg_ssm approx_model = model.approximate(tmp, 0, 0);
  for (unsigned int i = 0; i < theta.n_cols; i++) {
    
    arma::vec theta_i = theta.col(i);
    model.set_theta(theta_i);
    model.approximate(approx_model, tmp, 0, 0);
    approx_model.y = y.col(i);
    approx_model.H = H.col(i);
    approx_model.compute_HH();
    unsigned int m = nsim_states * counts(i);
    arma::cube alpha_i(model.m, model.n, m);
    arma::mat weights_i(m, model.n);
    arma::umat indices(m, model.n - 1);
    weights(i) = exp(model.psi_filter(approx_model, 0, scales.col(i), m, 
      alpha_i, weights_i, indices));
    filter_smoother(alpha_i, indices);
    arma::vec w = weights_i.col(model.n - 1);
    std::discrete_distribution<> sample(w.begin(), w.end());
    alpha.slice(i) = alpha_i.slice(sample(model.engine)).t();
  }
}


template void ung_amcmc::is_correction_bsf(ung_ssm model, 
  const unsigned int nsim_states, const bool const_sim, 
  const unsigned int n_threads);
template void ung_amcmc::is_correction_bsf(ung_bsm model, 
  const unsigned int nsim_states, const bool const_sim, 
  const unsigned int n_threads);
template void ung_amcmc::is_correction_bsf(ung_svm model, 
  const unsigned int nsim_states, const bool const_sim, 
  const unsigned int n_threads);

template <class T>
void ung_amcmc::is_correction_bsf(T model, const unsigned int nsim_states, 
  const bool const_sim, const unsigned int n_threads) {
  
  if(n_threads > 1) {
#ifdef _OPENMP
#pragma omp parallel num_threads(n_threads) default(none) firstprivate(model)
{
  model.engine = std::mt19937(omp_get_thread_num() + 1);
  unsigned thread_size = floor(n_stored / n_threads);
  unsigned int start = omp_get_thread_num() * thread_size;
  unsigned int end = (omp_get_thread_num() + 1) * thread_size - 1;
  if(omp_get_thread_num() == (n_threads - 1)) {
    end = n_stored - 1;
  }
  
  arma::mat theta_piece = theta_storage(arma::span::all, arma::span(start, end));
  arma::cube alpha_piece(model.n, model.m, thread_size);
  arma::vec weights_piece(thread_size);
  arma::vec approx_loglik_piece = approx_loglik_storage.subvec(start, end);
  if (const_sim) {
    state_sampler_bsf_is2(model, nsim_states, theta_piece, approx_loglik_piece, 
      alpha_piece, weights_piece);
  } else {
    arma::uvec count_piece = count_storage(arma::span(start, end));
    state_sampler_bsf_is1(model, nsim_states, theta_piece, approx_loglik_piece, 
      alpha_piece, weights_piece, count_piece);
  }
  alpha_storage.slices(start, end) = alpha_piece;
  weight_storage.subvec(start, end) = weights_piece;
  approx_loglik_storage.subvec(start, end) = approx_loglik_piece;
}
#else
    if (const_sim) {
      state_sampler_bsf_is2(model, nsim_states, approx_loglik_storage, theta_storage, 
        alpha_storage, weight_storage);
    } else {
      state_sampler_bsf_is1(model, nsim_states, approx_loglik_storage, theta_storage, 
        alpha_storage, weight_storage, count_storage);
    }
#endif
  } else {
    if (const_sim) {
      state_sampler_bsf_is2(model, nsim_states, approx_loglik_storage, theta_storage, 
        alpha_storage, weight_storage);
    } else {
      state_sampler_bsf_is1(model, nsim_states, approx_loglik_storage, theta_storage, 
        alpha_storage, weight_storage, count_storage);
    }
  }
  posterior_storage = prior_storage + log(weight_storage);
}

template void ung_amcmc::state_sampler_bsf_is2(ung_ssm model, unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights);
template void ung_amcmc::state_sampler_bsf_is2(ung_bsm model, unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights);
template void ung_amcmc::state_sampler_bsf_is2(ung_svm model, unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights);

template <class T>
void ung_amcmc::state_sampler_bsf_is2(T model, const unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights) {
  
  for (unsigned int i = 0; i < theta.n_cols; i++) {
    
    arma::vec theta_i = theta.col(i);
    model.set_theta(theta_i);
    
    arma::cube alpha_i(model.m, model.n, nsim_states);
    arma::mat weights_i(nsim_states, model.n);
    arma::umat indices(nsim_states, model.n - 1);
    double loglik = model.bsf_filter(nsim_states, alpha_i, weights_i, indices);
    weights(i) = exp(loglik - approx_loglik_storage(i));
    filter_smoother(alpha_i, indices);
    arma::vec w = weights_i.col(model.n - 1);
    std::discrete_distribution<> sample(w.begin(), w.end());
    alpha.slice(i) = alpha_i.slice(sample(model.engine)).t();
  }
}



template void ung_amcmc::state_sampler_bsf_is1(ung_ssm model, unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::uvec& counts);
template void ung_amcmc::state_sampler_bsf_is1(ung_bsm model, unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::uvec& counts);
template void ung_amcmc::state_sampler_bsf_is1(ung_svm model, unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::uvec& counts);

template <class T>
void ung_amcmc::state_sampler_bsf_is1(T model, const unsigned int nsim_states, 
  const arma::vec& approx_loglik_storage, const arma::mat& theta,
  arma::cube& alpha, arma::vec& weights, const arma::uvec& counts) {
  
  for (unsigned int i = 0; i < theta.n_cols; i++) {
    
    arma::vec theta_i = theta.col(i);
    model.set_theta(theta_i);
    
    unsigned int m = nsim_states * counts(i);
    arma::cube alpha_i(model.m, model.n, m);
    arma::mat weights_i(m, model.n);
    arma::umat indices(m, model.n - 1);
    double loglik = model.bsf_filter(m, alpha_i, weights_i, indices);
    weights(i) = exp(loglik - approx_loglik_storage(i));
    filter_smoother(alpha_i, indices);
    arma::vec w = weights_i.col(model.n - 1);
    std::discrete_distribution<> sample(w.begin(), w.end());
    alpha.slice(i) = alpha_i.slice(sample(model.engine)).t();
  }
}

template void ung_amcmc::is_correction_spdk(ung_ssm model, unsigned int nsim_states, 
  bool const_sim, unsigned int n_threads);
template void ung_amcmc::is_correction_spdk(ung_bsm model, unsigned int nsim_states, 
  bool const_sim, unsigned int n_threads);
template void ung_amcmc::is_correction_spdk(ung_svm model, unsigned int nsim_states, 
  bool const_sim, unsigned int n_threads);

template <class T>
void ung_amcmc::is_correction_spdk(T model, const unsigned int nsim_states, 
  const bool const_sim, const unsigned int n_threads) {
  
  if(n_threads > 1) {
#ifdef _OPENMP
#pragma omp parallel num_threads(n_threads) default(none) firstprivate(model)
{
  model.engine = std::mt19937(omp_get_thread_num() + 1);
  unsigned thread_size = floor(n_stored / n_threads);
  unsigned int start = omp_get_thread_num() * thread_size;
  unsigned int end = (omp_get_thread_num() + 1) * thread_size - 1;
  if(omp_get_thread_num() == (n_threads - 1)) {
    end = n_stored - 1;
  }
  
  arma::mat theta_piece = theta_storage(arma::span::all, arma::span(start, end));
  arma::cube alpha_piece(model.n, model.m, thread_size);
  arma::vec weights_piece(thread_size);
  arma::mat y_piece = y_storage(arma::span::all, arma::span(start, end));
  arma::mat H_piece = H_storage(arma::span::all, arma::span(start, end));
  arma::vec scales_piece = arma::sum(scales_storage(arma::span::all, 
    arma::span(start, end)));
  if (const_sim) {
    state_sampler_spdk_is2(model, nsim_states, theta_piece, alpha_piece, weights_piece,
      y_piece, H_piece, scales_piece);
  } else {
    arma::uvec count_piece = count_storage(arma::span(start, end));
    state_sampler_spdk_is1(model, nsim_states, theta_piece, alpha_piece, weights_piece,
      y_piece, H_piece, scales_piece, count_piece);
  }
  alpha_storage.slices(start, end) = alpha_piece;
  weight_storage.subvec(start, end) = weights_piece;
}
#else
    if (const_sim) {
      state_sampler_spdk_is2(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage);
    } else {
      state_sampler_spdk_is1(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage, count_storage);
    }
#endif
  } else {
    if (const_sim) {
      state_sampler_spdk_is2(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage);
    } else {
      state_sampler_spdk_is1(model, nsim_states, theta_storage, alpha_storage, weight_storage,
        y_storage, H_storage, scales_storage, count_storage);
    }
  }
  posterior_storage = prior_storage + approx_loglik_storage + 
    log(weight_storage);
}

template void ung_amcmc::state_sampler_spdk_is2(ung_ssm model, 
  unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::vec& scales);
template void ung_amcmc::state_sampler_spdk_is2(ung_bsm model, 
  unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::vec& scales);
template void ung_amcmc::state_sampler_spdk_is2(ung_svm model, 
  unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::vec& scales);

template <class T>
void ung_amcmc::state_sampler_spdk_is2(T model, const unsigned int nsim_states, 
  const arma::mat& theta, arma::cube& alpha, arma::vec& weights, 
  const arma::mat& y, const arma::mat& H, const arma::vec& scales) {
  
  arma::vec tmp(1);
  ugg_ssm approx_model = model.approximate(tmp, 0, 0);
  for (unsigned int i = 0; i < theta.n_cols; i++) {
    
    arma::vec theta_i = theta.col(i);
    model.set_theta(theta_i);
    model.approximate(approx_model, tmp, 0, 0);
    approx_model.y = y.col(i);
    approx_model.H = H.col(i);
    approx_model.compute_HH();
    
    arma::cube alpha_i = approx_model.simulate_states(nsim_states, true);
    
    arma::vec weights_i(nsim_states, arma::fill::zeros);
    for (unsigned int t = 0; t < model.n; t++) {
      weights_i += model.log_weights(approx_model, t, alpha_i);
    }
    weights_i = exp(weights_i - scales(i));
    weights(i) = arma::mean(weights_i);
    std::discrete_distribution<> sample(weights_i.begin(), weights_i.end());
    alpha.slice(i) = alpha_i.slice(sample(model.engine)).t();
  }
}



template void ung_amcmc::state_sampler_spdk_is1(ung_ssm model,
  unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::vec& scales, const arma::uvec& counts);
template void ung_amcmc::state_sampler_spdk_is1(ung_bsm model, 
  unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::vec& scales, const arma::uvec& counts);
template void ung_amcmc::state_sampler_spdk_is1(ung_svm model, 
  unsigned int nsim_states, const arma::mat& theta, arma::cube& alpha, 
  arma::vec& weights, const arma::mat& y, const arma::mat& H,
  const arma::vec& scales, const arma::uvec& counts);

template <class T>
void ung_amcmc::state_sampler_spdk_is1(T model, const unsigned int nsim_states, 
  const arma::mat& theta, arma::cube& alpha, arma::vec& weights, 
  const arma::mat& y, const arma::mat& H, const arma::vec& scales, 
  const arma::uvec& counts) {
  
  arma::vec tmp(1);
  ugg_ssm approx_model = model.approximate(tmp, 0, 0);
  for (unsigned int i = 0; i < theta.n_cols; i++) {
    
    arma::vec theta_i = theta.col(i);
    model.set_theta(theta_i);
    model.approximate(approx_model, tmp, 0, 0);
    approx_model.y = y.col(i);
    approx_model.H = H.col(i);
    approx_model.compute_HH();
    
    unsigned int m = nsim_states * counts(i);
    arma::cube alpha_i = approx_model.simulate_states(m, true);
    
    arma::vec weights_i(m, arma::fill::zeros);
    for (unsigned int t = 0; t < model.n; t++) {
      weights_i += model.log_weights(approx_model, t, alpha_i);
    }
    weights_i = exp(weights_i - scales(i));
    weights(i) = arma::mean(weights_i);
    std::discrete_distribution<> sample(weights_i.begin(), weights_i.end());
    alpha.slice(i) = alpha_i.slice(sample(model.engine)).t();
  }
}
