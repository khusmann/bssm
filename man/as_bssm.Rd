% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/as_bssm.R
\name{as_bssm}
\alias{as_bssm}
\title{Convert KFAS Model to bssm Model}
\usage{
as_bssm(model, kappa = 100, ...)
}
\arguments{
\item{model}{Object of class \code{SSModel}.}

\item{kappa}{For \code{SSModel} object, a prior variance for initial state
used to replace exact diffuse elements of the original model.}

\item{...}{Additional arguments to model building functions of \code{bssm}
(such as prior and updating functions).}
}
\value{
Object of class \code{ssm_ulg}, \code{ssm_mlg}, \code{ssm_ung} or \code{ssm_mng}.
}
\description{
Converts \code{SSModel} object of \code{KFAS} package to general
\code{bssm} model of type \code{ssm_ulg}, \code{ssm_mlg}, \code{ssm_ung} or \code{ssm_mng}.
}
