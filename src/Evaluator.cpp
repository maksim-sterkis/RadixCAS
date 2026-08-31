#include "Evaluator.hpp"
#include "Calculus.hpp"
#include "HybridInt.hpp"
#include <functional>
#include "Engine.hpp"
#include "Matrix.hpp"
#include "Parser.hpp"
#include "Polynomial.hpp"
#include "Tokenizer.hpp"
#include <cmath>
#include <complex>
#include <numbers>

std::map<std::string, ExactValue> global_variables;
std::string current_target_var = "";

ExactValue lookup_trig(const std::string &func, int ref, bool positive,
                       ParserState &state) {
  ExactValue ev;
  bool undefined = false;

  if (func == "sin") {
    if (ref == 0)
      ev = make_exact(0, 1, 1, 2, 0.0);
    else if (ref == 30)
      ev = make_exact(1, 1, 2, 2, 0.5);
    else if (ref == 45)
      ev = make_exact(1, 2, 2, 2, 0.70710678118);
    else if (ref == 60)
      ev = make_exact(1, 3, 2, 2, 0.86602540378);
    else if (ref == 90)
      ev = make_exact(1, 1, 1, 2, 1.0);
  } else if (func == "cos") {
    if (ref == 0)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 30)
      ev = make_exact(1, 3, 2, 2, 0.86602540378);
    else if (ref == 45)
      ev = make_exact(1, 2, 2, 2, 0.70710678118);
    else if (ref == 60)
      ev = make_exact(1, 1, 2, 2, 0.5);
    else if (ref == 90)
      ev = make_exact(0, 1, 1, 2, 0.0);
  } else if (func == "tan") {
    if (ref == 0)
      ev = make_exact(0, 1, 1, 2, 0.0);
    else if (ref == 30)
      ev = make_exact(1, 3, 3, 2, 0.57735026919);
    else if (ref == 45)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 60)
      ev = make_exact(1, 3, 1, 2, 1.73205080757);
    else if (ref == 90)
      undefined = true;
  } else if (func == "csc") {
    if (ref == 0)
      undefined = true;
    else if (ref == 30)
      ev = make_exact(2, 1, 1, 2, 2.0);
    else if (ref == 45)
      ev = make_exact(1, 2, 1, 2, 1.41421356237);
    else if (ref == 60)
      ev = make_exact(2, 3, 3, 2, 1.15470053838);
    else if (ref == 90)
      ev = make_exact(1, 1, 1, 2, 1.0);
  } else if (func == "sec") {
    if (ref == 0)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 30)
      ev = make_exact(2, 3, 3, 2, 1.15470053838);
    else if (ref == 45)
      ev = make_exact(1, 2, 1, 2, 1.41421356237);
    else if (ref == 60)
      ev = make_exact(2, 1, 1, 2, 2.0);
    else if (ref == 90)
      undefined = true;
  } else if (func == "cot") {
    if (ref == 0)
      undefined = true;
    else if (ref == 30)
      ev = make_exact(1, 3, 1, 2, 1.73205080757);
    else if (ref == 45)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 60)
      ev = make_exact(1, 3, 3, 2, 0.57735026919);
    else if (ref == 90)
      ev = make_exact(0, 1, 1, 2, 0.0);
  }

  if (undefined) {
    state.error = ParseError::DIVIDE_BY_ZERO;
    return {};
  }

  if (!positive) {
    for (auto &t : ev.terms)
      t.a = -t.a;
    ev.cached_double = -ev.cached_double;
  }
  ev.simplify();
  return ev;
}

int simplify_ast(ParserState &state, int node_idx) {
  if (node_idx < 0 || node_idx >= state.ast_pool.size())
    return node_idx;
  ASTNode &node = state.ast_pool[node_idx];

  if (node.type == ASTNodeType::FUNCTION || node.type == ASTNodeType::UNARY ||
      node.type == ASTNodeType::BINARY) {
    if (node.left_idx != -1)
      node.left_idx = simplify_ast(state, node.left_idx);
    if (node.right_idx != -1)
      node.right_idx = simplify_ast(state, node.right_idx);
  }

  if (node.type == ASTNodeType::BINARY && node.op == '+') {
    const ASTNode &left = state.ast_pool[node.left_idx];
    const ASTNode &right = state.ast_pool[node.right_idx];
    if (left.type == ASTNodeType::FUNCTION &&
        right.type == ASTNodeType::FUNCTION) {
      if ((left.func_name == "log" && right.func_name == "log") ||
          (left.func_name == "ln" && right.func_name == "ln")) {

        ASTNode mul_node;
        mul_node.type = ASTNodeType::BINARY;
        mul_node.op = '*';
        mul_node.left_idx = left.right_idx;
        mul_node.right_idx = right.right_idx;
        state.ast_pool.push_back(mul_node);
        int new_mul_idx = state.ast_pool.size() - 1;

        ASTNode log_node;
        log_node.type = ASTNodeType::FUNCTION;
        log_node.func_name = left.func_name;
        log_node.right_idx = new_mul_idx;
        state.ast_pool.push_back(log_node);
        return state.ast_pool.size() - 1;
      }
    }
  }

  if (node.type == ASTNodeType::BINARY && node.op == '-') {
    const ASTNode &left = state.ast_pool[node.left_idx];
    const ASTNode &right = state.ast_pool[node.right_idx];
    if (left.type == ASTNodeType::FUNCTION &&
        right.type == ASTNodeType::FUNCTION) {
      if ((left.func_name == "log" && right.func_name == "log") ||
          (left.func_name == "ln" && right.func_name == "ln")) {

        ASTNode div_node;
        div_node.type = ASTNodeType::BINARY;
        div_node.op = '/';
        div_node.left_idx = left.right_idx;
        div_node.right_idx = right.right_idx;
        state.ast_pool.push_back(div_node);
        int new_div_idx = state.ast_pool.size() - 1;

        ASTNode log_node;
        log_node.type = ASTNodeType::FUNCTION;
        log_node.func_name = left.func_name;
        log_node.right_idx = new_div_idx;
        state.ast_pool.push_back(log_node);
        return state.ast_pool.size() - 1;
      }
    }
  }

  if (node.type == ASTNodeType::BINARY && node.op == '*') {
    const ASTNode &left = state.ast_pool[node.left_idx];
    const ASTNode &right = state.ast_pool[node.right_idx];
    if (right.type == ASTNodeType::FUNCTION &&
        (right.func_name == "log" || right.func_name == "ln")) {
      ASTNode pow_node;
      pow_node.type = ASTNodeType::BINARY;
      pow_node.op = '^';
      pow_node.left_idx = right.right_idx;
      pow_node.right_idx = node.left_idx;
      state.ast_pool.push_back(pow_node);
      int new_pow_idx = state.ast_pool.size() - 1;

      ASTNode log_node;
      log_node.type = ASTNodeType::FUNCTION;
      log_node.func_name = right.func_name;
      log_node.right_idx = new_pow_idx;
      state.ast_pool.push_back(log_node);
      return state.ast_pool.size() - 1;
    }
  }

  // sin^2(x) -> 1 - cos^2(x)
  if (node.type == ASTNodeType::BINARY && node.op == '^') {
    const ASTNode &left = state.ast_pool[node.left_idx];
    const ASTNode &right = state.ast_pool[node.right_idx];
    if (left.type == ASTNodeType::FUNCTION && left.func_name == "sin" &&
        right.type == ASTNodeType::LITERAL &&
        right.value.cached_double == 2.0) {

      ASTNode cos_node;
      cos_node.type = ASTNodeType::FUNCTION;
      cos_node.func_name = "cos";
      cos_node.right_idx = left.right_idx;
      state.ast_pool.push_back(cos_node);
      int new_cos_idx = state.ast_pool.size() - 1;

      ASTNode pow_node;
      pow_node.type = ASTNodeType::BINARY;
      pow_node.op = '^';
      pow_node.left_idx = new_cos_idx;
      pow_node.right_idx = node.right_idx;
      state.ast_pool.push_back(pow_node);
      int new_pow_idx = state.ast_pool.size() - 1;

      ASTNode one_node;
      one_node.type = ASTNodeType::LITERAL;
      one_node.value = make_exact(1, 1, 1, 2, 1.0);
      state.ast_pool.push_back(one_node);
      int new_one_idx = state.ast_pool.size() - 1;

      ASTNode sub_node;
      sub_node.type = ASTNodeType::BINARY;
      sub_node.op = '-';
      sub_node.left_idx = new_one_idx;
      sub_node.right_idx = new_pow_idx;
      state.ast_pool.push_back(sub_node);
      return state.ast_pool.size() - 1;
    }
  }

  return node_idx;
}

int clone_and_substitute(ParserState &state, int node_idx,
                         const std::string &target_var, int sub_idx) {
  if (node_idx < 0 || node_idx >= state.ast_pool.size())
    return node_idx;
  const ASTNode &old_node = state.ast_pool[node_idx];

  if (old_node.type == ASTNodeType::LITERAL &&
      old_node.value.terms.size() == 1 &&
      old_node.value.terms[0].vars.size() == 1) {
    if (old_node.value.terms[0].vars[0].name == target_var &&
        old_node.value.terms[0].a == 1 && old_node.value.terms[0].b == 1 &&
        old_node.value.terms[0].c == 1 &&
        old_node.value.terms[0].root_degree == 2) {
      // Clone the sub_idx tree
      int sub_clone =
          clone_and_substitute(state, sub_idx, "", -1); // Just deep clone
      return sub_clone;
    }
  }

  ASTNode new_node = old_node;
  if (new_node.left_idx != -1)
    new_node.left_idx =
        clone_and_substitute(state, new_node.left_idx, target_var, sub_idx);
  if (new_node.right_idx != -1)
    new_node.right_idx =
        clone_and_substitute(state, new_node.right_idx, target_var, sub_idx);

  // Arrays/functions
  for (size_t i = 0; i < new_node.args.size(); ++i) {
    new_node.args[i] =
        clone_and_substitute(state, new_node.args[i], target_var, sub_idx);
  }

  state.ast_pool.push_back(new_node);
  return state.ast_pool.size() - 1;
}

ExactValue evaluate(ParserState &state, int node_idx) {
  if (state.error != ParseError::NONE)
    return {};
  if (node_idx < 0 || node_idx >= static_cast<int>(state.ast_pool.size()))
    return {};

  ASTNode node = state.ast_pool[node_idx];

  if (node.type == ASTNodeType::LITERAL) {
    return substitute_variables(node.value, state);
  }
  if (node.type == ASTNodeType::UNARY) {
    if (node.op == '-') {
      ExactValue res = evaluate(state, node.right_idx);
      for (auto &t : res.terms)
        t.a = -t.a;
      res.cached_double = -res.cached_double;
      res.cached_imag = -res.cached_imag;
      res.simplify();
      return res;
    }
    if (node.op == '!') {
      ExactValue res = evaluate(state, node.left_idx);
      if (res.terms.size() == 0) return make_exact(HybridInt(1), HybridInt(1), HybridInt(1), 2, 1.0);
      if (res.terms.size() != 1 || res.terms[0].b != HybridInt(1) || res.terms[0].c != HybridInt(1) || res.terms[0].root_degree != 2 || res.terms[0].is_imaginary || res.terms[0].vars.size() > 0) {
         state.error = ParseError::UNSUPPORTED_OPERATION;
         state.error_extra = "factorial only supports non-negative integers";
         return {};
      }
      if (res.terms[0].a < HybridInt(0)) {
         state.error = ParseError::UNSUPPORTED_OPERATION;
         state.error_extra = "factorial only supports non-negative integers";
         return {};
      }
      HybridInt n = res.terms[0].a;
      HybridInt fact = HybridInt(1);
      HybridInt zero = HybridInt(0);
      HybridInt one = HybridInt(1);
      while (zero < n) {
        fact = fact * n;
        n = n - one;
      }
      return make_exact(fact, HybridInt(1), HybridInt(1), 2, fact.to_double());
    }
  }
  if (node.type == ASTNodeType::FUNCTION) {
    if (node.func_name == "derivative") {
      if (node.args.size() != 2) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "derivative requires 2 arguments: derivative(expression, variable)";
        return {};
      }
      ExactValue var_ev = evaluate(state, node.right_idx);
      if (var_ev.terms.size() != 1 || var_ev.terms[0].vars.size() != 1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "Target variable for derivative must be singular";
        return {};
      }
      std::string target_var = var_ev.terms[0].vars[0].name;
      int du_idx = differentiate_ast(state, node.left_idx, target_var);
      return evaluate(state, du_idx);
    }

    if (node.func_name == "integral") {
      if (node.args.size() != 2) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "integral requires 2 arguments: integral(expression, variable)";
        return {};
      }
      ExactValue expr = evaluate(state, node.left_idx);
      ExactValue var_ev = evaluate(state, node.right_idx);
      if (var_ev.terms.size() != 1 || var_ev.terms[0].vars.size() != 1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "Target variable for integral must be singular";
        return {};
      }
      std::string target_var = var_ev.terms[0].vars[0].name;
      return integrate_polynomial(expr, target_var, state);
    }

    if (node.func_name == "limit") {
      if (node.args.size() != 3) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "limit requires 3 arguments: limit(expr, var, val)";
        return {};
      }
      
      int expr_idx = node.args[0];
      ExactValue var_ev = evaluate(state, node.args[1]);
      ExactValue val_ev = evaluate(state, node.args[2]);
      
      if (var_ev.terms.size() != 1 || var_ev.terms[0].vars.size() != 1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "Target variable for limit must be singular";
        return {};
      }
      std::string target_var = var_ev.terms[0].vars[0].name;

      // Function to attempt limit evaluation with L'Hopital's rule
      std::function<ExactValue(int, int)> eval_limit = [&](int e_idx, int depth) -> ExactValue {
          if (depth > 10) {
              state.error = ParseError::UNSUPPORTED_OPERATION;
              state.error_extra = "limit evaluation exceeded max L'Hopital iterations";
              return {};
          }
          
          // Clone expression and substitute
          int sub_idx = clone_and_substitute(state, e_idx, target_var, node.args[2]);
          ExactValue eval_res = evaluate(state, sub_idx);
          
          // Check if Division By Zero or 0/0 occurred
          if (state.error == ParseError::DIVIDE_BY_ZERO) {
              state.error = ParseError::NONE; // Reset error
              // Check if the original expression was a division
              ASTNode e_node = state.ast_pool[e_idx];
              if (e_node.type == ASTNodeType::BINARY && e_node.op == '/') {
                  // Verify that numerator is 0 for 0/0 form
                  int num_sub_idx = clone_and_substitute(state, e_node.left_idx, target_var, node.args[2]);
                  ExactValue num_val = evaluate(state, num_sub_idx);
                  bool is_zero = (num_val.cached_double == 0.0 && num_val.terms.empty() && num_val.symbolic_repr.empty());
                  if (!is_zero) {
                      ExactValue inf_res;
                      inf_res.symbolic_repr = "Infinity (Does Not Exist)";
                      inf_res.cached_double = __builtin_inf();
                      inf_res.is_approx = true;
                      return inf_res;
                  }
                  // We need to differentiate top and bottom
                  int num_du = differentiate_ast(state, e_node.left_idx, target_var);
                  int den_du = differentiate_ast(state, e_node.right_idx, target_var);
                  
                  // Construct new division
                  ASTNode div_node;
                  div_node.type = ASTNodeType::BINARY;
                  div_node.op = '/';
                  div_node.left_idx = num_du;
                  div_node.right_idx = den_du;
                  state.ast_pool.push_back(div_node);
                  return eval_limit(state.ast_pool.size() - 1, depth + 1);
              } else {
                  // E.g. 1/x -> DNE
                  state.error = ParseError::UNSUPPORTED_OPERATION;
                  state.error_extra = "limit Does Not Exist (infinity)";
                  return {};
              }
          }
          
          // Also need to handle 0/0 if it evaluated normally to 0/0
          // Wait, Engine.cpp divide() sets DIVISION_BY_ZERO.
          // Wait! What if `val_ev` is `infinity`? The engine does not support infinity yet.
          
          return eval_res;
      };
      
      return eval_limit(expr_idx, 0);
    }

    if (node.func_name == "sum") {
      if (node.args.size() != 4) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "sum requires 4 arguments: sum(expr, var, start, end)";
        return {};
      }
      
      int expr_idx = node.args[0];
      ExactValue var_ev = evaluate(state, node.args[1]);
      ExactValue start_ev = evaluate(state, node.args[2]);
      ExactValue end_ev = evaluate(state, node.args[3]);
      
      if (var_ev.terms.size() != 1 || var_ev.terms[0].vars.size() != 1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "Target variable for sum must be singular";
        return {};
      }
      
      std::string target_var = var_ev.terms[0].vars[0].name;
      int start_val = static_cast<int>(std::round(to_double(start_ev)));
      int end_val = static_cast<int>(std::round(to_double(end_ev)));
      
      if (end_val < start_val) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "end must be greater than or equal to start";
        return {};
      }
      if (end_val - start_val > 1000) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "sum range cannot exceed 1000 iterations for performance";
        return {};
      }
      
      ExactValue total = make_exact(0, 1, 1, 2, 0.0);
      for (int i = start_val; i <= end_val; ++i) {
        ASTNode i_node;
        i_node.type = ASTNodeType::LITERAL;
        i_node.value = make_exact(i, 1, 1, 2, static_cast<double>(i));
        state.ast_pool.push_back(i_node);
        int i_idx = state.ast_pool.size() - 1;
        
        int subbed_idx = clone_and_substitute(state, expr_idx, target_var, i_idx);
        ExactValue term = evaluate(state, subbed_idx);
        if (state.error != ParseError::NONE) return {};
        total = add(total, term, state);
      }
      return total;
    }

    if (node.func_name == "taylor") {
      if (node.args.size() != 4) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "taylor requires 4 arguments: taylor(expr, var, center, degree)";
        return {};
      }
      
      int expr_idx = node.args[0];
      ExactValue var_ev = evaluate(state, node.args[1]);
      ExactValue center_ev = evaluate(state, node.args[2]);
      ExactValue degree_ev = evaluate(state, node.args[3]);
      
      if (var_ev.terms.size() != 1 || var_ev.terms[0].vars.size() != 1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "Target variable for taylor must be singular";
        return {};
      }
      
      std::string target_var = var_ev.terms[0].vars[0].name;
      int degree = static_cast<int>(std::round(to_double(degree_ev)));
      
      if (degree < 0 || degree > 25) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "taylor degree must be between 0 and 25";
        return {};
      }
      
      ExactValue total_sum = make_exact(0, 1, 1, 2, 0.0);
      int curr_deriv_idx = expr_idx;
      
      ASTNode center_node;
      center_node.type = ASTNodeType::LITERAL;
      center_node.value = center_ev;
      state.ast_pool.push_back(center_node);
      int center_node_idx = state.ast_pool.size() - 1;
      
      long long factorial = 1;
      
      for (int n = 0; n <= degree; ++n) {
        if (n > 0) factorial *= n;
        
        int subbed_idx = clone_and_substitute(state, curr_deriv_idx, target_var, center_node_idx);
        ExactValue fn_c = evaluate(state, subbed_idx);
        
        if (state.error != ParseError::NONE) return {};
        
        bool is_fn_c_zero = fn_c.terms.empty() && fn_c.symbolic_repr.empty() && std::abs(fn_c.cached_double) < 1e-9;
        if (!is_fn_c_zero) {
          ExactValue fact_ev = make_exact(factorial, 1, 1, 2, static_cast<double>(factorial));
          ExactValue coef = divide(fn_c, fact_ev, state);
          
          ExactValue x_term;
          ExactTerm tv; tv.a = 1; tv.b = 1; tv.c = 1; tv.root_degree = 2;
          tv.vars.push_back({target_var, 1});
          x_term.terms.push_back(tv);
          x_term.cached_double = 0.0;
          
          ExactValue x_minus_c = subtract(x_term, center_ev, state);
          ExactValue power_term = power(x_minus_c, n, state);
          
          ExactValue full_term = multiply(coef, power_term, state);
          total_sum = add(total_sum, full_term, state);
        }
        
        if (n < degree) {
          curr_deriv_idx = differentiate_ast(state, curr_deriv_idx, target_var);
        }
      }
      return total_sum;
    }

    auto check_inequality = [&](char eq_op, double val) -> bool {
      if (std::abs(val) < 1e-9) val = 0.0;
      if (eq_op == '<') return val < 0;
      if (eq_op == '>') return val > 0;
      if (eq_op == 'L') return val <= 0;
      if (eq_op == 'G') return val >= 0;
      return false;
    };

    auto eval_poly_at = [&](const ExactValue &poly, const std::string &var_name, double x_val) -> double {
      double total = 0.0;
      for (const auto &t : poly.terms) {
        double term_val = t.a.to_double() / t.c.to_double() * std::pow(t.b.to_double(), 1.0 / t.root_degree);
        if (t.is_imaginary) continue;
        for (const auto &vp : t.vars) {
          if (vp.name == var_name) {
            term_val *= std::pow(x_val, vp.power);
          }
        }
        total += term_val;
      }
      return total;
    };

    // Linear & Quadratic rational solver

    if (node.func_name == "solve") {
      int eq_idx = -1;
      int var_idx = -1;
      if (node.args.size() >= 1) {
        eq_idx = node.args[0];
        if (node.args.size() >= 2) var_idx = node.args[1];
      } else {
        eq_idx = node.left_idx;
        var_idx = node.right_idx;
      }
      
      if (eq_idx == -1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "solve requires at least 1 argument";
        return {};
      }
      
      ASTNode left_arg = state.ast_pool[eq_idx];
      ASTNode right_arg;
      if (var_idx != -1) {
        right_arg = state.ast_pool[var_idx];
      } else {
        right_arg.type = ASTNodeType::LITERAL;
        ExactValue xv;
        ExactTerm t; t.a = 1; t.b = 1; t.c = 1; t.root_degree = 2;
        t.vars.push_back({"x", 1});
        xv.terms.push_back(t);
        xv.simplify();
        right_arg.value = xv;
        
        state.ast_pool.push_back(right_arg);
        var_idx = state.ast_pool.size() - 1;
        right_arg = state.ast_pool.back();
      }

      if (left_arg.type == ASTNodeType::ARRAY &&
          right_arg.type == ASTNodeType::ARRAY) {
        int num_eqs = left_arg.args.size();
        int num_vars = right_arg.args.size();

        if (num_eqs != num_vars) {
          state.error = ParseError::UNSUPPORTED_OPERATION;
          state.error_extra =
              "Number of equations must match number of variables";
          return {};
        }

        std::vector<std::string> vars;
        for (int i = 0; i < num_vars; ++i) {
          ExactValue var_ev = evaluate(state, right_arg.args[i]);
          if (var_ev.terms.size() == 1 && var_ev.terms[0].vars.size() == 1) {
            vars.push_back(var_ev.terms[0].vars[0].name);
          } else {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra = "Target variables must be singular unknowns";
            return {};
          }
        }

        std::vector<ExactValue> eq_vals;
        bool is_linear = true;
        bool is_polynomial = true;

        for (int i = 0; i < num_eqs; ++i) {
          int eq_idx = simplify_ast(state, left_arg.args[i]);
          ASTNode eq_node = state.ast_pool[eq_idx];
          if (eq_node.type != ASTNodeType::BINARY || eq_node.op != '=') {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra =
                "Array elements must be equations (e.g. x + y = 5)";
            return {};
          }

          ExactValue l_val = evaluate(state, eq_node.left_idx);
          ExactValue r_val = evaluate(state, eq_node.right_idx);
          ExactValue eq_val = subtract(l_val, r_val, state);
          eq_vals.push_back(eq_val);

          for (const auto &term : eq_val.terms) {
            if (term.c != 1 || term.is_imaginary || term.root_degree != 2)
              is_polynomial = false;

            int degree_sum = 0;
            for (const auto &vp : term.vars)
              degree_sum += vp.power;
            if (degree_sum > 1)
              is_linear = false;
          }
        }

        if (is_linear) {
          Matrix mat(num_eqs, num_vars + 1);
          for (int i = 0; i < num_eqs; ++i) {
            ExactValue eq_val = eq_vals[i];
            ExactValue c;
            c.cached_double = 0.0;
            std::vector<ExactValue> coeffs(num_vars,
                                           make_exact(0, 1, 1, 2, 0.0));

            for (const auto &term : eq_val.terms) {
              bool found = false;
              for (int j = 0; j < num_vars; ++j) {
                ExactTerm t = term;
                auto it = std::find_if(t.vars.begin(), t.vars.end(),
                                       [&](const VariablePower &vp) {
                                         return vp.name == vars[j];
                                       });
                if (it != t.vars.end()) {
                  t.vars.erase(it);
                  ExactValue coeff_val;
                  if (t.a != 0) {
                    coeff_val.terms.push_back(t);
                    coeff_val.simplify();
                  }
                  coeffs[j] = add(coeffs[j], coeff_val, state);
                  found = true;
                  break;
                }
              }
              if (!found) {
                ExactValue const_val;
                const_val.terms.push_back(term);
                const_val.simplify();
                c = add(c, const_val, state);
              }
            }

            for (int j = 0; j < num_vars; ++j) {
              mat.set(i, j, coeffs[j]);
            }
            ExactValue zero = make_exact(0, 1, 1, 2, 0.0);
            mat.set(i, num_vars, subtract(zero, c, state));
          }

          if (state.error != ParseError::NONE)
            return {};
          mat.rref(state);

          ExactValue res;
          res.symbolic_repr = "System Solutions: ";
          for (int i = 0; i < num_vars; ++i) {
            res.symbolic_repr +=
                vars[i] + " = " + to_exact_string(mat.get(i, num_vars));
            if (i < num_vars - 1)
              res.symbolic_repr += ", ";
          }
          return res;
        }

        if (is_polynomial) {
          std::vector<Polynomial> P;
          for (const auto &ev : eq_vals)
            P.push_back(Polynomial(ev));

          try {
            std::vector<Polynomial> G = buchberger(P);
            ExactValue res;
            res.symbolic_repr = "";
            if (num_vars == 2 && G.size() >= 2) {
              int one_var_idx = -1;
              std::string one_var_name = "";
              for (size_t i = 0; i < G.size(); ++i) {
                std::vector<std::string> p_vars;
                for (const auto &t : G[i].terms) {
                  for (const auto &v : t.vars) {
                    if (v.second > 0) {
                      bool found = false;
                      for (const auto &existing : p_vars)
                        if (existing == v.first)
                          found = true;
                      if (!found)
                        p_vars.push_back(v.first);
                    }
                  }
                }
                if (p_vars.size() == 1) {
                  one_var_idx = i;
                  one_var_name = p_vars[0];
                  break;
                }
              }

              if (one_var_idx != -1) {
                std::string eq1 = G[one_var_idx].to_string() + " = 0";
                auto old_tokens = state.tokens;
                auto old_idx = state.token_idx;
                state.tokens.clear();
                state.token_idx = 0;
                tokenize("solve(" + eq1 + ", " + one_var_name + ")", state);
                int sub_ast_idx = parse_expression(state, 0);
                state.tokens = old_tokens;
                state.token_idx = old_idx;

                ExactValue sol1 = evaluate(state, sub_ast_idx);

                if (state.error == ParseError::NONE) {
                  std::vector<std::string> roots;
                  std::string s_repr = sol1.symbolic_repr;
                  size_t pos = 0;
                  while ((pos = s_repr.find(one_var_name + " = ")) !=
                         std::string::npos) {
                    s_repr = s_repr.substr(pos + one_var_name.length() + 3);
                    size_t comma = s_repr.find(" , ");
                    if (comma != std::string::npos) {
                      roots.push_back(s_repr.substr(0, comma));
                      s_repr = s_repr.substr(comma + 3);
                    } else {
                      roots.push_back(s_repr);
                      break;
                    }
                  }

                  std::string other_var =
                      (vars[0] == one_var_name) ? vars[1] : vars[0];
                  int two_var_idx = -1;
                  for (int i = G.size() - 1; i >= 0; --i) {
                    if (i == one_var_idx)
                      continue;
                    std::vector<std::string> p_vars;
                    for (const auto &t : G[i].terms) {
                      for (const auto &v : t.vars) {
                        if (v.second > 0) {
                          bool found = false;
                          for (const auto &existing : p_vars)
                            if (existing == v.first)
                              found = true;
                          if (!found)
                            p_vars.push_back(v.first);
                        }
                      }
                    }
                    bool has_one = false, has_other = false;
                    for (const auto &v : p_vars) {
                      if (v == one_var_name)
                        has_one = true;
                      if (v == other_var)
                        has_other = true;
                    }
                    if (has_one && has_other) {
                      two_var_idx = i;
                      break;
                    }
                  }

                  if (two_var_idx != -1 && roots.size() > 0) {
                    std::string final_out = "System Solutions:\n";
                    for (size_t r = 0; r < roots.size(); ++r) {
                      std::string eq2 = G[two_var_idx].to_string() + " = 0";

                      auto t2 = state.tokens;
                      auto idx2 = state.token_idx;
                      state.tokens.clear();
                      state.token_idx = 0;
                      tokenize(roots[r], state);
                      int root_ast = parse_expression(state, 0);
                      state.tokens.clear();
                      state.token_idx = 0;
                      tokenize(eq2, state);
                      int eq2_ast = parse_expression(state, 0);
                      state.tokens = t2;
                      state.token_idx = idx2;

                      int subbed_eq = clone_and_substitute(
                          state, eq2_ast, one_var_name, root_ast);

                      ASTNode solve2;
                      solve2.type = ASTNodeType::FUNCTION;
                      solve2.func_name = "solve";
                      solve2.left_idx = subbed_eq;

                      ASTNode target_node;
                      target_node.type = ASTNodeType::LITERAL;
                      ExactTerm tv;
                      tv.a = 1;
                      tv.b = 1;
                      tv.c = 1;
                      tv.root_degree = 2;
                      tv.vars.push_back({other_var, 1});
                      target_node.value.terms.push_back(tv);
                      state.ast_pool.push_back(target_node);
                      solve2.right_idx = state.ast_pool.size() - 1;

                      state.ast_pool.push_back(solve2);
                      int solve2_idx = state.ast_pool.size() - 1;

                      ExactValue ans2 = evaluate(state, solve2_idx);

                      // Ans2 should be a list like "x = 4 , x = 5"
                      std::string a2_str = ans2.symbolic_repr;
                      size_t p2 = 0;
                      std::vector<std::string> roots2;
                      while ((p2 = a2_str.find(other_var + " = ")) !=
                             std::string::npos) {
                        a2_str = a2_str.substr(p2 + other_var.length() + 3);
                        size_t comma = a2_str.find(" , ");
                        if (comma != std::string::npos) {
                          roots2.push_back(a2_str.substr(0, comma));
                          a2_str = a2_str.substr(comma + 3);
                        } else {
                          roots2.push_back(a2_str);
                          break;
                        }
                      }

                      for (size_t r2 = 0; r2 < roots2.size(); ++r2) {
                        final_out += "(" + one_var_name + " = " + roots[r] +
                                     ", " + other_var + " = " + roots2[r2] +
                                     ")\n";
                      }
                    }
                    res.symbolic_repr = final_out;
                    return res;
                  }
                }
              }
            }

            res.symbolic_repr =
                "Gr\xc3\xb6"
                "bner Basis (Multivariate Polynomial Solutions):\n";
            for (size_t i = 0; i < G.size(); ++i) {
              res.symbolic_repr += G[i].to_string() + " = 0";
              if (i < G.size() - 1)
                res.symbolic_repr += "\n";
            }
            return res;
          } catch (const std::exception &e) {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra = e.what();
            return {};
          }
        }

        // Non-linear fallback for 2 equations using substitution
        if (num_eqs == 2 && num_vars == 2) {
          ASTNode solve_eq1;
          solve_eq1.type = ASTNodeType::FUNCTION;
          solve_eq1.func_name = "solve";
          solve_eq1.left_idx = left_arg.args[0];
          solve_eq1.right_idx = right_arg.args[0];
          state.ast_pool.push_back(solve_eq1);
          int solve1_idx = state.ast_pool.size() - 1;

          ExactValue eq1_sol = evaluate(state, solve1_idx);
          if (state.error != ParseError::NONE)
            return {};

          std::string rhs = "";
          size_t eq_pos = eq1_sol.symbolic_repr.find("=");
          if (eq_pos != std::string::npos) {
            size_t comma_pos = eq1_sol.symbolic_repr.find(",");
            if (comma_pos != std::string::npos) {
              rhs = eq1_sol.symbolic_repr.substr(eq_pos + 1,
                                                 comma_pos - eq_pos - 1);
            } else {
              rhs = eq1_sol.symbolic_repr.substr(eq_pos + 1);
            }
          } else {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra = "Failed to solve non-linear substitution";
            return {};
          }

          auto old_tokens = state.tokens;
          auto old_idx = state.token_idx;
          state.tokens.clear();
          state.token_idx = 0;
          tokenize(rhs, state);
          int sub_ast_idx = parse_expression(state, 0);
          state.tokens = old_tokens;
          state.token_idx = old_idx;

          int new_eq2_idx = clone_and_substitute(state, left_arg.args[1],
                                                 vars[0], sub_ast_idx);

          ASTNode solve_eq2;
          solve_eq2.type = ASTNodeType::FUNCTION;
          solve_eq2.func_name = "solve";
          solve_eq2.left_idx = new_eq2_idx;
          solve_eq2.right_idx = right_arg.args[1];
          state.ast_pool.push_back(solve_eq2);
          int solve2_idx = state.ast_pool.size() - 1;

          ExactValue eq2_sol = evaluate(state, solve2_idx);
          if (state.error != ParseError::NONE)
            return {};

          ExactValue res;
          res.symbolic_repr = "Non-Linear Substitution System Solutions:\n";
          res.symbolic_repr += "Using " + vars[0] + " = " + rhs + "\n";
          res.symbolic_repr += eq2_sol.symbolic_repr;
          return res;
        }

        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "Non-linear systems of >2 equations not supported "
                            "without Buchberger";
        return {};
      }

      ExactValue var_val = evaluate(state, var_idx);
      std::string target_var = "";
      if (var_val.terms.size() == 1 && var_val.terms[0].vars.size() == 1 &&
          var_val.terms[0].a == 1 && var_val.terms[0].b == 1 &&
          var_val.terms[0].c == 1 && var_val.terms[0].root_degree == 2) {
        target_var = var_val.terms[0].vars[0].name;
      } else {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "second argument to solve must be a single variable (e.g., x)";
        return {};
      }

      eq_idx = simplify_ast(state, eq_idx);

      char eq_op = '=';
      const ASTNode &eq_node = state.ast_pool[eq_idx];
      if (eq_node.type == ASTNodeType::BINARY && (eq_node.op == '=' || eq_node.op == '<' || eq_node.op == '>' || eq_node.op == 'L' || eq_node.op == 'G')) {
        eq_op = eq_node.op;
        const ASTNode &left = state.ast_pool[eq_node.left_idx];
        const ASTNode &right = state.ast_pool[eq_node.right_idx];
        if (left.type == ASTNodeType::FUNCTION &&
            right.type == ASTNodeType::FUNCTION) {
          if (left.func_name == right.func_name &&
              (left.func_name == "ln" || left.func_name == "log" ||
               left.func_name.starts_with("log"))) {
            ASTNode stripped_eq;
            stripped_eq.type = ASTNodeType::BINARY;
            stripped_eq.op = eq_op;
            stripped_eq.left_idx = left.right_idx;
            stripped_eq.right_idx = right.right_idx;
            state.ast_pool.push_back(stripped_eq);
            eq_idx = state.ast_pool.size() - 1;
          }
        }
      }

      current_target_var = target_var;
      ExactValue eq_val = evaluate(state, eq_idx);
      current_target_var = ""; // Reset target variable configuration

      if (state.error != ParseError::NONE)
        return {};

      // Clear division/rational relationships automatically by solving
      // Numerator = 0
      RationalValue rv = get_rational_form(eq_val, state);
      ExactValue solve_target = rv.num;
      ExactValue den_target = rv.den;

      // Phase 3: Radical Equation Expansion
      std::string root_var = "";
      for (const auto &term : solve_target.terms) {
        for (const auto &vp : term.vars) {
          if (vp.name.find("root(") == 0 || vp.name.find("√") == 0) {
            root_var = vp.name;
            break;
          }
        }
        if (!root_var.empty())
          break;
      }

      if (!root_var.empty() && target_var != root_var) {
        ExactValue a_root, b_root, c_root;
        a_root.cached_double = 0;
        b_root.cached_double = 0;
        c_root.cached_double = 0;
        for (const auto &term : solve_target.terms) {
          int p = 0;
          ExactTerm t = term;
          auto it = std::find_if(
              t.vars.begin(), t.vars.end(),
              [&](const VariablePower &vp) { return vp.name == root_var; });
          if (it != t.vars.end()) {
            p = it->power;
            t.vars.erase(it);
          }

          ExactValue temp;
          if (t.a != 0) {
            temp.terms.push_back(t);
            temp.simplify();
          }
          if (p == 0)
            c_root = add(c_root, temp, state);
          else if (p == 1)
            b_root = add(b_root, temp, state);
          else if (p == 2)
            a_root = add(a_root, temp, state);
        }

        if (a_root.terms.empty() && a_root.symbolic_repr.empty()) {
          ExactValue zero = make_exact(0, 1, 1, 2, 0);
          ExactValue minus_c = subtract(zero, c_root, state);
          ExactValue isolated_root = divide(minus_c, b_root, state);

          ExactValue squared_rhs = power(isolated_root, 2, state);

          std::string inside_str = root_var;
          if (inside_str.find("root(") == 0) {
            inside_str = inside_str.substr(5, inside_str.length() - 6);
          } else if (inside_str.find("√") == 0) {
            if (inside_str.length() > 3 && inside_str[3] == '(' &&
                inside_str.back() == ')') {
              inside_str = inside_str.substr(4, inside_str.length() - 5);
            } else {
              inside_str = inside_str.substr(3);
            }
          }

          std::string new_eq_str =
              inside_str + " = " + to_exact_string(squared_rhs);

          auto old_tokens = state.tokens;
          auto old_idx = state.token_idx;
          state.tokens.clear();
          state.token_idx = 0;
          tokenize("solve(" + new_eq_str + ", " + target_var + ")", state);
          int sub_ast_idx = parse_expression(state, 0);
          state.tokens = old_tokens;
          state.token_idx = old_idx;

          ExactValue final_res = evaluate(state, sub_ast_idx);

          ExactValue combined_res;
          combined_res.symbolic_repr =
              "Radical Expansion System Solution:\nExpanded: " + new_eq_str +
              "\n" + final_res.symbolic_repr;
          return combined_res;
        }
      }

      // Phase 2: Trigonometric Identity Substitution & U-Substitution

      bool has_sin = false, has_cos = false, sin_odd = false, cos_odd = false;
      bool has_tan = false, has_sec = false, tan_odd = false, sec_odd = false;
      bool has_cot = false, has_csc = false, cot_odd = false, csc_odd = false;

      std::string sin_var = "sin(" + target_var + ")";
      std::string cos_var = "cos(" + target_var + ")";
      std::string tan_var = "tan(" + target_var + ")";
      std::string sec_var = "sec(" + target_var + ")";
      std::string cot_var = "cot(" + target_var + ")";
      std::string csc_var = "csc(" + target_var + ")";

      for (const auto &term : solve_target.terms) {
        for (const auto &vp : term.vars) {
          if (vp.name == sin_var) {
            has_sin = true;
            if (vp.power % 2 != 0)
              sin_odd = true;
          }
          if (vp.name == cos_var) {
            has_cos = true;
            if (vp.power % 2 != 0)
              cos_odd = true;
          }
          if (vp.name == tan_var) {
            has_tan = true;
            if (vp.power % 2 != 0)
              tan_odd = true;
          }
          if (vp.name == sec_var) {
            has_sec = true;
            if (vp.power % 2 != 0)
              sec_odd = true;
          }
          if (vp.name == cot_var) {
            has_cot = true;
            if (vp.power % 2 != 0)
              cot_odd = true;
          }
          if (vp.name == csc_var) {
            has_csc = true;
            if (vp.power % 2 != 0)
              csc_odd = true;
          }
        }
      }

      if (has_sin && has_cos) {
        if (!sin_odd) {
          ExactValue new_eq;
          new_eq.cached_double = 0.0;
          for (const auto &term : solve_target.terms) {
            int p = 0;
            ExactTerm t = term;
            auto it = std::find_if(
                t.vars.begin(), t.vars.end(),
                [&](const VariablePower &vp) { return vp.name == sin_var; });
            if (it != t.vars.end()) {
              p = it->power;
              t.vars.erase(it);
            }

            ExactValue term_val;
            if (t.a != 0) {
              term_val.terms.push_back(t);
              term_val.simplify();
            } else {
              term_val.cached_double = 0;
            }

            if (p > 0) {
              ExactValue one = make_exact(1, 1, 1, 2, 0);
              ExactValue cos2;
              ExactTerm ct;
              ct.a = 1;
              ct.b = 1;
              ct.c = 1;
              ct.root_degree = 2;
              ct.vars.push_back({cos_var, 2});
              cos2.terms.push_back(ct);
              cos2.simplify();
              ExactValue sub = subtract(one, cos2, state);
              ExactValue sub_p = power(sub, p / 2, state);
              term_val = multiply(term_val, sub_p, state);
            }
            new_eq = add(new_eq, term_val, state);
          }
          solve_target = new_eq;
          has_sin = false; // Eliminated sin
        } else if (!cos_odd) {
          ExactValue new_eq;
          new_eq.cached_double = 0.0;
          for (const auto &term : solve_target.terms) {
            int p = 0;
            ExactTerm t = term;
            auto it = std::find_if(
                t.vars.begin(), t.vars.end(),
                [&](const VariablePower &vp) { return vp.name == cos_var; });
            if (it != t.vars.end()) {
              p = it->power;
              t.vars.erase(it);
            }

            ExactValue term_val;
            if (t.a != 0) {
              term_val.terms.push_back(t);
              term_val.simplify();
            } else {
              term_val.cached_double = 0;
            }

            if (p > 0) {
              ExactValue one = make_exact(1, 1, 1, 2, 0);
              ExactValue sin2;
              ExactTerm st;
              st.a = 1;
              st.b = 1;
              st.c = 1;
              st.root_degree = 2;
              st.vars.push_back({sin_var, 2});
              sin2.terms.push_back(st);
              sin2.simplify();
              ExactValue sub = subtract(one, sin2, state);
              ExactValue sub_p = power(sub, p / 2, state);
              term_val = multiply(term_val, sub_p, state);
            }
            new_eq = add(new_eq, term_val, state);
          }
          solve_target = new_eq;
          has_cos = false; // Eliminated cos
        }
      } else if (has_tan && has_sec) {
        if (!tan_odd) { // tan^2 = sec^2 - 1
          ExactValue new_eq;
          new_eq.cached_double = 0.0;
          for (const auto &term : solve_target.terms) {
            int p = 0;
            ExactTerm t = term;
            auto it = std::find_if(
                t.vars.begin(), t.vars.end(),
                [&](const VariablePower &vp) { return vp.name == tan_var; });
            if (it != t.vars.end()) {
              p = it->power;
              t.vars.erase(it);
            }

            ExactValue term_val;
            if (t.a != 0) {
              term_val.terms.push_back(t);
              term_val.simplify();
            } else {
              term_val.cached_double = 0;
            }

            if (p > 0) {
              ExactValue minus_one = make_exact(-1, 1, 1, 2, 0);
              ExactValue sec2;
              ExactTerm ct;
              ct.a = 1;
              ct.b = 1;
              ct.c = 1;
              ct.root_degree = 2;
              ct.vars.push_back({sec_var, 2});
              sec2.terms.push_back(ct);
              sec2.simplify();
              ExactValue sub = add(sec2, minus_one, state);
              ExactValue sub_p = power(sub, p / 2, state);
              term_val = multiply(term_val, sub_p, state);
            }
            new_eq = add(new_eq, term_val, state);
          }
          solve_target = new_eq;
          has_tan = false;
        } else if (!sec_odd) { // sec^2 = tan^2 + 1
          ExactValue new_eq;
          new_eq.cached_double = 0.0;
          for (const auto &term : solve_target.terms) {
            int p = 0;
            ExactTerm t = term;
            auto it = std::find_if(
                t.vars.begin(), t.vars.end(),
                [&](const VariablePower &vp) { return vp.name == sec_var; });
            if (it != t.vars.end()) {
              p = it->power;
              t.vars.erase(it);
            }

            ExactValue term_val;
            if (t.a != 0) {
              term_val.terms.push_back(t);
              term_val.simplify();
            } else {
              term_val.cached_double = 0;
            }

            if (p > 0) {
              ExactValue one = make_exact(1, 1, 1, 2, 0);
              ExactValue tan2;
              ExactTerm st;
              st.a = 1;
              st.b = 1;
              st.c = 1;
              st.root_degree = 2;
              st.vars.push_back({tan_var, 2});
              tan2.terms.push_back(st);
              tan2.simplify();
              ExactValue sub = add(one, tan2, state);
              ExactValue sub_p = power(sub, p / 2, state);
              term_val = multiply(term_val, sub_p, state);
            }
            new_eq = add(new_eq, term_val, state);
          }
          solve_target = new_eq;
          has_sec = false;
        }
      } else if (has_cot && has_csc) {
        if (!cot_odd) { // cot^2 = csc^2 - 1
          ExactValue new_eq;
          new_eq.cached_double = 0.0;
          for (const auto &term : solve_target.terms) {
            int p = 0;
            ExactTerm t = term;
            auto it = std::find_if(
                t.vars.begin(), t.vars.end(),
                [&](const VariablePower &vp) { return vp.name == cot_var; });
            if (it != t.vars.end()) {
              p = it->power;
              t.vars.erase(it);
            }

            ExactValue term_val;
            if (t.a != 0) {
              term_val.terms.push_back(t);
              term_val.simplify();
            } else {
              term_val.cached_double = 0;
            }

            if (p > 0) {
              ExactValue minus_one = make_exact(-1, 1, 1, 2, 0);
              ExactValue csc2;
              ExactTerm ct;
              ct.a = 1;
              ct.b = 1;
              ct.c = 1;
              ct.root_degree = 2;
              ct.vars.push_back({csc_var, 2});
              csc2.terms.push_back(ct);
              csc2.simplify();
              ExactValue sub = add(csc2, minus_one, state);
              ExactValue sub_p = power(sub, p / 2, state);
              term_val = multiply(term_val, sub_p, state);
            }
            new_eq = add(new_eq, term_val, state);
          }
          solve_target = new_eq;
          has_cot = false;
        } else if (!csc_odd) { // csc^2 = cot^2 + 1
          ExactValue new_eq;
          new_eq.cached_double = 0.0;
          for (const auto &term : solve_target.terms) {
            int p = 0;
            ExactTerm t = term;
            auto it = std::find_if(
                t.vars.begin(), t.vars.end(),
                [&](const VariablePower &vp) { return vp.name == csc_var; });
            if (it != t.vars.end()) {
              p = it->power;
              t.vars.erase(it);
            }

            ExactValue term_val;
            if (t.a != 0) {
              term_val.terms.push_back(t);
              term_val.simplify();
            } else {
              term_val.cached_double = 0;
            }

            if (p > 0) {
              ExactValue one = make_exact(1, 1, 1, 2, 0);
              ExactValue cot2;
              ExactTerm st;
              st.a = 1;
              st.b = 1;
              st.c = 1;
              st.root_degree = 2;
              st.vars.push_back({cot_var, 2});
              cot2.terms.push_back(st);
              cot2.simplify();
              ExactValue sub = add(one, cot2, state);
              ExactValue sub_p = power(sub, p / 2, state);
              term_val = multiply(term_val, sub_p, state);
            }
            new_eq = add(new_eq, term_val, state);
          }
          solve_target = new_eq;
          has_csc = false;
        }
      }

      // U-Substitution: If target_var is missing, but a functional wrapper
      // exists
      bool target_found = false;
      for (const auto &term : solve_target.terms) {
        for (const auto &vp : term.vars) {
          if (vp.name == target_var)
            target_found = true;
        }
      }
      if (!target_found) {
        if (has_cos && !has_sin)
          target_var = cos_var;
        else if (has_sin && !has_cos)
          target_var = sin_var;
        else if (has_sec && !has_tan)
          target_var = sec_var;
        else if (has_tan && !has_sec)
          target_var = tan_var;
        else if (has_csc && !has_cot)
          target_var = csc_var;
        else if (has_cot && !has_csc)
          target_var = cot_var;
      }

      auto get_poly_roots = [&](ExactValue target, const std::string& var_name) -> std::vector<ExactValue> {
        std::vector<ExactValue> rts;
        std::map<int, ExactValue> coeffs_map;
        int max_deg = 0;
        
        for (const auto &term : target.terms) {
          int power = 0; ExactTerm new_term = term;
          auto it = std::find_if(new_term.vars.begin(), new_term.vars.end(), [&](const VariablePower &vp) { return vp.name == var_name; });
          if (it != new_term.vars.end()) { power = it->power; new_term.vars.erase(it); }
          ExactValue temp;
          if (new_term.a != 0) { temp.terms.push_back(new_term); temp.simplify(); }
          
          if (power > max_deg) max_deg = power;
          if (coeffs_map.find(power) == coeffs_map.end()) coeffs_map[power] = make_exact(0, 1, 1, 2, 0.0);
          coeffs_map[power] = add(coeffs_map[power], temp, state);
        }
        
        ExactValue a = coeffs_map[2];
        ExactValue b = coeffs_map[1];
        ExactValue c = coeffs_map[0];
        
        if (max_deg <= 2) {
          bool is_a_zero = (a.terms.empty() && a.symbolic_repr.empty() && a.cached_double == 0.0);
          bool is_b_zero = (b.terms.empty() && b.symbolic_repr.empty() && b.cached_double == 0.0);
          ExactValue zero = make_exact(0, 1, 1, 2, 0.0);
          if (is_a_zero) {
            if (!is_b_zero) rts.push_back(divide(subtract(zero, c, state), b, state));
          } else {
            ExactValue b_sq = power(b, 2, state);
            ExactValue four_ac = multiply(make_exact(4, 1, 1, 2, 4.0), multiply(a, c, state), state);
            ExactValue disc = subtract(b_sq, four_ac, state);
            ExactValue root_disc = compute_root(disc, 2, state);
            if (state.error != ParseError::NONE) return {};
            ExactValue minus_b = subtract(zero, b, state);
            ExactValue two_a = multiply(make_exact(2, 1, 1, 2, 2.0), a, state);
            rts.push_back(divide(add(minus_b, root_disc, state), two_a, state));
            rts.push_back(divide(subtract(minus_b, root_disc, state), two_a, state));
          }
          return rts;
        }
        
        // Durand-Kerner for max_deg >= 3
        std::vector<std::complex<double>> c_coeffs(max_deg + 1, {0.0, 0.0});
        for (auto const& [deg, ev] : coeffs_map) {
            c_coeffs[deg] = {to_double(ev), ev.cached_imag};
        }
        
        // Normalize leading coefficient
        std::complex<double> lead = c_coeffs[max_deg];
        if (std::abs(lead) < 1e-12) {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra = "Leading coefficient evaluates to zero numerically.";
            return {};
        }
        for (int i = 0; i <= max_deg; ++i) c_coeffs[i] /= lead;
        
        int n = max_deg;
        std::vector<std::complex<double>> roots(n);
        std::complex<double> z(0.4, 0.9);
        for (int i = 0; i < n; ++i) roots[i] = std::pow(z, i);
        
        auto eval_poly = [&](std::complex<double> x) {
            std::complex<double> res = 0;
            for (int i = 0; i <= n; ++i) res += c_coeffs[i] * std::pow(x, i);
            return res;
        };
        
        for (int iter = 0; iter < 1000; ++iter) {
            std::vector<std::complex<double>> next_roots = roots;
            double max_err = 0;
            for (int k = 0; k < n; ++k) {
                std::complex<double> den = 1.0;
                for (int j = 0; j < n; ++j) {
                    if (j != k) den *= (roots[k] - roots[j]);
                }
                std::complex<double> num = eval_poly(roots[k]);
                next_roots[k] = roots[k] - num / den;
                max_err = std::max(max_err, std::abs(next_roots[k] - roots[k]));
            }
            roots = next_roots;
            if (max_err < 1e-10) break;
        }
        
        for (int i = 0; i < n; ++i) {
            ExactValue rv;
            rv.cached_double = roots[i].real();
            rv.cached_imag = roots[i].imag();
            if (std::abs(rv.cached_double) < 1e-9) rv.cached_double = 0.0;
            if (std::abs(rv.cached_imag) < 1e-9) rv.cached_imag = 0.0;
            rv.is_approx = true;
            rts.push_back(rv);
        }
        
        return rts;
      };

      auto parse_opaque_vars = [&](ExactValue ev) -> ExactValue {
          if (ev.terms.empty()) return ev;
          ExactValue parsed_ev = make_exact(0, 1, 1, 2, 0.0);
          for (const auto& term : ev.terms) {
              ExactValue term_val = make_exact(1, 1, 1, 2, 1.0);
              for (const auto& vp : term.vars) {
                  std::string name = vp.name;
                  if (name.front() == '(' && name.back() == ')') {
                      name = name.substr(1, name.length() - 2);
                      auto old_tokens = state.tokens; auto old_idx = state.token_idx;
                      state.tokens.clear(); state.token_idx = 0;
                      tokenize(name, state);
                      int sub_ast_idx = parse_expression(state, 0);
                      ExactValue parsed_val = evaluate(state, sub_ast_idx);
                      state.tokens = old_tokens; state.token_idx = old_idx;
                      term_val = multiply(term_val, power(parsed_val, vp.power, state), state);
                  } else {
                      ExactValue sym_var; ExactTerm st;
                      st.a = 1; st.b = 1; st.c = 1; st.root_degree = 2; st.vars.push_back(vp);
                      sym_var.terms.push_back(st); sym_var.simplify();
                      term_val = multiply(term_val, sym_var, state);
                  }
              }
              ExactTerm coef_t = term;
              coef_t.vars.clear();
              ExactValue coef_ev; coef_ev.terms.push_back(coef_t);
              term_val = multiply(term_val, coef_ev, state);
              parsed_ev = add(parsed_ev, term_val, state);
          }
          return parsed_ev;
      };

      ExactValue parsed_solve_target = parse_opaque_vars(solve_target);
      ExactValue parsed_den_target = parse_opaque_vars(den_target);

      std::vector<ExactValue> num_roots = get_poly_roots(parsed_solve_target, target_var);
      if (state.error != ParseError::NONE) return {};
      std::vector<ExactValue> den_roots;
      if (!parsed_den_target.terms.empty() && parsed_den_target.symbolic_repr.empty()) {
          bool is_den_const = parsed_den_target.terms.size() == 1 && parsed_den_target.terms[0].vars.empty();
          if (!is_den_const) den_roots = get_poly_roots(parsed_den_target, target_var);
      }

      
      num_roots.erase(std::remove_if(num_roots.begin(), num_roots.end(), [](const ExactValue& ev) { return std::abs(ev.cached_imag) > 1e-9; }), num_roots.end());
      den_roots.erase(std::remove_if(den_roots.begin(), den_roots.end(), [](const ExactValue& ev) { return std::abs(ev.cached_imag) > 1e-9; }), den_roots.end());
      
      if (eq_op == '=') {
          std::vector<ExactValue> valid_roots;
          for (const auto& nr : num_roots) {
              bool ok = true;
              for (const auto& dr : den_roots) {
                  if (std::abs(nr.cached_double - dr.cached_double) < 1e-9) ok = false;
              }
              if (ok) valid_roots.push_back(nr);
          }
          if (valid_roots.empty()) {
              ExactValue res; res.symbolic_repr = "No solution"; return res;
          }
          ExactValue res; res.symbolic_repr = "";
          
          std::string core_var = target_var;
          std::string outer_func = "";
          size_t paren_pos = target_var.find('(');
          if (paren_pos != std::string::npos && target_var.back() == ')') {
            outer_func = target_var.substr(0, paren_pos);
            core_var = target_var.substr(paren_pos + 1, target_var.length() - paren_pos - 2);
          }

          for (size_t i = 0; i < valid_roots.size(); ++i) {
              if (outer_func == "cos" || outer_func == "sin" || outer_func == "tan" || outer_func == "sec" || outer_func == "csc" || outer_func == "cot") {
                  res.symbolic_repr += core_var + " = " + outer_func + "^-1(" + to_exact_string(valid_roots[i]) + ")";
              } else {
                  res.symbolic_repr += target_var + " = " + to_exact_string(valid_roots[i]);
              }
              if (i < valid_roots.size() - 1) res.symbolic_repr += " , ";
          }
          return res;
      }
      
      std::vector<ExactValue> critical_points = num_roots;
      critical_points.insert(critical_points.end(), den_roots.begin(), den_roots.end());
      std::sort(critical_points.begin(), critical_points.end(), [](const ExactValue& a, const ExactValue& b) { return a.cached_double < b.cached_double; });
      
      std::vector<double> test_pts;
      if (critical_points.empty()) test_pts.push_back(0.0);
      else {
          test_pts.push_back(critical_points.front().cached_double - 1.0);
          for (size_t i = 0; i < critical_points.size() - 1; ++i) {
              test_pts.push_back((critical_points[i].cached_double + critical_points[i+1].cached_double) / 2.0);
          }
          test_pts.push_back(critical_points.back().cached_double + 1.0);
      }
      
      std::string ineq_str = "";
      auto format_pt = [&](int idx, bool is_left) -> std::string {
          const ExactValue& pt = critical_points[idx];
          bool is_den = false;
          for (const auto& dr : den_roots) {
              if (std::abs(dr.cached_double - pt.cached_double) < 1e-9) is_den = true;
          }
          if (eq_op == 'L' || eq_op == 'G') {
              if (is_den) return is_left ? ("< " + to_exact_string(pt)) : (to_exact_string(pt) + " <");
              else return is_left ? ("<= " + to_exact_string(pt)) : (to_exact_string(pt) + " <=");
          } else {
              return is_left ? ("< " + to_exact_string(pt)) : (to_exact_string(pt) + " <");
          }
      };

      for (size_t i = 0; i < test_pts.size(); ++i) {
          double num_val = eval_poly_at(parsed_solve_target, target_var, test_pts[i]);
          double den_val = eval_poly_at(parsed_den_target, target_var, test_pts[i]);
          bool valid = check_inequality(eq_op, den_val == 0 ? 0 : num_val / den_val);
          if (valid) {
              if (!ineq_str.empty()) ineq_str += " or ";
              if (test_pts.size() == 1) {
                  ineq_str = "All real numbers";
              } else if (i == 0) {
                  ineq_str += target_var + " " + format_pt(0, true);
              } else if (i == test_pts.size() - 1) {
                  std::string right_sym = (eq_op == 'L' || eq_op == 'G') ? ">=" : ">";
                  bool is_den = false;
                  for (const auto& dr : den_roots) if (std::abs(dr.cached_double - critical_points.back().cached_double) < 1e-9) is_den = true;
                  if ((eq_op == 'L' || eq_op == 'G') && is_den) right_sym = ">";
                  ineq_str += target_var + " " + right_sym + " " + to_exact_string(critical_points.back());
              } else {
                  ineq_str += format_pt(i-1, false) + " " + target_var + " " + format_pt(i, true);
              }
          }
      }
      if (ineq_str.empty()) ineq_str = "No solution";
      ExactValue res; res.symbolic_repr = ineq_str;
      return res;
    }

    if (node.func_name == "approx") {
      ExactValue arg = evaluate(state, node.right_idx);
      ExactValue res;
      res.cached_double = arg.cached_double;
      res.cached_imag = arg.cached_imag;
      res.is_approx = true;
      return res;
    }

    if (node.func_name == "round") {
      if (node.left_idx == -1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "round requires 2 arguments: round(decimals, value)";
        return {};
      }
      ExactValue decimals_ev = evaluate(state, node.left_idx);
      ExactValue val_ev = evaluate(state, node.right_idx);

      double decimals_dbl = std::round(to_double(decimals_ev));
      if (decimals_dbl < 0 || decimals_dbl > 15) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "decimals must be between 0 and 15";
        return {};
      }

      double multiplier = std::pow(10.0, decimals_dbl);
      double rounded_dbl =
          std::round(val_ev.cached_double * multiplier) / multiplier;
      double rounded_imag =
          std::round(val_ev.cached_imag * multiplier) / multiplier;

      ExactValue res;
      res.cached_double = rounded_dbl;
      res.cached_imag = rounded_imag;
      res.is_approx = true;
      return res;
    }

    if (node.func_name == "root" || node.func_name == "croot") {
      ExactValue arg = evaluate(state, node.right_idx);
      if (state.error != ParseError::NONE)
        return {};
      long long degree = (node.func_name == "root") ? 2 : 3;
      return compute_root(arg, degree, state);
    }

    if (node.func_name == "ln") {
      ExactValue arg_val = evaluate(state, node.right_idx);
      if (state.error != ParseError::NONE)
        return {};

      // Native Identity Check: ln(e^x) -> x or ln(e) -> 1
      if (arg_val.terms.size() == 1 && arg_val.terms[0].a == 1 &&
          arg_val.terms[0].b == 1 && arg_val.terms[0].c == 1 &&
          !arg_val.terms[0].is_imaginary && arg_val.terms[0].vars.size() == 1 &&
          arg_val.terms[0].vars[0].name == "e") {
        return make_exact(arg_val.terms[0].vars[0].power, 1, 1, 2,
                          arg_val.terms[0].vars[0].power);
      }

      if (arg_val.cached_double <= 0.0 ||
          std::abs(arg_val.cached_imag) > 1e-9) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "log input must be positive real";
        return {};
      }
      if (std::abs(arg_val.cached_double - 2.718281828459) < 1e-9)
        return make_exact(1, 1, 1, 2, 1.0);

      double ln_val = std::log(arg_val.cached_double);
      ExactValue res = double_to_exact(ln_val);
      res.symbolic_repr = "ln(" + to_exact_string(arg_val) + ")";
      res.cached_double = ln_val;
      return res;
    }

    if (node.func_name == "det" || node.func_name == "invert") {
      const ASTNode &arg_node = state.ast_pool[node.right_idx];
      if (arg_node.type != ASTNodeType::ARRAY) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = node.func_name + " requires a matrix argument";
        return {};
      }
      int rows = arg_node.args.size();
      if (rows == 0) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "empty matrix";
        return {};
      }
      const ASTNode &first_row = state.ast_pool[arg_node.args[0]];
      if (first_row.type != ASTNodeType::ARRAY) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "matrix must be 2D array";
        return {};
      }
      int cols = first_row.args.size();
      Matrix mat(rows, cols);
      for (int r = 0; r < rows; ++r) {
        const ASTNode &row_node = state.ast_pool[arg_node.args[r]];
        if (row_node.type != ASTNodeType::ARRAY || row_node.args.size() != cols) {
          state.error = ParseError::UNSUPPORTED_OPERATION;
          state.error_extra = "inconsistent matrix row length";
          return {};
        }
        for (int c = 0; c < cols; ++c) {
          ExactValue val = evaluate(state, row_node.args[c]);
          if (state.error != ParseError::NONE) return {};
          mat.set(r, c, val);
        }
      }
      if (node.func_name == "det") return mat.det(state);
      else return mat.invert(state);
    }

    if (node.func_name == "log" || node.func_name.rfind("log", 0) == 0) {

      double base_val = 10.0;
      ExactValue base_ev = make_exact(10, 1, 1, 2, 10.0);
      ExactValue arg_val;

      if (node.left_idx != -1) {
        base_ev = evaluate(state, node.left_idx);
        arg_val = evaluate(state, node.right_idx);
        base_val = to_double(base_ev);
      } else {
        arg_val = evaluate(state, node.right_idx);
        if (node.func_name != "log") {
          std::string base_str = node.func_name.substr(3);
          try {
            base_val = std::stod(base_str);
            base_ev = double_to_exact(base_val);
          } catch (...) {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra = "invalid log base label";
            return {};
          }
        }
      }
      if (state.error != ParseError::NONE)
        return {};

      // Native Identity Check: log_b(b^x) -> x
      const ASTNode &arg_node = state.ast_pool[node.right_idx];
      if (arg_node.type == ASTNodeType::BINARY && arg_node.op == '^') {
        ExactValue arg_base = evaluate(state, arg_node.left_idx);
        ExactValue diff = subtract(base_ev, arg_base, state);
        if (diff.terms.empty() && diff.symbolic_repr.empty()) {
          return evaluate(state, arg_node.right_idx);
        }
      }

      if (base_val <= 0.0 || base_val == 1.0 || arg_val.cached_double <= 0.0 ||
          std::abs(arg_val.cached_imag) > 1e-9) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        return {};
      }

      double p = std::log(arg_val.cached_double) / std::log(base_val);
      double rounded_p = std::round(p);
      if (std::abs(p - rounded_p) < 1e-9)
        return make_exact(static_cast<long long>(rounded_p), 1, 1, 2, p);

      ExactValue res = double_to_exact(p);
      res.symbolic_repr = "log" + to_exact_string(base_ev) + "(" +
                          to_exact_string(arg_val) + ")";
      res.cached_double = p;
      return res;
    }

    ExactValue arg_val = evaluate(state, node.right_idx);
    if (state.error != ParseError::NONE)
      return {};

    bool only_pi = true;
    bool has_pi = false;
    if (has_variables(arg_val)) {
        for (const auto &t : arg_val.terms) {
            for (const auto &vp : t.vars) {
                if (vp.name == "pi") has_pi = true;
                else if (vp.name != "e") only_pi = false;
            }
        }
    }
    
    if (has_variables(arg_val) && (!has_pi || !only_pi)) {
      ExactValue sym_res;
      ExactTerm st;
      st.a = 1;
      st.b = 1;
      st.c = 1;
      st.root_degree = 2;
      st.vars.push_back(
          {node.func_name + "(" + to_exact_string(arg_val) + ")", 1});
      sym_res.terms.push_back(st);
      sym_res.simplify();
      return sym_res;
    }

    double arg_dbl = to_double(arg_val);
    if (has_pi && only_pi) {
        arg_dbl = arg_dbl * 180.0 / std::numbers::pi;
    }
    double norm_angle = std::fmod(arg_dbl, 360.0);
    if (norm_angle < 0)
      norm_angle += 360.0;
    double rounded_angle = std::round(norm_angle);

    if (std::abs(norm_angle - rounded_angle) < 1e-9 &&
        std::abs(arg_val.cached_imag) < 1e-9) {
      int int_angle = static_cast<int>(rounded_angle);
      if (int_angle % 30 == 0 || int_angle == 45 || int_angle == 135 ||
          int_angle == 225 || int_angle == 315) {
        int ref_angle = 0, quadrant = 1;
        if (int_angle >= 0 && int_angle <= 90) {
          ref_angle = int_angle;
          quadrant = 1;
        } else if (int_angle > 90 && int_angle <= 180) {
          ref_angle = 180 - int_angle;
          quadrant = 2;
        } else if (int_angle > 180 && int_angle <= 270) {
          ref_angle = int_angle - 180;
          quadrant = 3;
        } else {
          ref_angle = 360 - int_angle;
          quadrant = 4;
        }

        bool pos = true;
        if (node.func_name == "sin" || node.func_name == "csc")
          pos = (quadrant == 1 || quadrant == 2);
        else if (node.func_name == "cos" || node.func_name == "sec")
          pos = (quadrant == 1 || quadrant == 4);
        else if (node.func_name == "tan" || node.func_name == "cot")
          pos = (quadrant == 1 || quadrant == 3);

        ExactValue res = lookup_trig(node.func_name, ref_angle, pos, state);
        if (state.error == ParseError::NONE)
          return res;
      }
    }

    state.error = ParseError::NONE;
    std::complex<double> c_arg(arg_val.cached_double, arg_val.cached_imag);
    std::complex<double> c_res(0.0, 0.0);

    if (node.func_name == "sin")
      c_res = std::sin(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "cos")
      c_res = std::cos(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "tan")
      c_res = std::tan(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "csc")
      c_res = 1.0 / std::sin(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "sec")
      c_res = 1.0 / std::cos(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "cot")
      c_res = 1.0 / std::tan(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "asin")
      c_res = std::asin(c_arg) * 180.0 / std::numbers::pi;
    else if (node.func_name == "acos")
      c_res = std::acos(c_arg) * 180.0 / std::numbers::pi;
    else if (node.func_name == "atan")
      c_res = std::atan(c_arg) * 180.0 / std::numbers::pi;
    else {
      state.error = ParseError::UNSUPPORTED_OPERATION;
      state.error_extra = "unknown function '" + node.func_name + "'";
      return {};
    }

    if (state.error == ParseError::NONE) {
      ExactValue final_res;
      final_res.cached_double = c_res.real();
      final_res.cached_imag = c_res.imag();
      final_res.symbolic_repr =
          node.func_name + "(" + to_exact_string(arg_val) + ")";
      return final_res;
    }
    // If it threw an error like "unknown function" or couldn't evaluate, clear
    // error and return symbolic
    state.error = ParseError::NONE;
    state.error_extra = "";
    ExactValue sym_res;
    ExactTerm st;
    st.a = 1;
    st.b = 1;
    st.c = 1;
    st.root_degree = 2;
    st.vars.push_back(
        {node.func_name + "(" + to_exact_string(arg_val) + ")", 1});
    sym_res.terms.push_back(st);
    sym_res.simplify();
    return sym_res;
  }

  if (node.type == ASTNodeType::BINARY) {

    ExactValue left_val = evaluate(state, node.left_idx);
    ExactValue right_val = evaluate(state, node.right_idx);
    if (state.error != ParseError::NONE)
      return {};

    switch (node.op) {
    case '=':
    case '<':
    case '>':
    case 'L':
    case 'G':
      return subtract(left_val, right_val, state);
    case '+':
      return add(left_val, right_val, state);
    case '-':
      return subtract(left_val, right_val, state);
    case '*':
      return multiply(left_val, right_val, state);
    case '/':
      return divide(left_val, right_val, state);
    case '^': {
      right_val.simplify();
      if (right_val.terms.empty() && right_val.symbolic_repr.empty() && std::abs(right_val.cached_double) < 1e-9) {
        return make_exact(1, 1, 1, 2, 1.0);
      }
      if (right_val.terms.size() != 1 || right_val.terms[0].b != 1 ||
          right_val.terms[0].c != 1 || right_val.terms[0].is_imaginary) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "exponent must be a clean integer value";
        return {};
      }
      return power(left_val, static_cast<int>(static_cast<long long>(right_val.terms[0].a)), state);
    }
    }
  }
  return {};
}
