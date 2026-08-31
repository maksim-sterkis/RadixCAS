#include "ExactValue.hpp"
#include "Config.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

void ExactTerm::simplify() {
  if (c == HybridInt(0))
    c = 1;
  if (c < HybridInt(0)) {
    a = -a;
    c = -c;
  }
  if (a == HybridInt(0) || b == HybridInt(0)) {
    a = 0;
    b = 1;
    c = 1;
    vars.clear();
    is_imaginary = false;
    return;
  }

  bool is_neg_b = (b < HybridInt(0));
  HybridInt temp_b = is_neg_b ? -b : b;
  long long d = 2;

  while (d <= 1000) {
    HybridInt hd(d);
    HybridInt d_pow(1);
    bool overflow = false;
    for (int i = 0; i < root_degree; ++i) {
      d_pow = d_pow * hd;
      if (d_pow > temp_b) { overflow = true; break; }
    }
    if (overflow || d_pow > temp_b)
      break;

    while (temp_b % d_pow == HybridInt(0)) {
      a = a * hd;
      temp_b = temp_b / d_pow;
    }
    d++;
  }

  if (is_neg_b) {
    if (root_degree % 2 != 0) {
      a = -a;
      b = temp_b;
    } else {
      is_imaginary = !is_imaginary;
      b = temp_b;
    }
  } else {
    b = temp_b;
  }

  HybridInt gcd_val = HybridInt::gcd(a, c);
  if (gcd_val < HybridInt(0)) gcd_val = -gcd_val;
  a = a / gcd_val;
  c = c / gcd_val;

  std::vector<VariablePower> simplified_vars;
  for (auto &vp : vars)
    if (vp.power != 0)
      simplified_vars.push_back(vp);

  std::sort(simplified_vars.begin(), simplified_vars.end(),
            [](const VariablePower &x, const VariablePower &y) {
              return x.name < y.name;
            });

  vars.clear();
  for (const auto &vp : simplified_vars) {
    if (!vars.empty() && vars.back().name == vp.name)
      vars.back().power += vp.power;
    else
      vars.push_back(vp);
  }

  auto it =
      std::remove_if(vars.begin(), vars.end(),
                     [](const VariablePower &vp) { return vp.power == 0; });
  vars.erase(it, vars.end());


}
void ExactValue::simplify() {
  if (!symbolic_repr.empty() || is_approx)
    return;

  std::vector<ExactTerm> new_terms;
  for (auto &t : terms) {
    t.simplify();
    if (t.a == 0)
      continue;

    bool combined = false;
    for (auto &nt : new_terms) {
      if (are_like_terms(nt, t)) {
        HybridInt new_a = nt.a * t.c + t.a * nt.c;
        HybridInt new_c = nt.c * t.c;
        nt.a = new_a;
        nt.c = new_c;
        nt.simplify();
        combined = true;
        break;
      }
    }
    if (!combined)
      new_terms.push_back(t);
  }

  terms.clear();
  double dbl_acc = 0.0, imag_acc = 0.0;
  for (auto &nt : new_terms) {
    if (nt.a != 0) {
      terms.push_back(nt);
      double var_val = 1.0;
      for (const auto &vp : nt.vars) {
        double base_val = 1.0;
        if (vp.name == "pi") base_val = 3.14159265358979323846;
        else if (vp.name == "e") base_val = 2.71828182845904523536;
        var_val *= std::pow(base_val, vp.power);
      }
      double val = (static_cast<double>(nt.a) *
                    std::pow(static_cast<double>(nt.b), 1.0 / nt.root_degree) *
                    var_val) /
                   static_cast<double>(nt.c);
      if (nt.is_imaginary)
        imag_acc += val;
      else
        dbl_acc += val;
    }
  }
  cached_double = dbl_acc;
  cached_imag = imag_acc;
}

ExactValue make_exact(HybridInt a, HybridInt b, HybridInt c, long long root,
                      double dbl, double imag) {
  ExactValue ev;
  ev.terms.push_back({a, b, c, root, {}, imag != 0.0});
  ev.cached_double = dbl;
  ev.cached_imag = imag;
  ev.simplify();
  return ev;
}

double to_double(const ExactValue &ev) { return ev.cached_double; }

ExactValue double_to_exact(double val) {
  if (std::isnan(val) || std::isinf(val))
    return make_exact(0, 1, 1, 2, 0.0);

  double rounded = std::round(val);
  if (std::abs(val - rounded) < 1e-9)
    val = rounded;

  long long sign = (val < 0) ? -1 : 1;
  double abs_val = std::abs(val);
  if (abs_val < 1e-9)
    return make_exact(0, 1, 1, 2, 0.0);

  long long c = 1000000;
  long long a = std::round(abs_val * c);
  return make_exact(sign * a, 1, c, 2, val);
}

bool is_terminating_decimal(HybridInt denominator) {
  if (denominator == HybridInt(0)) return false;
  if (denominator < HybridInt(0)) denominator = -denominator;
  while (denominator % HybridInt(2) == HybridInt(0))
    denominator = denominator / HybridInt(2);
  while (denominator % HybridInt(5) == HybridInt(0))
    denominator = denominator / HybridInt(5);
  return denominator == HybridInt(1);
}

std::string format_double(double val) {
  std::ostringstream oss;
  oss << std::setprecision(10) << val;
  std::string s = oss.str();
  if (s.find('.') != std::string::npos) {
    while (!s.empty() && s.back() == '0')
      s.pop_back();
    if (!s.empty() && s.back() == '.')
      s.pop_back();
  }
  return s;
}

std::string format_complex(double real, double imag) {
  if (std::abs(real) < 1e-9 && std::abs(imag) < 1e-9)
    return "0";
  if (std::abs(real) < 1e-9) {
    if (std::abs(imag - 1.0) < 1e-9)
      return "i";
    if (std::abs(imag + 1.0) < 1e-9)
      return "-i";
    return format_double(imag) + "i";
  }
  if (std::abs(imag) < 1e-9)
    return format_double(real);

  std::string res = format_double(real);
  if (imag > 0) {
    if (std::abs(imag - 1.0) < 1e-9)
      res += " + i";
    else
      res += " + " + format_double(imag) + "i";
  } else {
    if (std::abs(imag + 1.0) < 1e-9)
      res += " - i";
    else
      res += " - " + format_double(std::abs(imag)) + "i";
  }
  return res;
}

bool has_variables(const ExactValue &ev) {
  for (const auto &t : ev.terms)
    if (!t.vars.empty())
      return true;
  return false;
}

#include <sstream>
std::string to_exact_string(const ExactValue &ev) {
  if (!ev.symbolic_repr.empty())
    return ev.symbolic_repr;
  if (ev.terms.empty()) {
    if (ev.is_approx) {
        std::ostringstream oss;
        oss << ev.cached_double;
        if (ev.cached_imag != 0) {
            if (ev.cached_imag > 0) oss << " + " << ev.cached_imag << "i";
            else oss << " - " << -ev.cached_imag << "i";
        }
        return oss.str();
    }
    return "0";
  }

  std::string res = "";
  for (size_t i = 0; i < ev.terms.size(); ++i) {
    const auto &t = ev.terms[i];
    HybridInt a_print = t.a;

    if (i > 0) {
      if (t.a < HybridInt(0)) {
        res += " - ";
        a_print = -t.a;
      } else {
        res += " + ";
      }
    } else if (t.a < HybridInt(0)) {
      res += "-";
      a_print = -t.a;
    }

    std::string num_vars = "";
    std::string den_vars = "";
    for (const auto &vp : t.vars) {
      if (vp.power > 0) {
        num_vars += vp.name;
        if (vp.power != 1) num_vars += "^" + std::to_string(vp.power);
      } else {
        std::string den_name = vp.name;
        if (den_name.front() == '(' && den_name.back() == ')') {
            den_name = den_name.substr(1, den_name.length() - 2);
        }
        den_vars += den_name;
        if (vp.power != -1) den_vars += "^" + std::to_string(-vp.power);
      }
    }

    std::string i_str = t.is_imaginary ? "i" : "";
    std::string num_str = "";

    if (t.b == HybridInt(1)) {
      if (a_print == HybridInt(1) && (!num_vars.empty() || t.is_imaginary))
        num_str = i_str + num_vars;
      else
        num_str = a_print.to_string() + i_str + num_vars;
    } else {
      std::string coeff = "";
      if (a_print == HybridInt(1))
        coeff = i_str + num_vars;
      else
        coeff = a_print.to_string() + i_str + num_vars;

      std::string rad_sym = get_root_symbol(t.root_degree);
      num_str = coeff + rad_sym + (USE_UNICODE ? "" : "(") +
                t.b.to_string() + (USE_UNICODE ? "" : ")");
    }

    if (t.c == HybridInt(1) && den_vars.empty()) {
      res += num_str;
    } else {
      std::string den_str = t.c != HybridInt(1) ? t.c.to_string() : "";
      if (!den_vars.empty()) den_str += (den_str.empty() ? "" : "*") + den_vars;
      
      if (t.b == HybridInt(1) && num_vars.empty())
        res += a_print.to_string() + i_str + "/(" + den_str + ")";
      else
        res += "(" + num_str + ")/(" + den_str + ")";
    }
  }
  return res.empty() ? "0" : res;
}

// Global variable storage for Assignments

bool are_like_terms(const ExactTerm &t1, const ExactTerm &t2) {
  if (t1.is_imaginary != t2.is_imaginary)
    return false;
  if (t1.b != t2.b || t1.root_degree != t2.root_degree)
    return false;
  if (t1.vars.size() != t2.vars.size())
    return false;
  for (size_t i = 0; i < t1.vars.size(); ++i) {
    if (!(t1.vars[i] == t2.vars[i]))
      return false;
  }
  return true;
}
