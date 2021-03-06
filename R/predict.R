#' Predictions for State Space Models
#' 
#' Draw samples from the posterior predictive distribution for future time points 
#' given the posterior draws of hyperparameters \eqn{\theta} and \eqn{alpha_{n+1}}. 
#' Function can also be used to draw samples from the posterior predictive distribution
#' \eqn{p(\tilde y_1, \ldots, \tilde y_n | y_1,\ldots, y_n)}.
#'
#' @param object mcmc_output object obtained from 
#' \code{\link{run_mcmc}}
#' @param type Return predictions on \code{"mean"} 
#' \code{"response"}, or  \code{"state"} level. 
#' @param model Model for future observations. 
#' Should have same structure as the original model which was used in MCMC,
#' in order to plug the posterior samples of the model parameters to the right places. 
#' It is also possible to input the original model, which can be useful for example for 
#' posterior predictive checks. In this case, set argument \code{future} to \code{FALSE}.
#' @param nsim Number of samples to draw.
#' @param future Default is \code{TRUE}, in which case predictions are future. 
#' Otherwise it is assumed that \code{model} corresponds to the original model.
#' @param seed Seed for RNG.
#' @param ... Ignored.
#' @return Data frame of predicted samples.
#' @method predict mcmc_output
#' @rdname predict
#' @export
#' @examples
#' require("graphics")
#' y <- log10(JohnsonJohnson)
#' prior <- uniform(0.01, 0, 1)
#' model <- bsm_lg(window(y, end = c(1974, 4)), sd_y = prior,
#'   sd_level = prior, sd_slope = prior, sd_seasonal = prior)
#' 
#' mcmc_results <- run_mcmc(model, iter = 5000)
#' future_model <- model
#' future_model$y <- ts(rep(NA, 25), 
#'   start = tsp(model$y)[2] + 2 * deltat(model$y), 
#'   frequency = frequency(model$y))
#' # use "state" for illustrative purposes, we could use type = "mean" directly
#' pred <- predict(mcmc_results, future_model, type = "state", 
#'   nsim = 1000)
#' 
#' require("dplyr")
#' sumr_fit <- as.data.frame(mcmc_results, variable = "states") %>%
#'   group_by(time, iter) %>% 
#'   mutate(signal = 
#'       value[variable == "level"] + 
#'       value[variable == "seasonal_1"]) %>%
#'   group_by(time) %>%
#'   summarise(mean = mean(signal), 
#'     lwr = quantile(signal, 0.025), 
#'     upr = quantile(signal, 0.975))
#' 
#' sumr_pred <- pred %>% 
#'   group_by(time, sample) %>%
#'   mutate(signal = 
#'       value[variable == "level"] + 
#'       value[variable == "seasonal_1"]) %>%
#'   group_by(time) %>%
#'   summarise(mean = mean(signal),
#'     lwr = quantile(signal, 0.025), 
#'     upr = quantile(signal, 0.975)) 
#'     
#' # If we used type = "mean", we could do
#' # sumr_pred <- pred %>% 
#' #   group_by(time) %>%
#' #   summarise(mean = mean(value),
#' #     lwr = quantile(value, 0.025), 
#' #     upr = quantile(value, 0.975)) 
#'     
#' require("ggplot2")
#' rbind(sumr_fit, sumr_pred) %>% 
#'   ggplot(aes(x = time, y = mean)) + 
#'   geom_ribbon(aes(ymin = lwr, ymax = upr), 
#'    fill = "#92f0a8", alpha = 0.25) +
#'   geom_line(colour = "#92f0a8") +
#'   theme_bw() + 
#'   geom_point(data = data.frame(
#'     mean = log10(JohnsonJohnson), 
#'     time = time(JohnsonJohnson)))
#' 
#' # Posterior predictions for past observations:
#' yrep <- predict(mcmc_results, model, type = "response", 
#'   future = FALSE, nsim = 1000)
#' meanrep <- predict(mcmc_results, model, type = "mean", 
#'   future = FALSE, nsim = 1000)
#'   
#' sumr_yrep <- yrep %>% 
#'   group_by(time) %>%
#'   summarise(earnings = mean(value),
#'     lwr = quantile(value, 0.025), 
#'     upr = quantile(value, 0.975)) %>%
#'   mutate(interval = "Observations")
#'
#' sumr_meanrep <- meanrep %>% 
#'   group_by(time) %>%
#'   summarise(earnings = mean(value),
#'     lwr = quantile(value, 0.025), 
#'     upr = quantile(value, 0.975)) %>%
#'   mutate(interval = "Mean")
#'     
#' rbind(sumr_meanrep, sumr_yrep) %>% 
#'   mutate(interval = factor(interval, levels = c("Observations", "Mean"))) %>%
#'   ggplot(aes(x = time, y = earnings)) + 
#'   geom_ribbon(aes(ymin = lwr, ymax = upr, fill = interval), 
#'    alpha = 0.75) +
#'   theme_bw() + 
#'   geom_point(data = data.frame(
#'     earnings = model$y, 
#'     time = time(model$y)))    
#' 
#' 
predict.mcmc_output <- function(object, model, type = "response", nsim, future = TRUE,
  seed = sample(.Machine$integer.max, size = 1), ...) {
  
  type <- match.arg(type, c("response", "mean", "state"))
  
  if (object$output_type != 1) stop("MCMC output must contain posterior samples of the states.")
  
  
  if(!identical(attr(object, "model_type"), class(model)[1])) {
    stop("Model class does not correspond to the MCMC output. ")
  }
  if(!identical(ncol(object$theta), length(model$theta))) {
    stop("Number of unknown parameters 'theta' does not correspond to the MCMC output. ")
  }
  
  if(future) {
    
    if (attr(object, "model_type") %in% c("bsm_lg", "bsm_ng")) {
      object$theta[,1:(ncol(object$theta) - length(model$beta))] <- 
        log(object$theta[,1:(ncol(object$theta) - length(model$beta))])
    }
    w <- object$counts * (if(object$mcmc_type %in% paste0("is", 1:3)) object$weights else 1)
    idx <- sample(1:nrow(object$theta), size = nsim, prob = w, replace = TRUE)
    theta <- t(object$theta[idx, ])
    alpha <- matrix(object$alpha[nrow(object$alpha),,idx], nrow = ncol(object$alpha))
    
    switch(attr(object, "model_type"),
      ssm_mlg = ,
      ssm_ulg = ,
      bsm_lg = ,
      ar1_lg = {
        if (!identical(length(model$a1), ncol(object$alpha))) {
          stop("Model does not correspond to the MCMC output: Wrong number of states. ")
        }
        pred <- gaussian_predict(model, theta, alpha,
          pmatch(type, c("response", "mean", "state")), 
          seed, 
          pmatch(attr(object, "model_type"), 
            c("ssm_mng", "ssm_ulg", "bsm_lg", "ar1_lg")) - 1L)
        
      },
      ssm_mng = , 
      ssm_ung = , 
      bsm_ng = , 
      svm = ,
      ar1_ng = {
        if (!identical(length(model$a1), ncol(object$alpha))) {
          stop("Model does not correspond to the MCMC output: Wrong number of states. ")
        }
        model$distribution <- pmatch(model$distribution,
          c("svm", "poisson", "binomial", "negative binomial", "gamma", "gaussian"), 
          duplicates.ok = TRUE) - 1
        pred <- nongaussian_predict(model, theta, alpha,
          pmatch(type, c("response", "mean", "state")), seed, 
          pmatch(attr(object, "model_type"), 
            c("ssm_mng", "ssm_ung", "bsm_ng", "svm", "ar1_ng")) - 1L)
        
        if(anyNA(pred)) warning("NA or NaN values in predictions, possible under/overflow?")
      },
      ssm_nlg = {
        if (!identical(model$n_states, ncol(object$alpha))) {
          stop("Model does not correspond to the MCMC output: Wrong number of states. ")
        }
        pred <- nonlinear_predict(t(model$y), model$Z, 
          model$H, model$T, model$R, model$Z_gn, 
          model$T_gn, model$a1, model$P1, 
          model$log_prior_pdf, model$known_params, 
          model$known_tv_params, as.integer(model$time_varying),
          model$n_states, model$n_etas,
          theta, alpha, pmatch(type, c("response", "mean", "state")), seed)
        
      }
      , stop("Not yet implemented for ssm_sde. "))
    if(type == "state") {
      if(attr(object, "model_type") == "ssm_nlg") {
        variables <- model$state_names
      } else {
        variables <- names(model$a1)
      }
    } else {
      variables <- colnames(model$y)
      if(is.null(variables)) variables <- "Series 1"
    }
    d <- data.frame(value = as.numeric(pred),
      variable = variables,
      time = rep(time(model$y), each = nrow(pred)),
      sample = rep(1:nsim, each = nrow(pred) * ncol(pred)))
    
  } else {
    
    if(!identical(nrow(object$alpha) - 1L, length(model$y))) {
      stop("Number of observations of the model and MCMC output do not match. ") 
    }
    
    w <- object$counts * (if(object$mcmc_type %in% paste0("is", 1:3)) object$weights else 1)
    idx <- sample(1:nrow(object$theta), size = nsim, prob = w, replace = TRUE)
    n <- nrow(object$alpha) - 1L
    m <- ncol(object$alpha)
    
    states <- object$alpha[1:n, , idx]
    
    if(type == "state") {
      if(attr(object, "model_type") == "ssm_nlg") {
        variables <- model$state_names
      } else {
        variables <- names(model$a1)
      }
      d <- data.frame(value = as.numeric(states),
        variable = rep(variables, each = n),
        time = rep(time(model$y), times = m),
        sample = rep(1:nsim, each = n * m))
    } else {
      
      variables <- colnames(model$y)
      if(is.null(variables)) variables <- "Series 1"
      
      if (attr(object, "model_type") %in% c("bsm_lg", "bsm_ng")) {
        object$theta[,1:(ncol(object$theta) - length(model$beta))] <- 
          log(object$theta[,1:(ncol(object$theta) - length(model$beta))])
      }
      theta <- t(object$theta[idx, ])
      states <- aperm(states, c(2, 1, 3))
      
  
      switch(attr(object, "model_type"),
        ssm_mlg = ,
        ssm_ulg = ,
        bsm_lg = ,
        ar1_lg = {
          if (!identical(length(model$a1), m)) {
            stop("Model does not correspond to the MCMC output: Wrong number of states. ")
          }
          pred <- gaussian_predict_past(model, theta, states,
            pmatch(type, c("response", "mean", "state")), 
            seed, 
            pmatch(attr(object, "model_type"), 
              c("ssm_mng", "ssm_ulg", "bsm_lg", "ar1_lg")) - 1L)
          
        },
        ssm_mng = , 
        ssm_ung = , 
        bsm_ng = , 
        svm = ,
        ar1_ng = {
          if (!identical(length(model$a1), m)) {
            stop("Model does not correspond to the MCMC output: Wrong number of states. ")
          }
          model$distribution <- pmatch(model$distribution,
            c("svm", "poisson", "binomial", "negative binomial", "gamma", "gaussian"), 
            duplicates.ok = TRUE) - 1
          pred <- nongaussian_predict_past(model, theta, states,
            pmatch(type, c("response", "mean", "state")), seed, 
            pmatch(attr(object, "model_type"), 
              c("ssm_mng", "ssm_ung", "bsm_ng", "svm", "ar1_ng")) - 1L)
          
          if(anyNA(pred)) warning("NA or NaN values in predictions, possible under/overflow?")
        },
        ssm_nlg = {
          if (!identical(model$n_states, m)) {
            stop("Model does not correspond to the MCMC output: Wrong number of states. ")
          }
          pred <- nonlinear_predict_past(t(model$y), model$Z, 
            model$H, model$T, model$R, model$Z_gn, 
            model$T_gn, model$a1, model$P1, 
            model$log_prior_pdf, model$known_params, 
            model$known_tv_params, as.integer(model$time_varying),
            model$n_states, model$n_etas,
            theta, states, pmatch(type, c("response", "mean", "state")), seed)
          
        }
        , stop("Not yet implemented for ssm_sde. "))
     
      d <- data.frame(value = as.numeric(pred),
        variable = variables,
        time = rep(time(model$y), each = nrow(pred)),
        sample = rep(1:nsim, each = nrow(pred) * ncol(pred)))
    }
  }
  d
}
