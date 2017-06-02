#include "mgg_ssm.h"
#include "ugg_ssm.h"
#include "ung_ssm.h"
#include "ugg_bsm.h"
#include "ung_bsm.h"
#include "ung_svm.h"
#include "nlg_ssm.h"

#include "filter_smoother.h"
#include "summary.h"

// [[Rcpp::export]]
Rcpp::List aux_nlg(const arma::mat& y, SEXP Z_fn_, SEXP H_fn_, 
  SEXP T_fn_, SEXP R_fn_, SEXP Z_gn_, SEXP T_gn_, SEXP a1_fn_, SEXP P1_fn_, 
  const arma::vec& theta, SEXP log_prior_pdf_, const arma::vec& known_params, 
  const arma::mat& known_tv_params, const unsigned int n_states, 
  const unsigned int n_etas,  const arma::uvec& time_varying,
  const arma::uvec& state_varying, const unsigned int nsim_states, 
  const unsigned int seed) {
  
  nlg_ssm model(y, Z_fn_, H_fn_, T_fn_, R_fn_, Z_gn_, T_gn_, a1_fn_, P1_fn_, 
    theta, log_prior_pdf_, known_params, known_tv_params, n_states, n_etas,
    time_varying, state_varying, seed);
  
  unsigned int m = model.m;
  unsigned n = model.n;
  
  arma::cube alpha(m, n, nsim_states);
  arma::mat weights(nsim_states, n);
  arma::umat indices(nsim_states, n - 1);
  double loglik;
  
    loglik = model.aux_filter(nsim_states, alpha, weights, indices);
  
  
  arma::mat at(m, n);
  arma::mat att(m, n);
  arma::cube Pt(m, m, n);
  arma::cube Ptt(m, m, n);
  filter_summary(alpha, at, att, Pt, Ptt, weights);
  
  arma::inplace_trans(att);
  return Rcpp::List::create(
    Rcpp::Named("att") = att, 
    Rcpp::Named("Ptt") = Ptt, 
    Rcpp::Named("weights") = weights,
    Rcpp::Named("logLik") = loglik, Rcpp::Named("alpha") = alpha);
}

// [[Rcpp::export]]
Rcpp::List aux_smoother_nlg(const arma::mat& y, SEXP Z_fn_, SEXP H_fn_, 
  SEXP T_fn_, SEXP R_fn_, SEXP Z_gn_, SEXP T_gn_, SEXP a1_fn_, SEXP P1_fn_, 
  const arma::vec& theta, SEXP log_prior_pdf_, const arma::vec& known_params, 
  const arma::mat& known_tv_params, const unsigned int n_states, 
  const unsigned int n_etas,  const arma::uvec& time_varying,
  const arma::uvec& state_varying, const unsigned int nsim_states, 
  const unsigned int seed) {
  
  
  nlg_ssm model(y, Z_fn_, H_fn_, T_fn_, R_fn_, Z_gn_, T_gn_, a1_fn_, P1_fn_, 
    theta, log_prior_pdf_, known_params, known_tv_params, n_states, n_etas,
    time_varying, state_varying, seed);
  
  unsigned int m = model.m;
  unsigned n = model.n;
  
  arma::cube alpha(m, n, nsim_states);
  arma::mat weights(nsim_states, n);
  arma::umat indices(nsim_states, n - 1);
  double loglik;
 
  loglik = model.aux_filter(nsim_states, alpha, weights, indices);

  
  arma::mat alphahat(model.m, model.n);
  arma::cube Vt(model.m, model.m, model.n);
  
  //  if (smoothing_type == 1) {
  filter_smoother(alpha, indices);
  running_weighted_summary(alpha, alphahat, Vt, weights.col(model.n - 1));
  /*} else {
  Rcpp::stop("Forward-backward smoothing with psi-filter is not yet implemented.");
}*/
  arma::inplace_trans(alphahat);
  
  return Rcpp::List::create(
    Rcpp::Named("alphahat") = alphahat, Rcpp::Named("Vt") = Vt, 
    Rcpp::Named("weights") = weights,
    Rcpp::Named("logLik") = loglik, Rcpp::Named("alpha") = alpha);
  }


// [[Rcpp::export]]
Rcpp::List aux(const Rcpp::List& model_,
  const unsigned int nsim_states, const unsigned int seed, 
  bool gaussian, const int model_type, bool optimal) {
  
  if (gaussian) {
    switch (model_type) {
    case 1: {
  ugg_ssm model(clone(model_), seed);
  unsigned int m = model.m;
  unsigned n = model.n;
  
  arma::cube alpha(m, n, nsim_states);
  arma::mat weights(nsim_states, n);
  arma::umat indices(nsim_states, n - 1);
  double loglik;
  if (optimal) {
    loglik = model.oaux_filter(nsim_states, alpha, weights, indices);
    
  } else {
    loglik = model.aux_filter(nsim_states, alpha, weights, indices);
  }
  arma::mat at(m, n);
  arma::mat att(m, n);
  arma::cube Pt(m, m, n);
  arma::cube Ptt(m, m, n);
  filter_summary(alpha, at, att, Pt, Ptt, weights);
  
  arma::inplace_trans(att);
  return Rcpp::List::create(
    Rcpp::Named("att") = att, 
    Rcpp::Named("Ptt") = Ptt, 
    Rcpp::Named("weights") = weights,
    Rcpp::Named("logLik") = loglik, Rcpp::Named("alpha") = alpha);
} break;
    case 2: {
      ugg_bsm model(clone(model_), seed);
      unsigned int m = model.m;
      unsigned n = model.n;
      
      arma::cube alpha(m, n, nsim_states);
      arma::mat weights(nsim_states, n);
      arma::umat indices(nsim_states, n - 1);
      double loglik;
      if (optimal) {
        loglik = model.oaux_filter(nsim_states, alpha, weights, indices);
        
      } else {
        loglik = model.aux_filter(nsim_states, alpha, weights, indices);
      }
      
      arma::mat at(m, n);
      arma::mat att(m, n);
      arma::cube Pt(m, m, n);
      arma::cube Ptt(m, m, n);
      filter_summary(alpha, at, att, Pt, Ptt, weights);
      
      arma::inplace_trans(att);
      return Rcpp::List::create(
        Rcpp::Named("att") = att, 
        Rcpp::Named("Ptt") = Ptt, 
        Rcpp::Named("weights") = weights,
        Rcpp::Named("logLik") = loglik, Rcpp::Named("alpha") = alpha);
    } break;
    }
  } 
  return Rcpp::List::create(Rcpp::Named("error") = 0);
}

// [[Rcpp::export]]
Rcpp::List aux_smoother(const Rcpp::List& model_,
  const unsigned int nsim_states, const unsigned int seed, 
  bool gaussian, const int model_type, bool optimal) {
  
  if (gaussian) {
    switch (model_type) {
    case 1: {
  ugg_ssm model(clone(model_), seed);
  unsigned int m = model.m;
  unsigned n = model.n;
  
  arma::cube alpha(m, n, nsim_states);
  arma::mat weights(nsim_states, n);
  arma::umat indices(nsim_states, n - 1);
  double loglik;
  if (optimal) {
    loglik = model.oaux_filter(nsim_states, alpha, weights, indices);
  } else {
    loglik = model.aux_filter(nsim_states, alpha, weights, indices);
  }
  arma::mat alphahat(model.m, model.n);
  arma::cube Vt(model.m, model.m, model.n);
  
  filter_smoother(alpha, indices);
  running_weighted_summary(alpha, alphahat, Vt, weights.col(model.n - 1));
  
  arma::inplace_trans(alphahat);
  
  return Rcpp::List::create(
    Rcpp::Named("alphahat") = alphahat, Rcpp::Named("Vt") = Vt, 
    Rcpp::Named("weights") = weights,
    Rcpp::Named("logLik") = loglik, Rcpp::Named("alpha") = alpha);
  
} break;
    case 2: {
      ugg_bsm model(clone(model_), seed);
      unsigned int m = model.m;
      unsigned n = model.n;
      
      arma::cube alpha(m, n, nsim_states);
      arma::mat weights(nsim_states, n);
      arma::umat indices(nsim_states, n - 1);
      double loglik;
      if (optimal) {
        loglik = model.oaux_filter(nsim_states, alpha, weights, indices);
        
      } else {
        loglik = model.aux_filter(nsim_states, alpha, weights, indices);
      }
      
      arma::mat alphahat(model.m, model.n);
      arma::cube Vt(model.m, model.m, model.n);
      
      filter_smoother(alpha, indices);
      running_weighted_summary(alpha, alphahat, Vt, weights.col(model.n - 1));
      
      arma::inplace_trans(alphahat);
      
      return Rcpp::List::create(
        Rcpp::Named("alphahat") = alphahat, Rcpp::Named("Vt") = Vt, 
        Rcpp::Named("weights") = weights,
        Rcpp::Named("logLik") = loglik, Rcpp::Named("alpha") = alpha);
    } break;
    }
  } 
  return Rcpp::List::create(Rcpp::Named("error") = 0);
}


