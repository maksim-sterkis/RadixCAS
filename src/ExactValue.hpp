#pragma once
#include <string>
#include <vector>

#include "HybridInt.hpp"

struct VariablePower {
  std::string name;
  int power = 1;

  bool operator==(const VariablePower &o) const {
    return name == o.name && power == o.power;
  }
};

struct ExactTerm {
  HybridInt a = 1;
  HybridInt b = 1;
  HybridInt c = 1;
  long long root_degree = 2; // root_degree stays long long
  std::vector<VariablePower> vars;
  bool is_imaginary = false;

  void simplify();
};

bool are_like_terms(const ExactTerm &t1, const ExactTerm &t2);

struct ExactValue {
  std::vector<ExactTerm> terms;
  std::string symbolic_repr = "";
  double cached_double = 0.0;
  double cached_imag = 0.0;
  bool is_approx = false;

  void simplify();
};

ExactValue make_exact(HybridInt a, HybridInt b, HybridInt c, long long root,
                      double dbl, double imag = 0.0);
double to_double(const ExactValue &ev);
ExactValue double_to_exact(double val);
bool is_terminating_decimal(HybridInt denominator);
std::string format_double(double val);
std::string format_complex(double real, double imag);
bool has_variables(const ExactValue &ev);
std::string to_exact_string(const ExactValue &ev);
