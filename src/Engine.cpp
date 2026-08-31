#include "Engine.hpp"
#include "Config.hpp"
#include <cmath>
#include <complex>

ExactValue add(ExactValue val1, ExactValue val2, ParserState &state) {
  if (val1.is_approx || val2.is_approx) {
    ExactValue res;
    res.cached_double = val1.cached_double + val2.cached_double;
    res.cached_imag = val1.cached_imag + val2.cached_imag;
    res.is_approx = true;
    return res;
  }

  val1.simplify();
  val2.simplify();

  bool v1_zero = (val1.terms.empty() && val1.symbolic_repr.empty()) || (val1.terms.size() == 1 && val1.terms[0].a == 0 && val1.symbolic_repr.empty());
  if (v1_zero) return val2;
  bool v2_zero = (val2.terms.empty() && val2.symbolic_repr.empty()) || (val2.terms.size() == 1 && val2.terms[0].a == 0 && val2.symbolic_repr.empty());
  if (v2_zero) return val1;

  if (!val1.symbolic_repr.empty() || !val2.symbolic_repr.empty()) {
    ExactValue res;
    res.symbolic_repr =
        "(" + to_exact_string(val1) + " + " + to_exact_string(val2) + ")";
    res.cached_double = val1.cached_double + val2.cached_double;
    res.cached_imag = val1.cached_imag + val2.cached_imag;
    return res;
  }

  ExactValue res;
  res.terms = val1.terms;
  res.terms.insert(res.terms.end(), val2.terms.begin(), val2.terms.end());
  res.cached_double = val1.cached_double + val2.cached_double;
  res.cached_imag = val1.cached_imag + val2.cached_imag;
  res.simplify();
  return res;
}

ExactValue subtract(ExactValue val1, ExactValue val2, ParserState &state) {
  if (val1.is_approx || val2.is_approx) {
    ExactValue res;
    res.cached_double = val1.cached_double - val2.cached_double;
    res.cached_imag = val1.cached_imag - val2.cached_imag;
    res.is_approx = true;
    return res;
  }

  val1.simplify();
  val2.simplify();

  bool v2_zero = (val2.terms.empty() && val2.symbolic_repr.empty()) || (val2.terms.size() == 1 && val2.terms[0].a == 0 && val2.symbolic_repr.empty());
  if (v2_zero) return val1;
  bool v1_zero = (val1.terms.empty() && val1.symbolic_repr.empty()) || (val1.terms.size() == 1 && val1.terms[0].a == 0 && val1.symbolic_repr.empty());
  if (v1_zero) {
      ExactValue neg_val2 = val2;
      for(auto& t : neg_val2.terms) t.a = -t.a;
      neg_val2.cached_double = -neg_val2.cached_double;
      neg_val2.cached_imag = -neg_val2.cached_imag;
      if (!neg_val2.symbolic_repr.empty()) neg_val2.symbolic_repr = "(-" + neg_val2.symbolic_repr + ")";
      return neg_val2;
  }

  if (!val1.symbolic_repr.empty() || !val2.symbolic_repr.empty()) {
    ExactValue res;
    res.symbolic_repr =
        "(" + to_exact_string(val1) + " - " + to_exact_string(val2) + ")";
    res.cached_double = val1.cached_double - val2.cached_double;
    res.cached_imag = val1.cached_imag - val2.cached_imag;
    return res;
  }

  for (auto &t : val2.terms)
    t.a = -t.a;
  return add(val1, val2, state);
}

ExactValue multiply(ExactValue val1, ExactValue val2, ParserState &state) {
  if (val1.is_approx || val2.is_approx) {
    ExactValue res;
    res.cached_double = val1.cached_double * val2.cached_double -
                        val1.cached_imag * val2.cached_imag;
    res.cached_imag = val1.cached_double * val2.cached_imag +
                      val1.cached_imag * val2.cached_double;
    res.is_approx = true;
    return res;
  }

  val1.simplify();
  val2.simplify();
  
  bool v1_zero = (val1.terms.empty() && val1.symbolic_repr.empty()) || (val1.terms.size() == 1 && val1.terms[0].a == 0 && val1.symbolic_repr.empty());
  bool v2_zero = (val2.terms.empty() && val2.symbolic_repr.empty()) || (val2.terms.size() == 1 && val2.terms[0].a == 0 && val2.symbolic_repr.empty());
  if (v1_zero || v2_zero) {
    ExactValue res;
    res.cached_double = 0;
    res.cached_imag = 0;
    return res;
  }
  
  bool v1_one = val1.terms.size() == 1 && val1.terms[0].a == 1 && val1.terms[0].b == 1 && val1.terms[0].c == 1 && val1.terms[0].vars.empty() && !val1.terms[0].is_imaginary && val1.symbolic_repr.empty();
  if (v1_one) return val2;
  bool v2_one = val2.terms.size() == 1 && val2.terms[0].a == 1 && val2.terms[0].b == 1 && val2.terms[0].c == 1 && val2.terms[0].vars.empty() && !val2.terms[0].is_imaginary && val2.symbolic_repr.empty();
  if (v2_one) return val1;

  if (!val1.symbolic_repr.empty() || !val2.symbolic_repr.empty()) {
    ExactValue res;
    res.symbolic_repr =
        "(" + to_exact_string(val1) + " * " + to_exact_string(val2) + ")";
    res.cached_double = val1.cached_double * val2.cached_double -
                        val1.cached_imag * val2.cached_imag;
    res.cached_imag = val1.cached_double * val2.cached_imag +
                      val1.cached_imag * val2.cached_double;
    return res;
  }

  ExactValue res;
  res.cached_double = val1.cached_double * val2.cached_double -
                      val1.cached_imag * val2.cached_imag;
  res.cached_imag = val1.cached_double * val2.cached_imag +
                    val1.cached_imag * val2.cached_double;

  for (const auto &t1 : val1.terms) {
    for (const auto &t2 : val2.terms) {
      if (t1.root_degree != t2.root_degree) {
        ExactValue sym_res;
        sym_res.symbolic_repr =
            "(" + to_exact_string(val1) + " * " + to_exact_string(val2) + ")";
        sym_res.cached_double = res.cached_double;
        sym_res.cached_imag = res.cached_imag;
        return sym_res;
      }
      ExactTerm new_t;
      new_t.is_imaginary = (t1.is_imaginary != t2.is_imaginary);
      new_t.a =
          (t1.is_imaginary && t2.is_imaginary) ? -(t1.a * t2.a) : (t1.a * t2.a);
      new_t.b = t1.b * t2.b;
      new_t.c = t1.c * t2.c;
      new_t.root_degree = t1.root_degree;

      new_t.vars = t1.vars;
      new_t.vars.insert(new_t.vars.end(), t2.vars.begin(), t2.vars.end());
      res.terms.push_back(new_t);
    }
  }
  res.simplify();
  return res;
}

bool get_univariate_info(const ExactValue& ev, std::string& var_name, int& max_deg) {
  if (ev.terms.empty() && !ev.symbolic_repr.empty()) return false;
  var_name = "";
  max_deg = 0;
  for (const auto& t : ev.terms) {
    if (t.vars.size() > 1) return false;
    if (t.vars.size() == 1) {
      if (var_name.empty()) var_name = t.vars[0].name;
      else if (var_name != t.vars[0].name) return false;
      if (t.vars[0].power > max_deg) max_deg = t.vars[0].power;
      if (t.vars[0].power < 0) return false;
    }
    if (t.c != 1 || t.root_degree != 2 || t.is_imaginary) return false;
  }
  return true;
}

ExactTerm get_leading_term(const ExactValue& ev, const std::string& var_name, int deg) {
  ExactTerm res;
  res.a = 0; res.b = 1; res.c = 1; res.root_degree = 2;
  for (const auto& t : ev.terms) {
    int d = 0;
    if (!t.vars.empty() && t.vars[0].name == var_name) d = t.vars[0].power;
    if (d == deg) return t;
  }
  return res;
}

ExactTerm divide_terms(const ExactTerm& num, const ExactTerm& den) {
  ExactTerm res;
  res.a = num.a * den.b;
  res.b = num.b * den.a;
  res.c = 1; res.root_degree = 2;
  if (res.b < HybridInt(0)) { res.a = -res.a; res.b = -res.b; }
  int n_deg = num.vars.empty() ? 0 : num.vars[0].power;
  int d_deg = den.vars.empty() ? 0 : den.vars[0].power;
  int r_deg = n_deg - d_deg;
  if (r_deg > 0) res.vars.push_back({num.vars.empty() ? den.vars[0].name : num.vars[0].name, r_deg});
  return res;
}

bool poly_divide(ExactValue num, ExactValue den, ExactValue& quotient, ExactValue& remainder, ParserState& state) {
  std::string n_var, d_var;
  int n_deg = 0, d_deg = 0;
  if (den.terms.size() <= 1) return false;
  if (!get_univariate_info(num, n_var, n_deg)) return false;
  if (!get_univariate_info(den, d_var, d_deg)) return false;
  if (!n_var.empty() && !d_var.empty() && n_var != d_var) return false;
  std::string var_name = n_var.empty() ? d_var : n_var;
  
  quotient = make_exact(0, 1, 1, 2, 0);
  remainder = num;
  int r_deg = n_deg;
  
  while (r_deg >= d_deg) {
    ExactTerm r_lead = get_leading_term(remainder, var_name, r_deg);
    if (r_lead.a == 0) { r_deg--; continue; }
    
    ExactTerm d_lead = get_leading_term(den, var_name, d_deg);
    if (d_lead.a == 0) break;
    ExactTerm q_term = divide_terms(r_lead, d_lead);
    ExactValue q_val; q_val.terms.push_back(q_term);
    
    quotient = add(quotient, q_val, state);
    ExactValue sub = multiply(den, q_val, state);
    remainder = subtract(remainder, sub, state);
    
    std::string rem_var; int new_r_deg;
    if (!get_univariate_info(remainder, rem_var, new_r_deg)) break;
    
    if (new_r_deg == r_deg) r_deg--;
    else r_deg = new_r_deg;
    
    if ((remainder.terms.empty() && remainder.symbolic_repr.empty()) || (remainder.terms.size() == 1 && remainder.terms[0].a == 0 && remainder.symbolic_repr.empty())) break;
  }
  return true;
}

ExactValue divide(ExactValue val1, ExactValue val2, ParserState &state) {
  bool is_zero = val2.terms.empty() && val2.symbolic_repr.empty() && val2.cached_double == 0.0 && val2.cached_imag == 0.0;
  if (is_zero) {
    state.error = ParseError::DIVIDE_BY_ZERO;
    return {};
  }

  double denom_mag = val2.cached_double * val2.cached_double +
                     val2.cached_imag * val2.cached_imag;

  if (val1.is_approx || val2.is_approx) {
    ExactValue res;
    res.cached_double = (val1.cached_double * val2.cached_double +
                         val1.cached_imag * val2.cached_imag) /
                        denom_mag;
    res.cached_imag = (val1.cached_imag * val2.cached_double -
                       val1.cached_double * val2.cached_imag) /
                      denom_mag;
    res.is_approx = true;
    return res;
  }

  val1.simplify();
  val2.simplify();
  if (!val1.symbolic_repr.empty() || !val2.symbolic_repr.empty() || val2.terms.size() > 1) {
    double res_dbl = (val1.cached_double * val2.cached_double + val1.cached_imag * val2.cached_imag) / denom_mag;
    double res_img = (val1.cached_imag * val2.cached_double - val1.cached_double * val2.cached_imag) / denom_mag;

    ExactValue q, r;
    if (val1.symbolic_repr.empty() && val2.symbolic_repr.empty() && poly_divide(val1, val2, q, r, state)) {
      bool r_zero = (r.terms.empty() && r.symbolic_repr.empty()) || (r.terms.size() == 1 && r.terms[0].a == 0 && r.symbolic_repr.empty());
      if (r_zero) {
        q.cached_double = res_dbl;
        q.cached_imag = res_img;
        return q;
      } else {
        ExactTerm den_t;
        den_t.a = 1; den_t.b = 1; den_t.c = 1; den_t.root_degree = 2;
        den_t.vars.push_back({"(" + to_exact_string(val2) + ")", -1});
        ExactValue inv_val2;
        inv_val2.terms.push_back(den_t);
        inv_val2.cached_double = 1.0 / denom_mag;
        inv_val2.cached_imag = 0;
        ExactValue r_div_den = multiply(r, inv_val2, state);
        return add(q, r_div_den, state);
      }
    }

    if (val1.symbolic_repr.empty() && val2.symbolic_repr.empty() && val2.terms.size() > 1) {
      ExactTerm den_t;
      den_t.a = 1; den_t.b = 1; den_t.c = 1; den_t.root_degree = 2;
      den_t.vars.push_back({"(" + to_exact_string(val2) + ")", -1});
      ExactValue inv_val2;
      inv_val2.terms.push_back(den_t);
      inv_val2.cached_double = 1.0 / denom_mag;
      inv_val2.cached_imag = 0;
      return multiply(val1, inv_val2, state);
    }

    ExactValue sym_res;
    sym_res.symbolic_repr = "(" + to_exact_string(val1) + " / " + to_exact_string(val2) + ")";
    sym_res.cached_double = res_dbl;
    sym_res.cached_imag = res_img;
    return sym_res;
  }

  ExactValue res;
  res.cached_double = (val1.cached_double * val2.cached_double +
                       val1.cached_imag * val2.cached_imag) /
                      denom_mag;
  res.cached_imag = (val1.cached_imag * val2.cached_double -
                     val1.cached_double * val2.cached_imag) /
                    denom_mag;

  const auto &den = val2.terms[0];

  for (const auto &num : val1.terms) {
    if (num.root_degree != den.root_degree) {
      ExactValue sym;
      sym.symbolic_repr =
          "(" + to_exact_string(val1) + " / " + to_exact_string(val2) + ")";
      sym.cached_double = res.cached_double;
      sym.cached_imag = res.cached_imag;
      return sym;
    }
    ExactTerm new_t;
    HybridInt b2_power(1);
    for (int i = 0; i < den.root_degree - 1; ++i)
      b2_power = b2_power * den.b;

    HybridInt sign_mult(1);
    if (den.is_imaginary) {
      new_t.is_imaginary = !num.is_imaginary;
      sign_mult = num.is_imaginary ? 1 : -1;
    } else {
      new_t.is_imaginary = num.is_imaginary;
    }

    new_t.a = num.a * den.c * sign_mult;
    new_t.b = num.b * b2_power;
    new_t.c = num.c * den.a * den.b;
    new_t.root_degree = num.root_degree;

    new_t.vars = num.vars;
    for (const auto &dv : den.vars) {
      bool found = false;
      for (auto &nv : new_t.vars) {
        if (nv.name == dv.name) {
          nv.power -= dv.power;
          found = true;
          break;
        }
      }
      if (!found)
        new_t.vars.push_back({dv.name, -dv.power});
    }
    res.terms.push_back(new_t);
  }
  res.simplify();
  return res;
}

ExactValue power(ExactValue val, int n, ParserState &state) {
  std::complex<double> base(val.cached_double, val.cached_imag);
  std::complex<double> result = std::pow(base, n);

  if (val.is_approx) {
    ExactValue res;
    res.cached_double = result.real();
    res.cached_imag = result.imag();
    res.is_approx = true;
    return res;
  }

  val.simplify();
  if (!val.symbolic_repr.empty()) {
    ExactValue res;
    res.symbolic_repr = "(" + val.symbolic_repr + ")^" + std::to_string(n);
    res.cached_double = result.real();
    res.cached_imag = result.imag();
    return res;
  }
  if (n == 0)
    return make_exact(1, 1, 1, 2, 1.0);

  if (n < 0) {
    if (val.cached_double == 0.0 && val.cached_imag == 0.0) {
      state.error = ParseError::DIVIDE_BY_ZERO;
      return {};
    }
    ExactValue one = make_exact(1, 1, 1, 2, 1.0);
    return power(divide(one, val, state), -n, state);
  }

  ExactValue res = val;
  for (int i = 1; i < n; ++i)
    res = multiply(res, val, state);
  return res;
}

ExactValue compute_root(ExactValue arg, int degree, ParserState &state) {
  arg.simplify();
  bool make_imaginary = false;

  if (arg.cached_double < 0 && std::abs(arg.cached_imag) < 1e-9 &&
      degree % 2 == 0) {
    if (arg.terms.size() == 1 && !arg.terms[0].is_imaginary) {
      make_imaginary = true;
      arg.terms[0].a = -arg.terms[0].a;
      arg.cached_double = -arg.cached_double;
    } else {
      state.error = ParseError::UNSUPPORTED_OPERATION;
      state.error_extra =
          "no real solutions (cannot take even root of complex polynomial)";
      return {};
    }
  } else if (std::abs(arg.cached_imag) > 1e-9) {
    state.error = ParseError::UNSUPPORTED_OPERATION;
    state.error_extra = "roots of complex numbers unsupported in exact mode";
    return {};
  }

  bool arg_has_vars = false;
  for (const auto &t : arg.terms)
    if (!t.vars.empty())
      arg_has_vars = true;

  if (arg_has_vars || arg.symbolic_repr.size() > 0) {
    ExactValue sym_res;
    ExactTerm st;
    st.a = 1;
    st.b = 1;
    st.c = 1;
    st.root_degree = 2;
    std::string root_str = get_root_symbol(degree);
    st.vars.push_back({(make_imaginary ? "i" : "") + root_str +
                           (USE_UNICODE ? "" : "(") + to_exact_string(arg) +
                           (USE_UNICODE ? "" : ")"),
                       1});
    sym_res.terms.push_back(st);
    sym_res.simplify();
    return sym_res;
  }

  if (arg.terms.size() == 1 && arg.terms[0].b == 1 &&
      arg.symbolic_repr.empty() && !arg.is_approx) {
    ExactValue res;
    ExactTerm t;
    t.a = 1;
    t.root_degree = degree;
    if (degree == 2)
      t.b = arg.terms[0].a * arg.terms[0].c;
    else
      t.b = arg.terms[0].a * arg.terms[0].c * arg.terms[0].c;
    t.c = arg.terms[0].c;
    t.vars = arg.terms[0].vars;
    t.is_imaginary = make_imaginary;

    if (!t.vars.empty() || arg.symbolic_repr.size() > 0) {
      ExactValue sym_res;
      ExactTerm st;
      st.a = 1;
      st.b = 1;
      st.c = 1;
      st.root_degree = 2;
      std::string root_str = get_root_symbol(degree);
      st.vars.push_back({(make_imaginary ? "i" : "") + root_str +
                             (USE_UNICODE ? "" : "(") + to_exact_string(arg) +
                             (USE_UNICODE ? "" : ")"),
                         1});
      sym_res.terms.push_back(st);
      sym_res.simplify();
      return sym_res;
    }

    res.terms.push_back(t);
    res.simplify();
    res.cached_imag =
        make_imaginary ? std::pow(arg.cached_double, 1.0 / degree) : 0.0;
    res.cached_double =
        make_imaginary ? 0.0 : std::pow(arg.cached_double, 1.0 / degree);
    return res;
  } else {
    ExactValue res;
    res.cached_imag =
        make_imaginary ? std::pow(arg.cached_double, 1.0 / degree) : 0.0;
    res.cached_double =
        make_imaginary ? 0.0 : std::pow(arg.cached_double, 1.0 / degree);
    std::string root_str = get_root_symbol(degree);
    res.symbolic_repr = (make_imaginary ? "i" : "") + root_str +
                        (USE_UNICODE ? "" : "(") + to_exact_string(arg) +
                        (USE_UNICODE ? "" : ")");
    return res;
  }
}

// Global substitution helper to replace defined variable assignments
ExactValue substitute_variables(const ExactValue &ev, ParserState &state) {
  if (ev.is_approx || (ev.terms.empty() && !ev.symbolic_repr.empty()))
    return ev;

  ExactValue result = make_exact(0, 1, 1, 2, 0.0);
  for (const auto &term : ev.terms) {
    ExactValue term_val;
    ExactTerm base_term = term;
    base_term.vars.clear();
    term_val.terms.push_back(base_term);
    term_val.simplify();

    for (const auto &vp : term.vars) {
      if (vp.name == current_target_var) {
        // Keep the solver target variable symbolic
        ExactValue sym_var;
        ExactTerm st;
        st.a = 1;
        st.b = 1;
        st.c = 1;
        st.root_degree = 2;
        st.vars.push_back(vp);
        sym_var.terms.push_back(st);
        sym_var.simplify();
        term_val = multiply(term_val, sym_var, state);
      } else {
        auto it = global_variables.find(vp.name);
        if (it != global_variables.end()) {
          ExactValue sub_val = it->second;
          ExactValue sub_pow = power(sub_val, vp.power, state);
          term_val = multiply(term_val, sub_pow, state);
        } else {
          // Keep undefined variables symbolic
          ExactValue sym_var;
          ExactTerm st;
          st.a = 1;
          st.b = 1;
          st.c = 1;
          st.root_degree = 2;
          st.vars.push_back(vp);
          sym_var.terms.push_back(st);
          sym_var.simplify();
          term_val = multiply(term_val, sym_var, state);
        }
      }
    }
    result = add(result, term_val, state);
  }
  if (!ev.symbolic_repr.empty()) {
    result.symbolic_repr = ev.symbolic_repr;
  }
  return result;
}

RationalValue get_rational_form(ExactValue ev, ParserState &state) {
  ev.simplify();
  RationalValue rv;
  rv.num = ev;
  rv.den = make_exact(1, 1, 1, 2, 1.0);
  rv.is_rational = false;

  if (ev.terms.empty())
    return rv;

  bool has_neg = false;
  for (const auto &t : ev.terms) {
    for (const auto &vp : t.vars) {
      if (vp.power < 0) {
        has_neg = true;
        break;
      }
    }
  }

  if (!has_neg)
    return rv;

  std::map<std::string, int> den_vars;
  HybridInt common_c(1);

  for (const auto &t : ev.terms) {
    common_c = HybridInt::lcm(common_c, t.c);
    for (const auto &vp : t.vars) {
      if (vp.power < 0) {
        int den_pow = -vp.power;
        den_vars[vp.name] = std::max(den_vars[vp.name], den_pow);
      }
    }
  }

  ExactValue D;
  ExactTerm dt;
  dt.a = 1;
  dt.b = 1;
  dt.c = 1;
  dt.root_degree = 2;
  dt.is_imaginary = false;
  for (const auto &[name, pow] : den_vars) {
    dt.vars.push_back({name, pow});
  }
  D.terms.push_back(dt);
  D.simplify();

  ExactValue N = make_exact(0, 1, 1, 2, 0.0);
  for (const auto &t : ev.terms) {
    ExactValue t_val;
    ExactTerm nt = t;
    nt.c = 1;
    t_val.terms.push_back(nt);
    t_val.simplify();

    ExactValue scale =
        make_exact(common_c / t.c, 1, 1, 2, static_cast<double>(common_c) / static_cast<double>(t.c));
    ExactValue t_num = multiply(t_val, D, state);
    t_num = multiply(t_num, scale, state);

    N = add(N, t_num, state);
  }

  ExactValue scale_D = make_exact(common_c, 1, 1, 2, (double)common_c);
  D = multiply(D, scale_D, state);

  N.simplify();
  D.simplify();

  rv.num = N;
  rv.den = D;
  rv.is_rational = true;
  return rv;
}

std::string format_output(const ExactValue &ev, OutputMode mode) {
  ParserState dummy_state;
  
  bool has_pos = false;
  bool has_neg = false;
  for (const auto &t : ev.terms) {
      for (const auto &vp : t.vars) {
          if (vp.power > 0) has_pos = true;
          if (vp.power < 0) has_neg = true;
      }
  }
  
  if (has_pos && has_neg && mode == OutputMode::AUTO) {
      return to_exact_string(ev);
  }
  
  RationalValue rv = get_rational_form(ev, dummy_state);
  if (rv.is_rational) {
    if (mode == OutputMode::DECIMAL) {
      double num_val = rv.num.cached_double;
      double den_val = rv.den.cached_double;
      return format_complex(den_val != 0.0 ? num_val / den_val : 0.0, 0.0);
    }
    return "(" + to_exact_string(rv.num) + ")/(" + to_exact_string(rv.den) +
           ")";
  }

  if (!ev.symbolic_repr.empty() || ev.is_approx) {
    if (mode == OutputMode::AUTO && !ev.is_approx) {
      if (ev.symbolic_repr.find("=") != std::string::npos ||
          ev.symbolic_repr.find("<") != std::string::npos ||
          ev.symbolic_repr.find(">") != std::string::npos ||
          ev.symbolic_repr.find("numbers") != std::string::npos ||
          ev.symbolic_repr.find("solution") != std::string::npos)
        return ev.symbolic_repr;
      return ev.symbolic_repr + " (~" +
             format_complex(ev.cached_double, ev.cached_imag) + ")";
    }
    return ev.is_approx ? format_complex(ev.cached_double, ev.cached_imag)
                        : ev.symbolic_repr;
  }
  if (ev.terms.empty())
    return "0";

  switch (mode) {
  case OutputMode::DECIMAL:
    return format_complex(ev.cached_double, ev.cached_imag);
  case OutputMode::FRACTION:
    return to_exact_string(ev);
  case OutputMode::AUTO: {
    if (has_variables(ev)) {
      bool only_constants = true;
      for (const auto &t : ev.terms) {
        for (const auto &vp : t.vars) {
          if (vp.name != "pi" && vp.name != "e") {
            only_constants = false;
            break;
          }
        }
        if (!only_constants) break;
      }
      if (!only_constants)
        return to_exact_string(ev);
      else
        return to_exact_string(ev) + " (~" + format_complex(ev.cached_double, ev.cached_imag) + ")";
    }
    bool is_simple = false;
    if (ev.terms.size() == 1 && ev.terms[0].b == 1) {
      if (ev.terms[0].c == 1 || is_terminating_decimal(ev.terms[0].c))
        is_simple = true;
    } else if (ev.terms.size() == 2 && ev.terms[0].b == 1 &&
               ev.terms[1].b == 1) {
      if ((ev.terms[0].c == 1 || is_terminating_decimal(ev.terms[0].c)) &&
          (ev.terms[1].c == 1 || is_terminating_decimal(ev.terms[1].c)))
        is_simple = true;
    }

    if (is_simple) {
      std::string exact_str = to_exact_string(ev);
      std::string approx_str = format_complex(ev.cached_double, ev.cached_imag);
      if (exact_str == approx_str || exact_str == "i" || exact_str == "-i")
        return exact_str;
    }
    return to_exact_string(ev) + " (~" +
           format_complex(ev.cached_double, ev.cached_imag) + ")";
  }
  }
  return "0";
}

ExactValue parse_number_string(const std::string &val) {
  size_t dot_pos = val.find('.');
  if (dot_pos == std::string::npos) {
    return make_exact(HybridInt(val), 1, 1, 2, std::stod(val));
  } else {
    std::string whole = val.substr(0, dot_pos);
    std::string frac = val.substr(dot_pos + 1);
    HybridInt denominator(1);
    for (int i = 0; i < frac.length(); ++i) {
      denominator = denominator * HybridInt(10);
    }
    return make_exact(HybridInt(whole + frac), 1, denominator, 2,
                      std::stod(val));
  }
}
