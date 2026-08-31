#include "Matrix.hpp"
#include "Engine.hpp"


Matrix::Matrix(int r, int c) : rows(r), cols(c) {
  ExactValue zero = make_exact(0, 1, 1, 2, 0.0);
  data.resize(r, std::vector<ExactValue>(c, zero));
}

void Matrix::set(int r, int c, const ExactValue &val) { data[r][c] = val; }

ExactValue Matrix::get(int r, int c) const { return data[r][c]; }

bool Matrix::rref(ParserState &state) {
  int lead = 0;
  for (int r = 0; r < rows; r++) {
    if (cols <= lead)
      return true;

    int i = r;
    while (data[i][lead].cached_double == 0.0 &&
           data[i][lead].symbolic_repr.empty() && data[i][lead].terms.empty()) {
      i++;
      if (rows == i) {
        i = r;
        lead++;
        if (cols == lead)
          return true;
      }
    }

    std::swap(data[i], data[r]);

    ExactValue lv = data[r][lead];
    if (lv.cached_double != 0.0 || !lv.terms.empty() ||
        !lv.symbolic_repr.empty()) {
      for (int c = 0; c < cols; c++) {
        data[r][c] = divide(data[r][c], lv, state);
      }
    }

    for (int i = 0; i < rows; i++) {
      if (i != r) {
        ExactValue lv_i = data[i][lead];
        for (int c = 0; c < cols; c++) {
          ExactValue mult = multiply(lv_i, data[r][c], state);
          data[i][c] = subtract(data[i][c], mult, state);
        }
      }
    }
    lead++;
  }
  return true;
}
ExactValue Matrix::det(ParserState &state) {
  if (rows != cols) {
    state.error = ParseError::UNSUPPORTED_OPERATION;
    state.error_extra = "determinant requires a square matrix";
    return {};
  }
  int n = rows;
  ExactValue det_val = make_exact(1, 1, 1, 2, 1.0);
  int sign = 1;
  Matrix mat = *this;

  for (int r = 0; r < n; ++r) {
    int pivot = r;
    for (int i = r; i < n; ++i) {
      if (mat.data[i][r].cached_double != 0.0 || !mat.data[i][r].terms.empty() || !mat.data[i][r].symbolic_repr.empty()) {
        pivot = i; break;
      }
    }
    ExactValue pivot_val = mat.data[pivot][r];
    bool is_zero = (pivot_val.cached_double == 0.0 && pivot_val.terms.empty() && pivot_val.symbolic_repr.empty());
    if (is_zero) return make_exact(0, 1, 1, 2, 0.0);
    
    if (pivot != r) {
      std::swap(mat.data[r], mat.data[pivot]);
      sign = -sign;
    }
    
    ExactValue curr_pivot = mat.data[r][r];
    det_val = multiply(det_val, curr_pivot, state);
    
    for (int i = r + 1; i < n; ++i) {
      ExactValue factor = divide(mat.data[i][r], curr_pivot, state);
      for (int j = r; j < n; ++j) {
        ExactValue sub = multiply(factor, mat.data[r][j], state);
        mat.data[i][j] = subtract(mat.data[i][j], sub, state);
      }
    }
  }

  if (sign == -1) {
    ExactValue neg_one = make_exact(-1, 1, 1, 2, -1.0);
    det_val = multiply(det_val, neg_one, state);
  }
  return det_val;
}

ExactValue Matrix::invert(ParserState &state) {
  if (rows != cols) {
    state.error = ParseError::UNSUPPORTED_OPERATION;
    state.error_extra = "invert requires a square matrix";
    return {};
  }
  int n = rows;
  Matrix aug(n, 2 * n);
  for (int r = 0; r < n; ++r) {
    for (int c = 0; c < n; ++c) aug.set(r, c, data[r][c]);
    aug.set(r, n + r, make_exact(1, 1, 1, 2, 1.0));
  }
  
  if (!aug.rref(state)) {
    state.error = ParseError::UNSUPPORTED_OPERATION;
    state.error_extra = "matrix is singular";
    return {};
  }
  
  for (int r = 0; r < n; ++r) {
    ExactValue diag = aug.get(r, r);
    ExactValue diff = subtract(diag, make_exact(1, 1, 1, 2, 1.0), state);
    bool is_one = diff.terms.empty() && diff.symbolic_repr.empty() && std::abs(diff.cached_double) < 1e-9;
    if (!is_one) {
      state.error = ParseError::UNSUPPORTED_OPERATION;
      state.error_extra = "matrix is singular";
      return {};
    }
  }
  
  std::string res = "[";
  for (int r = 0; r < n; ++r) {
    res += "[";
    for (int c = 0; c < n; ++c) {
      res += to_exact_string(aug.get(r, n + c));
      if (c < n - 1) res += ", ";
    }
    res += "]";
    if (r < n - 1) res += ", ";
  }
  res += "]";
  
  ExactValue ev;
  ev.symbolic_repr = res;
  return ev;
}
