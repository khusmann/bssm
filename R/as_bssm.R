#' Convert KFAS Model to bssm Model
#'
#' Converts \code{SSModel} object of \code{KFAS} package to general
#' \code{bssm} model of type \code{ssm_ulg}, \code{ssm_mlg}, \code{ssm_ung} or \code{ssm_mng}.
#' 
#' @param model Object of class \code{SSModel}.
#' @param kappa For \code{SSModel} object, a prior variance for initial state
#' used to replace exact diffuse elements of the original model.
#' @param ... Additional arguments to model building functions of \code{bssm}
#' (such as prior and updating functions).
#' @return Object of class \code{ssm_ulg}, \code{ssm_mlg}, \code{ssm_ung} or \code{ssm_mng}.
#' @export
as_bssm <- function(model, kappa = 100, ...) {
  
  if (!requireNamespace("KFAS", quietly = TRUE)) {
    stop("This function depends on the KFAS package. ", call. = FALSE)
  }
  
  model$P1[model$P1inf > 0] <- kappa
  
  tvr <- dim(model$R)[3] > 1
  tvq <- dim(model$Q)[3] > 1
  tvrq <- max(tvr, tvq)
  
  R <- array(0, c(dim(model$R)[1:2], tvrq * (nrow(model$y) - 1) + 1))
  
  if (dim(model$R)[2] > 1) {
    for (i in 1:dim(R)[3]) {
      L <- KFAS::ldl(model$Q[, , (i - 1) * tvq + 1])
      D <- sqrt(diag(diag(L)))
      diag(L) <- 1
      R[, , i] <- model$R[, , (i - 1) * tvr + 1] %*% L %*% D
    }
  } else {
    R <- model$R * sqrt(c(model$Q))
  }
  if (attr(model, "p") == 1) {
    Z <- aperm(model$Z, c(2, 3, 1))
    dim(Z) <- dim(Z)[1:2]
  } else {
    Z = model$Z
  }
  
  if (any(model$distribution != "gaussian")) {
    if (attr(model, "p") == 1) {
      
      if (model$distribution == "negative binomial" && length(unique(model$u)) > 1) {
        stop("Time-varying dispersion parameter for negative binomial is not supported in 'bssm'.")
      } 
      if (model$distribution == "gamma" && length(unique(model$u)) > 1) {
        stop("Time-varying shape parameter for gamma is not supported in 'bssm'.")
      }
      
      switch(model$distribution,
        poisson = {
          phi <- 1
          u <- model$u
        },
        binomial = {
          phi <- 1
          u <- model$u
        },
        gamma = {
          phi <- model$u[1]
          u <- rep(1, length(model$u))
        },
        "negative binomial" = {
          phi <- model$u[1]
          u <- rep(1, length(model$u))
        })
      out <- ssm_ung(y = model$y, Z = Z, T = model$T, R = R, a1 = c(model$a1), 
        P1 = model$P1, phi = phi, u = u, 
        distribution = model$distribution, state_names = rownames(model$a1), 
        ...)
    } else {
      phi <- numeric(attr(model, "p"))
      u <- model$u
      for(i in 1:attr(model, "p")) {
        switch(model$distribution[i],
          poisson = {
            phi[i] <- 1
            u[,i] <- model$u[,i]
          },
          poisson = {
            phi[i] <- 1
            u[,i] <- model$u[,i]
          },
          binomial = {
            phi[i] <- 1
            u[,i] <- model$u[,i]
          },
          gamma = {
            if(length(unique(model$u[,i])) > 1)
              stop("Time-varying shape parameter for gamma is not (yet) supported in 'bssm'.")
            phi[i] <- model$u[1,i]
            u[,i] <- 1
          },
          "negative binomial" = {
            if(length(unique(model$u[,i])) > 1)
              stop("Time-varying dispersion parameter for negative binomial is not (yet) supported in 'bssm'.")
            phi[i] <- model$u[1,i]
            u[,i] <- 1
          }, 
          gaussian = {
            if(length(unique(model$u[,i])) > 1)
              stop("Time-varying standard deviation for gaussian distribution with non-gaussian series is not supported in 'bssm'.")
            phi <- model$u[1,i]
            u[,i] <- 1
          })
      }
      
      out <- ssm_mng(y = model$y, Z = Z, T = model$T, R = R, a1 = c(model$a1), 
        P1 = model$P1, phi = phi, u = u, 
        distribution = model$distribution, state_names = rownames(model$a1), 
        ...)
    }
    
  } else {
    if (attr(model, "p") == 1) {
      H = sqrt(c(model$H))
      out <- ssm_ulg(y = model$y, Z =Z, H = H, T = model$T, R = R, 
        a1 = c(model$a1), P1 = model$P1, state_names = rownames(model$a1), ...)
    } else {
      H <- model$H
      for (i in 1:dim(H)[3]) {
        L <- KFAS::ldl(model$H[, , i])
        D <- sqrt(diag(diag(L)))
        diag(L) <- 1
        H[, , i] <- L %*% D
      }
      
      out <- ssm_mlg(y = model$y, Z = Z, H = H, T = model$T, R = R, 
        a1 = c(model$a1), P1 = model$P1, state_names = rownames(model$a1), ...)
    }
  }
  
  out
}

