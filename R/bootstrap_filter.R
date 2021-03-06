#' Bootstrap Filtering
#'
#' Function \code{bootstrap_filter} performs a bootstrap filtering with stratification
#' resampling.
#' @param model of class \code{bsm_lg}, \code{bsm_ng} or \code{svm}.
#' @param nsim Number of samples.
#' @param seed Seed for RNG.
#' @param ... Ignored.
#' @return A list containing samples, weights from the last time point, and an
#' estimate of log-likelihood.
#' @export
#' @references 
#' Gordon, N. J., Salmond, D. J., & Smith, A. F. M. (1993). 
#' Novel approach to nonlinear/non-Gaussian Bayesian state estimation. IEE Proceedings-F, 140, 107–113.
#' @rdname bootstrap_filter
bootstrap_filter <- function(model, nsim, ...) {
  UseMethod("bootstrap_filter", model)
}
#' @method bootstrap_filter gaussian
#' @rdname bootstrap_filter
#' @export
#' @examples 
#' set.seed(1)
#' x <- cumsum(rnorm(50))
#' y <- rnorm(50, x, 0.5) 
#' model <- bsm_lg(y, sd_y = 0.5, sd_level = 1, P1 = 1)
#'   
#' out <- bootstrap_filter(model, nsim = 1000)
#' ts.plot(cbind(y, x, out$att), col = 1:3)
#' ts.plot(cbind(kfilter(model)$att, out$att), col = 1:3)
#' 
bootstrap_filter.gaussian <- function(model, nsim,
  seed = sample(.Machine$integer.max, size = 1), ...) {

  out <- bsf(model, nsim, seed, TRUE, model_type(model))
  colnames(out$at) <- colnames(out$att) <- colnames(out$Pt) <-
    colnames(out$Ptt) <- rownames(out$Pt) <- rownames(out$Ptt) <- names(model$a1)
  out$at <- ts(out$at, start = start(model$y), frequency = frequency(model$y))
  out$att <- ts(out$att, start = start(model$y), frequency = frequency(model$y))
  rownames(out$alpha) <- names(model$a1)
  out$alpha <- aperm(out$alpha, c(2, 1, 3))
  out
}

#' @method bootstrap_filter nongaussian
#' @rdname bootstrap_filter
#' @export
#' @examples 
#' data("poisson_series")
#' model <- bsm_ng(poisson_series, sd_level = 0.1, sd_slope = 0.01, 
#'   P1 = diag(1, 2), distribution = "poisson")
#'   
#' out <- bootstrap_filter(model, nsim = 100)
#' ts.plot(cbind(poisson_series, exp(out$att[, 1])), col = 1:2)
#' 
bootstrap_filter.nongaussian <- function(model, nsim,
  seed = sample(.Machine$integer.max, size = 1), ...) {

  model$distribution <- 
    pmatch(model$distribution, 
      c("svm", "poisson", "binomial", "negative binomial", "gamma", "gaussian"),
      duplicates.ok = TRUE) - 1

  out <- bsf(model, nsim, seed, FALSE, 1L)
  colnames(out$at) <- colnames(out$att) <- colnames(out$Pt) <-
    colnames(out$Ptt) <- rownames(out$Pt) <- rownames(out$Ptt) <- names(model$a1)
  out$at <- ts(out$at, start = start(model$y), frequency = frequency(model$y))
  out$att <- ts(out$att, start = start(model$y), frequency = frequency(model$y))
  rownames(out$alpha) <- names(model$a1)
  out$alpha <- aperm(out$alpha, c(2, 1, 3))
  out
}
#' @method bootstrap_filter ssm_nlg
#' @rdname bootstrap_filter
#' @export
bootstrap_filter.ssm_nlg <- function(model, nsim,
  seed = sample(.Machine$integer.max, size = 1), ...) {

  out <- bsf_nlg(t(model$y), model$Z, model$H, model$T,
    model$R, model$Z_gn, model$T_gn, model$a1, model$P1,
    model$theta, model$log_prior_pdf, model$known_params,
    model$known_tv_params, model$n_states, model$n_etas,
    as.integer(model$time_varying), nsim, seed, 
    default_update_fn, default_prior_fn)
  colnames(out$at) <- colnames(out$att) <- colnames(out$Pt) <-
    colnames(out$Ptt) <- rownames(out$Pt) <- rownames(out$Ptt) <-
    rownames(out$alpha) <- model$state_names
  out$at <- ts(out$at, start = start(model$y), frequency = frequency(model$y))
  out$att <- ts(out$att, start = start(model$y), frequency = frequency(model$y))
  out$alpha <- aperm(out$alpha, c(2, 1, 3))
  out
}

#' @method bootstrap_filter ssm_sde
#' @rdname bootstrap_filter
#' @param L Integer defining the discretization level for SDE models.
#' @export
bootstrap_filter.ssm_sde <- function(model, nsim, L,
  seed = sample(.Machine$integer.max, size = 1), ...) {
  if(L < 1) stop("Discretization level L must be larger than 0.")
  out <- bsf_sde(model$y, model$x0, model$positive,
    model$drift, model$diffusion, model$ddiffusion,
    model$prior_pdf, model$obs_pdf, model$theta,
    nsim, round(L), seed)
  colnames(out$at) <- colnames(out$att) <- colnames(out$Pt) <-
    colnames(out$Ptt) <- rownames(out$Pt) <- rownames(out$Ptt) <-
    rownames(out$alpha) <- model$state_names
  out$at <- ts(out$at, start = start(model$y), frequency = frequency(model$y))
  out$att <- ts(out$att, start = start(model$y), frequency = frequency(model$y))
  out$alpha <- aperm(out$alpha, c(2, 1, 3))
  out
}
