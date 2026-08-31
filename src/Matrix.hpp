#pragma once
#include "AST.hpp"
#include "ExactValue.hpp"
#include <vector>

class Matrix {
public:
  int rows;
  int cols;
  std::vector<std::vector<ExactValue>> data;

  Matrix(int r, int c);

  void set(int r, int c, const ExactValue &val);
  ExactValue get(int r, int c) const;

  // Gaussian elimination to solve Ax = B. Modifies the matrix in place to
  // Reduced Row Echelon Form
  bool rref(ParserState &state);
  ExactValue det(ParserState &state);
  ExactValue invert(ParserState &state);
};
