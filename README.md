[![Build Status](https://travis-ci.org/helske/bssm.png?branch=master)](https://travis-ci.org/helske/bssm)
[![cran version](http://www.r-pkg.org/badges/version/bssm)](http://cran.r-project.org/package=bssm)
[![downloads](http://cranlogs.r-pkg.org/badges/bssm)](http://cranlogs.r-pkg.org/badges/bssm)



bssm: an R package for Bayesian inference of state space models
==========================================================================

Efficient methods for Bayesian inference of state space models via particle Markov 
chain Monte Carlo and importance sampling type weighted Markov chain Monte Carlo. 
Currently Gaussian, Poisson, binomial, negative binomial, and Gamma observation densities 
and linear-Gaussian state dynamics, as well as general non-linear Gaussian models are supported.

For details, see [package vignettess at CRAN](https://cran.r-project.org/web/packages/bssm/index.html) and paper [Importance sampling type estimators based on approximate marginal Markov chain Monte Carlo](https://onlinelibrary.wiley.com/doi/abs/10.1111/sjos.12492). There are also couple posters related to IS-correction methodology: [SMC 2017 workshop: Accelerating MCMC with an approximation ](http://users.jyu.fi/~jovetale/posters/SMC2017) and [UseR!2017: Bayesian non-Gaussian state space models in R](http://users.jyu.fi/~jovetale/posters/user2017.pdf).


You can install the latest development version by using the devtools package:

```R
install.packages("devtools")
devtools::install_github("helske/bssm")
```

NEWS
==========================================================================

bssm 1.0.1 (Release date: 2020-11-11)
==============

  * Added an argument `future` for predict method which allows 
    predictions for current time points by supplying the original model 
    (e.g., for posterior predictive checks). 
    At the same time the argument name `future_model` was changed to `model`.
  * Fixed a bug in summary.mcmc_run which resulted error when 
    trying to obtain summary for states only.
  * Added a check for Kalman filter for a degenerate case where all 
    observational level and state level variances are zero.
  * Renamed argument `n_threads` to `threads` for consistency 
    with `iter` and `burnin` arguments.
  * Improved documentation, added examples.
  * Added a vignette regarding psi-APF for non-linear models.
  
bssm 1.0.0 (Release date: 2020-06-09)
==============
Major update

  * Major changes for model definitions, now model updating and priors 
    can be defined via R functions (non-linear and SDE models still rely on C++ snippets).
  * Added support for multivariate non-Gaussian models.
  * Added support for gamma distributions.
  * Added the function as.data.frame for mcmc output which converts the MCMC samples 
    to data.frame format for easier post-processing.
  * Added truncated normal prior.
  * Many argument names and model building functions have been changed for clarity and consistency.
  * Major overhaul of C++ internals which can bring minor efficiency gains and smaller installation size.
  * Allow zero as initial value for positive-constrained parameters of bsm models.
  * Small changes to summary method which can now return also only summaries of the states.
  * Fixed a bug in initializing run_mcmc for negative binomial model. 
  * Fixed a bug in phi-APF for non-linear models.
  * Reimplemented predict method which now always produces data frame of samples.
  
bssm 0.1.11 (Release date: 2020-02-25)
==============
  * Switched (back) to approximate posterior in RAM for PM-SPDK and PM-PSI, 
    as it seems to work better with noisy likelihood estimates.
  * Print and summary methods for MCMC output are now coherent in their output.
  
bssm 0.1.10 (Release date: 2020-02-04)
==============
  * Fixed missing weight update for IS-SPDK without OPENMP flag.
  * Removed unused usage argument ... from expand_sample.
  
bssm 0.1.9 (Release date: 2020-01-27)
==============
  * Fixed state sampling for PM-MCMC with SPDK.
  * Added ts attribute for svm model.
  * Corrected asymptotic variance for summary methods.
  
bssm 0.1.8-1 (Release date: 2019-12-20)
==============
  * Tweaked tests in order to pass MKL case at CRAN.

bssm 0.1.8 (Release date: 2019-09-23)
==============
  * Fixed a bug in predict method which prevented the method working in case of ngssm models.
  * Fixed a bug in predict method which threw an error due to dimension drop of models with single state.
  * Fixed issues with the vignette.

bssm 0.1.7 (Release date: 2019-03-19)
==============
  * Fixed a bug in EKF smoother which resulted wrong smoothed state estimates in 
    case of partially missing multivariate observations. Thanks for Santeri Karppinen for spotting the bug. 
  * Added twisted SMC based simulation smoothing algorithm for Gaussian models, as an alternative to 
    Kalman smoother based simulation.
  
bssm 0.1.6-1 (Release date: 2018-11-20)
==============
  * Fixed wrong dimension declarations in pseudo-marginal MCMC and logLik methods for SDE and ng_ar1 models.
  * Added a missing Jacobian for ng_bsm and bsm models using IS-correction.
  * Changed internal parameterization of ng_bsm and bsm models from log(1+theta) to log(theta).
  
bssm 0.1.5 (Release date: 2018-05-23)
==============
  * Fixed the Cholesky decomposition in filtering recursions of multivariate models.
  * as_gssm now works for multivariate Gaussian models of KFAS as well.
  * Fixed several issues regarding partially missing observations in multivariate models.
  * Added the MASS package to Suggests as it is used in some unit tests.
  * Added missing type argument to SDE MCMC call with delayed acceptance.
  
bssm 0.1.4-1 (Release date: 2018-02-04)
==============
  * Fixed the use of uninitialized values in psi-filter from version 0.1.3.

bssm 0.1.4 (Release date: 2018-02-04)
==============
  * MCMC output can now be defined with argument `type`. Instead of returning joint posterior 
    samples, run_mcmc can now return only marginal samples of theta, or summary statistics of 
    the states.
  * Due to the above change, argument `sim_states` was removed from the Gaussian MCMC methods.
  * MCMC functions are now less memory intensive, especially with `type="theta"`.


bssm 0.1.3 (Release date: 2018-01-07)
==============
  * Streamlined the output of the print method for MCMC results.
  * Fixed major bugs in predict method which caused wrong values for the prediction intervals.
  * Fixed some package dependencies.
  * Sampling for standard deviation parameters of BSM and their non-Gaussian counterparts 
    is now done in logarithmic scale for slightly increased efficiency.
  * Added a new model class ar1 for univariate (possibly noisy) Gaussian AR(1) processes.
  * MCMC output now includes posterior predictive distribution of states for one step ahead 
    to the future.
  
bssm 0.1.2 (Release date: 2017-11-21)
==============
  * API change for run_mcmc: All MCMC methods are now under the argument method, 
    instead of having separate arguments for delayed acceptance and IS schemes.
  * summary method for MCMC output now omits the computation of SE and ESS in order 
    to speed up the function.
  * Added new model class lgg_ssm, which is a linear-Gaussian model defined 
    directly via C++ like non-linear ssm_nlg models. This allows more flexible
    prior definitions and complex system matrix constructions.
  * Added another new model class, ssm_sde, which is a model with continuous 
    state dynamics defined as SDE. These too are defined via couple 
    simple C++ functions.
  * Added non-gaussian AR(1) model class.
  * Added argument nsim for predict method, which allows multiple draws per MCMC iteration.
  * The noise multiplier matrices H and R in ssm_nlg models can now depend on states.
  
bssm 0.1.1-1 (Release date: 2017-06-27)
==============
  * Use byte compiler.
  * Skip tests relying in certain numerical precision on CRAN.
  
bssm 0.1.1 (Release date: 2017-06-27)
==============
  
  * Switched from C++11 PRNGs to sitmo.
  * Fixed some portability issues in C++ codes.

bssm 0.1.0 (Release date: 2017-06-24)
==============

  * Initial release.
