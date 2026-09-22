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

  // Convert this matrix to an ExactValue with symbolic_repr = "[[...], ...]"
  ExactValue to_exact_value() const;

  // Element-wise addition (requires same dimensions MxN + MxN -> MxN)
  Matrix mat_add(const Matrix &other, ParserState &state) const;

  // Element-wise subtraction (requires same dimensions MxN - MxN -> MxN)
  Matrix mat_subtract(const Matrix &other, ParserState &state) const;

  // Matrix multiplication: (MxK) * (KxN) -> (MxN)
  Matrix mat_multiply(const Matrix &other, ParserState &state) const;

  // Scalar multiplication: scalar * (MxN) -> (MxN)
  Matrix mat_scalar(const ExactValue &scalar, ParserState &state) const;

  // Gaussian elimination to solve Ax = B. Modifies the matrix in place to
  // Reduced Row Echelon Form
  bool rref(ParserState &state);
  ExactValue det(ParserState &state);
  ExactValue invert(ParserState &state);
};
