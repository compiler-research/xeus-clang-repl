#include <vector>

#pragma once

class KalmanFilter {

public:

  /**
  * Create a Kalman filter with the specified matrices.
  *   A - System dynamics matrix
  *   C - Output matrix
  *   Q - Process noise covariance
  *   R - Measurement noise covariance
  *   P - Estimate error covariance
  */
  KalmanFilter(
      double dt,
      const std::vector<std::vector<double>>& A,
      const std::vector<std::vector<double>>& C,
      const std::vector<std::vector<double>>& Q,
      const std::vector<std::vector<double>>& R,
      const std::vector<std::vector<double>>& P
  );

  /**
  * Create a blank estimator.
  */
  KalmanFilter();

  /**
  * Initialize the filter with initial states as zero.
  */
  void init();

  /**
  * Initialize the filter with a guess for initial states.
  */
  void init(double t0, const std::vector<double>& x0);

  /**
  * Update the estimated state based on measured values. The
  * time step is assumed to remain constant.
  */
  void update(const std::vector<double>& y);

  /**
  * Update the estimated state based on measured values,
  * using the given time step and dynamics matrix.
  */
  void update(const std::vector<double>& y, double dt, const std::vector<std::vector<double>>& A);

  /**
  * Return the current state and time.
  */
  std::vector<double> state() { return x_hat; };
  double time() { return t; };

private:

  // Matrices for computation
  std::vector<std::vector<double>> A, C, Q, R, P, K, P0;

  // System dimensions
  int m, n;

  // Initial and current time
  double t0, t;

  // Discrete time step
  double dt;

  // Is the filter initialized?
  bool initialized;

  // n-size identity
  std::vector<std::vector<double>> I;

  // Estimated states
  std::vector<double> x_hat, x_hat_new;
};
