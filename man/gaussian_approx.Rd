% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/generics.R, R/ng_bsm.R, R/ngssm.R, R/svm.R
\name{gaussian_approx}
\alias{gaussian_approx}
\alias{gaussian_approx.ng_bsm}
\alias{gaussian_approx.ngssm}
\alias{gaussian_approx.svm}
\title{Gaussian Approximation of non-Gaussian State Space Model}
\usage{
gaussian_approx(object, max_iter, conv_tol, ...)

\method{gaussian_approx}{ng_bsm}(object, max_iter = 100, conv_tol = 1e-08,
  ...)

\method{gaussian_approx}{ngssm}(object, max_iter = 100, conv_tol = 1e-08,
  ...)

\method{gaussian_approx}{svm}(object, max_iter = 100, conv_tol = 1e-08, ...)
}
\arguments{
\item{object}{of class \code{svm}.}

\item{max_iter}{Maximum number of iterations.}

\item{conv_tol}{Tolerance parameter. Document properly later!}

\item{...}{Ignored.}
}
\description{
Returns the approximating Gaussian model.
}
