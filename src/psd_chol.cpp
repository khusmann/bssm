#include "psd_chol.h"

// [[Rcpp::export]]
arma::mat psd_chol(const arma::mat& x) {
  
  arma::uvec nonzero = 
    arma::find(x.diag() > std::max(arma::datum::eps, arma::datum::eps * x.n_cols * x.diag().max()));
  unsigned int k = nonzero.n_elem;
  
  arma::mat cholx(x.n_cols,x.n_cols, arma::fill::zeros);
  if (k > 0) {
    cholx.submat(nonzero, nonzero) = arma::chol(x.submat(nonzero, nonzero), "lower");
  }
  return cholx;
}