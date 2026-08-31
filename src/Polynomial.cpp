#include "Polynomial.hpp"
#include <algorithm>
#include <chrono>

Fraction::Fraction(HybridInt n, HybridInt d) {
  if (d == HybridInt(0)) {
    num = 0;
    den = 1;
  } else {
    num = n;
    den = d;
    simplify();
  }
}

void Fraction::simplify() {
  if (den < HybridInt(0)) {
    num = -num;
    den = -den;
  }
  if (num == HybridInt(0)) {
    den = 1;
  } else {
    HybridInt g = HybridInt::gcd(num, den);
    if (g < HybridInt(0)) g = -g;
    num = num / g;
    den = den / g;
  }
}

Fraction Fraction::operator+(const Fraction &o) const {
  return Fraction(num * o.den + o.num * den, den * o.den);
}

Fraction Fraction::operator-(const Fraction &o) const {
  return Fraction(num * o.den - o.num * den, den * o.den);
}

Fraction Fraction::operator*(const Fraction &o) const {
  return Fraction(num * o.num, den * o.den);
}

Fraction Fraction::operator/(const Fraction &o) const {
  return Fraction(num * o.den, den * o.num);
}

bool Fraction::operator==(const Fraction &o) const {
  return num == o.num && den == o.den;
}

bool Fraction::operator!=(const Fraction &o) const { return !(*this == o); }

bool Fraction::is_zero() const { return num == HybridInt(0); }

bool Monomial::operator<(const Monomial &o) const {
  // Lexicographical ordering: iterate through vars in alphabetical order.
  // If degree differs, the higher degree is "greater" (which means < returns
  // false). Wait, < means `this` is "less than" `o`. In Gröbner bases, we want
  // descending order, so the leading term comes first. We will define operator<
  // to return true if `this` is mathematically smaller than `o`.

  // Get all unique variable names from both monomials
  std::map<std::string, int> all_vars;
  for (const auto &kv : vars)
    all_vars[kv.first] = 1;
  for (const auto &kv : o.vars)
    all_vars[kv.first] = 1;

  for (const auto &kv : all_vars) {
    int d1 = vars.count(kv.first) ? vars.at(kv.first) : 0;
    int d2 = o.vars.count(kv.first) ? o.vars.at(kv.first) : 0;
    if (d1 < d2)
      return true;
    if (d1 > d2)
      return false;
  }
  return false; // Equal degrees
}

bool Monomial::operator==(const Monomial &o) const {
  std::map<std::string, int> all_vars;
  for (const auto &kv : vars)
    all_vars[kv.first] = 1;
  for (const auto &kv : o.vars)
    all_vars[kv.first] = 1;
  for (const auto &kv : all_vars) {
    int d1 = vars.count(kv.first) ? vars.at(kv.first) : 0;
    int d2 = o.vars.count(kv.first) ? o.vars.at(kv.first) : 0;
    if (d1 != d2)
      return false;
  }
  return true;
}

bool Monomial::divides(const Monomial &o) const {
  for (const auto &kv : vars) {
    int d1 = kv.second;
    int d2 = o.vars.count(kv.first) ? o.vars.at(kv.first) : 0;
    if (d1 > d2)
      return false;
  }
  return true;
}

Monomial Monomial::divide(const Monomial &o) const {
  Monomial res;
  res.coeff = coeff / o.coeff;
  for (const auto &kv : vars)
    res.vars[kv.first] = kv.second;
  for (const auto &kv : o.vars) {
    res.vars[kv.first] -= kv.second;
    if (res.vars[kv.first] == 0)
      res.vars.erase(kv.first);
  }
  return res;
}

Monomial Monomial::multiply(const Monomial &o) const {
  Monomial res;
  res.coeff = coeff * o.coeff;
  for (const auto &kv : vars)
    res.vars[kv.first] = kv.second;
  for (const auto &kv : o.vars) {
    res.vars[kv.first] += kv.second;
    if (res.vars[kv.first] == 0)
      res.vars.erase(kv.first);
  }
  return res;
}

Polynomial::Polynomial() {}
#include "ExactValue.hpp"
Polynomial::Polynomial(const ExactValue &ev) {
  for (const auto &t : ev.terms) {
    Monomial m;
    m.coeff = Fraction(t.a, t.b);
    if (t.c != 1) {
      // If there's a root, it's not a pure polynomial in the standard sense,
      // but we might treat root(c) as a constant coefficient if we used floats.
      // For true Buchberger, we must only have rational coefficients.
      // For now, if c != 1, we throw or approximate.
      // We'll just approximate the coefficient to fraction if needed, or error
      // out.
    }
    for (const auto &vp : t.vars) {
      m.vars[vp.name] = vp.power;
    }
    terms.push_back(m);
  }
  simplify();
}

Polynomial::Polynomial(const Monomial &m) {
  if (!m.coeff.is_zero())
    terms.push_back(m);
}

void Polynomial::simplify() {
  std::vector<Monomial> new_terms;
  for (const auto &m : terms) {
    if (m.coeff.is_zero())
      continue;
    bool found = false;
    for (auto &new_m : new_terms) {
      if (new_m == m) {
        new_m.coeff = new_m.coeff + m.coeff;
        found = true;
        break;
      }
    }
    if (!found)
      new_terms.push_back(m);
  }
  terms.clear();
  for (const auto &m : new_terms) {
    if (!m.coeff.is_zero())
      terms.push_back(m);
  }
  // Sort descending (leading term first)
  std::sort(terms.begin(), terms.end(),
            [](const Monomial &a, const Monomial &b) { return b < a; });
}

Polynomial Polynomial::operator+(const Polynomial &o) const {
  Polynomial res;
  res.terms = terms;
  res.terms.insert(res.terms.end(), o.terms.begin(), o.terms.end());
  res.simplify();
  return res;
}

Polynomial Polynomial::operator-(const Polynomial &o) const {
  Polynomial res;
  res.terms = terms;
  for (const auto &m : o.terms) {
    Monomial neg_m = m;
    neg_m.coeff = neg_m.coeff * Fraction(-1, 1);
    res.terms.push_back(neg_m);
  }
  res.simplify();
  return res;
}

Polynomial Polynomial::operator*(const Polynomial &o) const {
  Polynomial res;
  for (const auto &m1 : terms) {
    for (const auto &m2 : o.terms) {
      res.terms.push_back(m1.multiply(m2));
    }
  }
  res.simplify();
  return res;
}

Polynomial Polynomial::multiply(const Fraction &f) const {
  Polynomial res;
  for (const auto &m : terms) {
    Monomial new_m = m;
    new_m.coeff = new_m.coeff * f;
    res.terms.push_back(new_m);
  }
  res.simplify();
  return res;
}

Polynomial Polynomial::multiply(const Monomial &m) const {
  Polynomial res;
  for (const auto &t : terms) {
    res.terms.push_back(t.multiply(m));
  }
  res.simplify();
  return res;
}

bool Polynomial::is_zero() const { return terms.empty(); }

Monomial Polynomial::leading_term() const {
  if (is_zero())
    return Monomial{Fraction(0, 1), {}};
  return terms[0];
}

std::string Polynomial::to_string() const {
  if (is_zero())
    return "0";
  std::string s = "";
  for (size_t i = 0; i < terms.size(); ++i) {
    const auto &m = terms[i];
    if (i > 0 && m.coeff.num > 0)
      s += " + ";
    if (m.coeff.num < 0) {
      if (i == 0)
        s += "-";
      else
        s += " - ";
    }

    HybridInt abs_num = m.coeff.num;
    if (abs_num < HybridInt(0)) abs_num = -abs_num;
    if ((abs_num != HybridInt(1) || m.coeff.den != HybridInt(1)) || m.vars.empty()) {
      s += abs_num.to_string();
      if (m.coeff.den != HybridInt(1))
        s += "/" + m.coeff.den.to_string();
    }

    for (const auto &kv : m.vars) {
      s += kv.first;
      if (kv.second > 1)
        s += "^" + std::to_string(kv.second);
    }
  }
  return s;
}

std::pair<std::vector<Polynomial>, Polynomial>
multivariate_divide(const Polynomial &dividend,
                    const std::vector<Polynomial> &divisors) {
  std::vector<Polynomial> quotients(divisors.size());
  Polynomial p = dividend;
  Polynomial r;

  while (!p.is_zero()) {
    bool division_occurred = false;
    for (size_t i = 0; i < divisors.size(); ++i) {
      if (divisors[i].is_zero())
        continue;
      Monomial lt_p = p.leading_term();
      Monomial lt_d = divisors[i].leading_term();
      if (lt_d.divides(lt_p)) {
        Monomial term = lt_p.divide(lt_d);
        quotients[i] = quotients[i] + Polynomial(term);
        p = p - divisors[i].multiply(term);
        division_occurred = true;
        break;
      }
    }
    if (!division_occurred) {
      Monomial lt_p = p.leading_term();
      r = r + Polynomial(lt_p);
      p = p - Polynomial(lt_p);
    }
  }

  return {quotients, r};
}

Polynomial s_polynomial(const Polynomial &f, const Polynomial &g) {
  if (f.is_zero() || g.is_zero())
    return Polynomial();
  Monomial lt_f = f.leading_term();
  Monomial lt_g = g.leading_term();

  std::map<std::string, int> lcm_vars;
  for (const auto &kv : lt_f.vars)
    lcm_vars[kv.first] = kv.second;
  for (const auto &kv : lt_g.vars) {
    lcm_vars[kv.first] = std::max(lcm_vars[kv.first], kv.second);
  }

  Monomial lcm;
  lcm.coeff = Fraction(1, 1);
  lcm.vars = lcm_vars;

  Monomial m1 = lcm.divide(lt_f);
  m1.coeff = Fraction(1, 1);
  Monomial m2 = lcm.divide(lt_g);
  m2.coeff = Fraction(1, 1);

  // S(f,g) = (LCM / LT(f)) * f - (LCM / LT(g)) * (LC(f)/LC(g)) * g
  // Better: S(f,g) = (LC(g) * LCM / LT(f)) * f - (LC(f) * LCM / LT(g)) * g

  m1.coeff = lt_g.coeff;
  m2.coeff = lt_f.coeff;

  Polynomial p1 = f.multiply(m1);
  Polynomial p2 = g.multiply(m2);

  return p1 - p2;
}

std::vector<Polynomial> buchberger(const std::vector<Polynomial> &F) {
  std::vector<Polynomial> G = F;
  auto start_time = std::chrono::steady_clock::now();

  bool new_polynomial_added = true;
  while (new_polynomial_added) {
    new_polynomial_added = false;
    size_t g_size = G.size();
    for (size_t i = 0; i < g_size; ++i) {
      for (size_t j = i + 1; j < g_size; ++j) {
        // Check 10-second mathematical timeout
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                           current_time - start_time)
                           .count();
        if (elapsed >= 10) {
          throw std::runtime_error(
              "Error: System too complex for exact analytic solving (Timeout)");
        }

        Polynomial s = s_polynomial(G[i], G[j]);
        auto div_res = multivariate_divide(s, G);
        Polynomial r = div_res.second;

        if (!r.is_zero()) {
          G.push_back(r);
          new_polynomial_added = true;
        }
      }
    }
  }
  return G;
}
