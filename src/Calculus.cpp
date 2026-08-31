#include "Calculus.hpp"
#include "Evaluator.hpp"

static int push_literal(ParserState &state, double val) {
  ASTNode node;
  node.type = ASTNodeType::LITERAL;
  node.value = make_exact((long long)val, 1, 1, 2, val);
  state.ast_pool.push_back(node);
  return state.ast_pool.size() - 1;
}

static int push_binary(ParserState &state, char op, int left, int right) {
  ASTNode node;
  node.type = ASTNodeType::BINARY;
  node.op = op;
  node.left_idx = left;
  node.right_idx = right;
  state.ast_pool.push_back(node);
  return state.ast_pool.size() - 1;
}

static int push_function(ParserState &state, const std::string &name, int arg) {
  ASTNode node;
  node.type = ASTNodeType::FUNCTION;
  node.func_name = name;
  node.right_idx = arg;
  state.ast_pool.push_back(node);
  return state.ast_pool.size() - 1;
}

int differentiate_ast(ParserState &state, int node_idx,
                      const std::string &target_var) {
  if (state.error != ParseError::NONE)
    return -1;
  if (node_idx < 0 || node_idx >= state.ast_pool.size())
    return -1;

  ASTNode node = state.ast_pool[node_idx];

  if (node.type == ASTNodeType::LITERAL) {
    bool has_var = false;
    for (const auto &term : node.value.terms) {
      for (const auto &v : term.vars) {
        if (v.name == target_var)
          has_var = true;
      }
    }
    if (has_var) {
      return push_literal(state, 1.0);
    } else {
      return push_literal(state, 0.0);
    }
  }

  if (node.type == ASTNodeType::UNARY) {
    int du = differentiate_ast(state, node.right_idx, target_var);
    ASTNode un_node;
    un_node.type = ASTNodeType::UNARY;
    un_node.op = node.op;
    un_node.right_idx = du;
    state.ast_pool.push_back(un_node);
    return state.ast_pool.size() - 1;
  }

  if (node.type == ASTNodeType::BINARY) {
    if (node.op == '+' || node.op == '-') {
      int du = differentiate_ast(state, node.left_idx, target_var);
      int dv = differentiate_ast(state, node.right_idx, target_var);
      return push_binary(state, node.op, du, dv);
    }
    if (node.op == '*') {
      int u = node.left_idx;
      int v = node.right_idx;
      int du = differentiate_ast(state, u, target_var);
      int dv = differentiate_ast(state, v, target_var);

      int part1 = push_binary(state, '*', du, v);
      int part2 = push_binary(state, '*', u, dv);
      return push_binary(state, '+', part1, part2);
    }
    if (node.op == '/') {
      int u = node.left_idx;
      int v = node.right_idx;
      int du = differentiate_ast(state, u, target_var);
      int dv = differentiate_ast(state, v, target_var);

      int part1 = push_binary(state, '*', du, v);
      int part2 = push_binary(state, '*', u, dv);
      int num = push_binary(state, '-', part1, part2);

      int two = push_literal(state, 2.0);
      int den = push_binary(state, '^', v, two);

      return push_binary(state, '/', num, den);
    }
    if (node.op == '^') {
      int u = node.left_idx;
      int v = node.right_idx;
      int du = differentiate_ast(state, u, target_var);
      int dv = differentiate_ast(state, v, target_var);

      int ln_u = push_function(state, "ln", u);
      int part1 = push_binary(state, '*', dv, ln_u);

      int u_prime_over_u = push_binary(state, '/', du, u);
      int part2 = push_binary(state, '*', v, u_prime_over_u);

      int sum = push_binary(state, '+', part1, part2);

      return push_binary(state, '*', node_idx, sum);
    }
  }

  if (node.type == ASTNodeType::FUNCTION) {
    int u = node.right_idx;
    int du = differentiate_ast(state, u, target_var);

    int outer_deriv = -1;
    if (node.func_name == "sin") {
      outer_deriv = push_function(state, "cos", u);
    } else if (node.func_name == "cos") {
      int neg_one = push_literal(state, -1.0);
      int sin_u = push_function(state, "sin", u);
      outer_deriv = push_binary(state, '*', neg_one, sin_u);
    } else if (node.func_name == "tan") {
      int sec_u = push_function(state, "sec", u);
      int two = push_literal(state, 2.0);
      outer_deriv = push_binary(state, '^', sec_u, two);
    } else if (node.func_name == "ln") {
      int one = push_literal(state, 1.0);
      outer_deriv = push_binary(state, '/', one, u);
    } else if (node.func_name == "root") {
      int one = push_literal(state, 1.0);
      int two = push_literal(state, 2.0);
      int denom = push_binary(state, '*', two, node_idx);
      outer_deriv = push_binary(state, '/', one, denom);
    } else if (node.func_name == "taylor" || node.func_name == "sum" || node.func_name == "integral") {
      ExactValue evaluated = evaluate(state, node_idx);
      if (state.error != ParseError::NONE) return -1;
      ExactValue diffed = differentiate_polynomial(evaluated, target_var, state);
      ASTNode lit;
      lit.type = ASTNodeType::LITERAL;
      lit.value = diffed;
      state.ast_pool.push_back(lit);
      return state.ast_pool.size() - 1;
    } else {
      state.error = ParseError::UNSUPPORTED_OPERATION;
      state.error_extra = "Cannot differentiate function " + node.func_name;
      return -1;
    }

    return push_binary(state, '*', outer_deriv, du);
  }

  return push_literal(state, 0.0);
}

ExactValue differentiate_polynomial(const ExactValue &expr,
                                    const std::string &target_var,
                                    ParserState &state) {
  ExactValue res;
  res.cached_double = 0.0;
  res.cached_imag = 0.0;

  if (expr.is_approx) {
    state.error = ParseError::UNSUPPORTED_OPERATION;
    state.error_extra = "Cannot analytically differentiate approximate decimals";
    return res;
  }

  for (const auto &term : expr.terms) {
    ExactTerm new_t = term;
    
    auto it = std::find_if(new_t.vars.begin(), new_t.vars.end(),
                           [&](const VariablePower &vp) { return vp.name == target_var; });
    
    if (it != new_t.vars.end() && it->power > 0) {
      new_t.a = new_t.a * HybridInt(it->power);
      it->power -= 1;
      if (it->power == 0) {
        new_t.vars.erase(it);
      }
      res.terms.push_back(new_t);
    }
  }

  res.simplify();
  return res;
}

ExactValue integrate_polynomial(const ExactValue &expr,
                                const std::string &target_var,
                                ParserState &state) {
  ExactValue res;
  res.cached_double = 0.0;
  res.cached_imag = 0.0;

  if (expr.is_approx) {
    state.error = ParseError::UNSUPPORTED_OPERATION;
    state.error_extra = "Cannot analytically integrate approximate decimals";
    return res;
  }

  for (const auto &term : expr.terms) {
    ExactTerm new_t = term;

    int power = 0;
    for (auto &v : new_t.vars) {
      if (v.name == target_var) {
        power = v.power;
        break;
      }
    }

    if (power == -1) {
      state.error = ParseError::UNSUPPORTED_OPERATION;
      state.error_extra = "Integration of 1/x -> ln(x) natively not supported "
                          "via exact terms yet";
      return res;
    }

    int new_power = power + 1;

    bool found = false;
    for (auto &v : new_t.vars) {
      if (v.name == target_var) {
        v.power = new_power;
        found = true;
        break;
      }
    }
    if (!found) {
      new_t.vars.push_back({target_var, new_power});
    }

    new_t.c *= new_power;
    res.terms.push_back(new_t);
  }

  res.simplify();
  return res;
}
