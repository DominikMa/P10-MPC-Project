#include "MPC.h"
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp>
#include "Eigen-3.3/Eigen/Core"

using CppAD::AD;

size_t N = 10;
double dt = 0.05;

// This value assumes the model presented in the classroom is used.
//
// It was obtained by measuring the radius formed by running the vehicle in the
// simulator around in a circle with a constant steering angle and velocity on a
// flat terrain.
//
// Lf was tuned until the the radius formed by the simulating the model
// presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
const double Lf = 2.67;

double ref_v = 150;

// The solver takes all the state variables and actuator
// variables in a singular vector. Thus, we should to establish
// when one variable starts and another ends to make our lifes easier.
size_t x_start = 0;
size_t y_start = x_start + N;
size_t psi_start = y_start + N;
size_t v_start = psi_start + N;
size_t cte_start = v_start + N;
size_t epsi_start = cte_start + N;
size_t delta_start = epsi_start + N;
size_t a_start = delta_start + N - 1;


// static double weight_cte_inc = 0;

class FG_eval {
 public:
 
  double weight_cte = 0;
  double weight_cte_change = 0;
  double weight_epsi = 0;
  
  double weight_delta = 0;
  double weight_delta_dt = 0.0000;
  double weight_delta_mean = 0.000;
  double weight_delta_change = 0;

  double weight_v = 0;
  double weight_a = 0;
  double weight_a_dt = 0;

  // Fitted polynomial coefficients
  Eigen::VectorXd coeffs;
  FG_eval(Eigen::VectorXd coeffs) { this->coeffs = coeffs; }

  typedef CPPAD_TESTVECTOR(AD<double>) ADvector;
  void operator()(ADvector &fg, const ADvector &vars) {
    fg[0] = 0;

    
    // The part of the cost based on the reference state.
    for (int t = 0; t < N; t++) {
      fg[0] += weight_cte * CppAD::pow(vars[cte_start + t], 2);
      fg[0] += weight_epsi * CppAD::pow(vars[epsi_start + t], 2);
      fg[0] += weight_v * CppAD::pow(vars[v_start + t] - ref_v, 2);
    }



    // Minimize the use of actuators.
    for (int t = 0; t < N - 1; t++) {
      fg[0] += weight_cte_change * CppAD::pow(CppAD::abs(vars[cte_start + t + 1]) + CppAD::abs(vars[cte_start + t]) - CppAD::abs(vars[cte_start + t +1] + vars[cte_start + t]), 6);
      fg[0] += weight_delta * CppAD::pow(vars[delta_start + t] / (CppAD::abs(vars[cte_start + t])+0.001), 2);      
      fg[0] += weight_a * CppAD::pow(vars[a_start + t], 2);
    }

    // Minimize the value gap between sequential actuations.
    AD<double> sum_delta = 0.0;
    AD<double> count_delta = 0.0;
    for (int t = 0; t < N - 2; t++) {
      fg[0] += weight_delta_dt * CppAD::pow((vars[delta_start + t + 1] - vars[delta_start + t]) / (CppAD::abs(vars[cte_start + t])+0.001), 2);
      fg[0] += weight_delta_change * CppAD::pow(CppAD::abs(vars[delta_start + t + 1]) + CppAD::abs(vars[delta_start + t]) - CppAD::abs(vars[delta_start + t +1] + vars[delta_start + t]), 6);
      fg[0] += weight_a_dt * CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);
      
      sum_delta += CppAD::pow((vars[delta_start + t + 1] - vars[delta_start + t]), 2);
      count_delta += 1.0;
    }
    if(count_delta != 0.0){
      fg[0] += weight_delta_mean * sum_delta/count_delta;
    }









    // Set the constraints for the initial state
    fg[1 + x_start] = vars[x_start];
    fg[1 + y_start] = vars[y_start];
    fg[1 + psi_start] = vars[psi_start];
    fg[1 + v_start] = vars[v_start];
    fg[1 + cte_start] = vars[cte_start];
    fg[1 + epsi_start] = vars[epsi_start];

    // The rest of the constraints
    for (int t = 1; t < N; t++) {
      // The state at time t.
      AD<double> x_t0 = vars[x_start + t - 1];
      AD<double> y_t0 = vars[y_start + t - 1];
      AD<double> psi_t0 = vars[psi_start + t - 1];
      AD<double> v_t0 = vars[v_start + t - 1];
      AD<double> cte_t0 = vars[cte_start + t - 1];
      AD<double> epsi_t0 = vars[epsi_start + t - 1];

      // The state at time t+1 .
      AD<double> x_t1 = vars[x_start + t];
      AD<double> y_t1 = vars[y_start + t];
      AD<double> psi_t1 = vars[psi_start + t];
      AD<double> v_t1 = vars[v_start + t];
      AD<double> cte_t1 = vars[cte_start + t];
      AD<double> epsi_t1 = vars[epsi_start + t];

      // Only consider the actuation at time t.
      AD<double> delta_t0 = vars[delta_start + t - 1];
      AD<double> a_t0 = vars[a_start + t - 1];

      AD<double>
          f_t0 = coeffs[0] + coeffs[1] * x_t0 + coeffs[2] * CppAD::pow(x_t0, 2) + coeffs[3] * CppAD::pow(x_t0, 3);
      AD<double> psides_t0 = CppAD::atan(coeffs[1] + 2 * coeffs[2] * x_t0 + 3 * coeffs[3] * CppAD::pow(x_t0, 2));

      fg[1 + x_start + t] = x_t1 - (x_t0 + v_t0 * CppAD::cos(psi_t0) * dt);
      fg[1 + y_start + t] = y_t1 - (y_t0 + v_t0 * CppAD::sin(psi_t0) * dt);
      fg[1 + psi_start + t] = psi_t1 - (psi_t0 + v_t0 * delta_t0 / Lf * dt);
      fg[1 + v_start + t] = v_t1 - (v_t0 + a_t0 * dt);
      fg[1 + cte_start + t] = cte_t1 - ((f_t0 - y_t0) + (v_t0 * CppAD::sin(epsi_t0) * dt));
      fg[1 + epsi_start + t] = epsi_t1 - ((psi_t0 - psides_t0) + v_t0 * delta_t0 / Lf * dt);
    }
  }
};

//
// MPC class definition implementation.
//
MPC::MPC() {}
MPC::~MPC() {}

std::vector<double> MPC::Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs) {
  bool ok = true;
  size_t i;
  typedef CPPAD_TESTVECTOR(double) Dvector;

  double x = state[0];
  double y = state[1];
  double psi = state[2];
  double v = state[3];
  double cte = state[4];
  double epsi = state[5];

  // Set the number of model variables (includes both states and inputs).
  size_t n_vars = N * 6 + (N - 1) * 2;
  // Set the number of constraints
  size_t n_constraints = N * 6;

  // Initial value of the independent variables.
  // SHOULD BE 0 besides initial state.
  Dvector vars(n_vars);
  for (int i = 0; i < n_vars; i++) {
    vars[i] = 0;
  }

  // Set the initial variable values
  vars[x_start] = x;
  vars[y_start] = y;
  vars[psi_start] = psi;
  vars[v_start] = v;
  vars[cte_start] = cte;
  vars[epsi_start] = epsi;

  Dvector vars_lowerbound(n_vars);
  Dvector vars_upperbound(n_vars);
  // Set lower and upper limits for variables.

  // Set all non-actuators upper and lowerlimits
  // to the max negative and positive values.
  for (int i = 0; i < delta_start; i++) {
    vars_lowerbound[i] = -1.0e19;
    vars_upperbound[i] = 1.0e19;
  }

  // The upper and lower limits of delta are set to -25 and 25
  // degrees (values in radians).
  // NOTE: Feel free to change this to something else.
  for (int i = delta_start; i < a_start; i++) {
    vars_lowerbound[i] = -0.436332;
    vars_upperbound[i] = 0.436332;
  }

  // Acceleration/decceleration upper and lower limits.
  // NOTE: Feel free to change this to something else.
  for (int i = a_start; i < n_vars; i++) {
    vars_lowerbound[i] = -1.0;
    vars_upperbound[i] = 1.0;
  }

  // Lower and upper limits for the constraints
  // Should be 0 besides initial state.
  Dvector constraints_lowerbound(n_constraints);
  Dvector constraints_upperbound(n_constraints);
  for (int i = 0; i < n_constraints; i++) {
    constraints_lowerbound[i] = 0;
    constraints_upperbound[i] = 0;
  }

  constraints_lowerbound[x_start] = x;
  constraints_lowerbound[y_start] = y;
  constraints_lowerbound[psi_start] = psi;
  constraints_lowerbound[v_start] = v;
  constraints_lowerbound[cte_start] = cte;
  constraints_lowerbound[epsi_start] = epsi;

  constraints_upperbound[x_start] = x;
  constraints_upperbound[y_start] = y;
  constraints_upperbound[psi_start] = psi;
  constraints_upperbound[v_start] = v;
  constraints_upperbound[cte_start] = cte;
  constraints_upperbound[epsi_start] = epsi;

  // object that computes objective and constraints
  FG_eval fg_eval(coeffs);
  fg_eval.weight_cte = this->weight_cte;
  fg_eval.weight_cte_change = this->weight_cte_change;
  fg_eval.weight_delta = this->weight_delta;
  fg_eval.weight_delta_dt = this->weight_delta_dt;
  fg_eval.weight_delta_mean = this->weight_delta_mean;
  fg_eval.weight_delta_change = this->weight_delta_change;
  fg_eval.weight_v = this->weight_v;
  fg_eval.weight_a = this->weight_a;
  fg_eval.weight_a_dt = this->weight_a_dt;
  

  //
  // NOTE: You don't have to worry about these options
  //
  // options for IPOPT solver
  std::string options;
  // Uncomment this if you'd like more print information
  options += "Integer print_level  0\n";
  // NOTE: Setting sparse to true allows the solver to take advantage
  // of sparse routines, this makes the computation MUCH FASTER. If you
  // can uncomment 1 of these and see if it makes a difference or not but
  // if you uncomment both the computation time should go up in orders of
  // magnitude.
  options += "Sparse  true        forward\n";
  options += "Sparse  true        reverse\n";
  // NOTE: Currently the solver has a maximum time limit of 0.5 seconds.
  // Change this as you see fit.
  options += "Numeric max_cpu_time          0.5\n";

  // place to return solution
  CppAD::ipopt::solve_result<Dvector> solution;

  // solve the problem
  CppAD::ipopt::solve<Dvector, FG_eval>(
      options, vars, vars_lowerbound, vars_upperbound, constraints_lowerbound,
      constraints_upperbound, fg_eval, solution);

  // Check some of the solution values
  ok &= solution.status == CppAD::ipopt::solve_result<Dvector>::success;

  // Cost
  auto cost = solution.obj_value;
  std::cout << "Cost " << cost << std::endl;

  // Return the first actuator values. The variables can be accessed with
  std::vector<double> result;
  result.push_back(solution.x[delta_start]);
  result.push_back(solution.x[a_start]);

  for (int i = 0; i < N; ++i) {
    result.push_back(solution.x[x_start + i]);
    result.push_back(solution.x[y_start + i]);
  }
  
  
  
  AD<double> cte_e = 0.0;
  AD<double> weight_delta_e = 0.0;
  AD<double> weight_delta_dt_e = 0.0;
  AD<double> weight_delta_mean_e = 0.0;
  AD<double> weight_v_e = 0.0;
  AD<double> weight_a_dt_e = 0.0;
  
  
  // The part of the cost based on the reference state.
    for (int t = 0; t < N; t++) {
      cte_e += weight_cte * CppAD::pow(solution.x[cte_start + t], 2);
      weight_v_e += weight_v * CppAD::abs(solution.x[v_start + t] - ref_v);
    }

    // Minimize the use of actuators.
    AD<double> sum_delta = 0.0;
    AD<double> count_delta = 0.0;
    for (int t = 0; t < N - 1; t++) {
      AD<double> cte_pow = CppAD::pow(solution.x[cte_start + t], 2);
      if (cte_pow <= 0.1) {
        cte_pow = 0.1;
      }
      weight_delta_e += weight_delta * CppAD::pow(solution.x[delta_start + t] / cte_pow, 2);
      sum_delta += CppAD::pow(solution.x[delta_start + t] / cte_pow, 2);
      count_delta += 1.0;
    }
    if(count_delta != 0.0){
      weight_delta_mean_e += weight_delta_mean * sum_delta/count_delta;
    }

    // Minimize the value gap between sequential actuations.
    for (int t = 0; t < N - 2; t++) {
      AD<double> cte_pow = CppAD::pow(solution.x[cte_start + t], 2);
      if (cte_pow <= 0.1) {
        cte_pow = 0.1;
      }
      weight_delta_dt_e += weight_delta_dt * CppAD::pow((solution.x[delta_start + t + 1] - solution.x[delta_start + t]) / cte_pow, 2);
      weight_a_dt_e += weight_a_dt * CppAD::pow(solution.x[a_start + t + 1] - solution.x[a_start + t], 2);
    }
    
    
  std::cout << "cte_e " << cte_e << std::endl;
  std::cout << "weight_delta_e " << weight_delta_e << std::endl;
  std::cout << "weight_delta_dt_e " << weight_delta_dt_e << std::endl;
  std::cout << "weight_delta_mean_e " << weight_delta_mean_e << std::endl;
  std::cout << "weight_v_e " << weight_v_e << std::endl;
  std::cout << "weight_a_dt_e " << weight_a_dt_e << std::endl;
  
    
    

  return result;
}
