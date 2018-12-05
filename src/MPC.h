#ifndef MPC_H
#define MPC_H

#include <vector>
#include "Eigen-3.3/Eigen/Core"

class MPC {
 public:
  MPC();

  virtual ~MPC();

  // Solve the model given an initial state and polynomial coefficients.
  // Return the first actuations.
  std::vector<double> Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs);
  
  double weight_cte = 0;
  double weight_cte_change = 0;
  double weight_epsi = 0;
  
  double weight_delta = 0;
  double weight_delta_dt = 0;
  double weight_delta_mean = 0;
  double weight_delta_change = 0;
  
  double weight_v = 0;
  double weight_a = 0;
  double weight_a_dt = 0;
};

#endif /* MPC_H */
