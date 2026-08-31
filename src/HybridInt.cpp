#include "HybridInt.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

HybridInt::HybridInt() : is_big(false), small_val(0), big_sign(false) {}
HybridInt::HybridInt(long long v) : is_big(false), small_val(v), big_sign(false) {}
HybridInt::HybridInt(int v) : is_big(false), small_val(v), big_sign(false) {}

HybridInt::HybridInt(const std::string& s) {
    if (s.empty()) {
        is_big = false; small_val = 0; big_sign = false; return;
    }
    big_sign = false;
    size_t start = 0;
    if (s[0] == '-') { big_sign = true; start = 1; }
    else if (s[0] == '+') { start = 1; }

    if (s.length() - start <= 18) {
        is_big = false;
        small_val = std::stoll(s);
        return;
    }

    is_big = true;
    for (int i = (int)s.length(); i > (int)start; i -= 9) {
        int len = std::min(9, i - (int)start);
        big_val.push_back(std::stoul(s.substr(i - len, len)));
    }
    optimize();
}

void HybridInt::promote_to_big() {
    if (is_big) return;
    is_big = true;
    big_sign = (small_val < 0);
    long long abs_val = std::abs(small_val);
    big_val.clear();
    if (abs_val == 0) {
        big_val.push_back(0);
        big_sign = false;
    } else {
        while (abs_val > 0) {
            big_val.push_back(abs_val % BASE);
            abs_val /= BASE;
        }
    }
}

void HybridInt::optimize() {
    if (!is_big) return;
    while (big_val.size() > 1 && big_val.back() == 0) {
        big_val.pop_back();
    }
    if (big_val.empty()) {
        is_big = false;
        small_val = 0;
        big_sign = false;
        return;
    }
    if (big_val.size() <= 2) {
        long long val = 0;
        if (big_val.size() == 2) {
            val = (long long)big_val[1] * BASE + big_val[0];
        } else {
            val = big_val[0];
        }
        // Max long long is ~9 * 10^18. So 2 limbs fits safely.
        if (val >= 0) { // Catch overflow if it was actually larger than long long max
            is_big = false;
            small_val = big_sign ? -val : val;
            big_val.clear();
        }
    }
    if (is_big && big_val.size() == 1 && big_val[0] == 0) {
        big_sign = false;
    }
}

std::string HybridInt::to_string() const {
    if (!is_big) return std::to_string(small_val);
    if (big_val.empty()) return "0";
    std::stringstream ss;
    if (big_sign) ss << "-";
    ss << big_val.back();
    for (int i = (int)big_val.size() - 2; i >= 0; --i) {
        ss << std::setfill('0') << std::setw(9) << big_val[i];
    }
    return ss.str();
}

double HybridInt::to_double() const {
    if (!is_big) return (double)small_val;
    double res = 0;
    double factor = 1.0;
    for (size_t i = 0; i < big_val.size(); ++i) {
        res += big_val[i] * factor;
        factor *= BASE;
    }
    return big_sign ? -res : res;
}

HybridInt::operator long long() const {
    if (!is_big) return small_val;
    long long val = 0;
    if (big_val.size() >= 2) {
        val = (long long)big_val[1] * BASE + big_val[0];
    } else if (big_val.size() == 1) {
        val = big_val[0];
    }
    return big_sign ? -val : val;
}

HybridInt::operator double() const {
    return to_double();
}

int HybridInt::sign() const {
    if (!is_big) return (small_val > 0) - (small_val < 0);
    if (big_val.empty() || (big_val.size() == 1 && big_val[0] == 0)) return 0;
    return big_sign ? -1 : 1;
}

std::vector<uint32_t> HybridInt::add_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    std::vector<uint32_t> res;
    uint32_t carry = 0;
    size_t n = std::max(a.size(), b.size());
    for (size_t i = 0; i < n || carry; ++i) {
        uint64_t sum = carry;
        if (i < a.size()) sum += a[i];
        if (i < b.size()) sum += b[i];
        res.push_back(sum % BASE);
        carry = sum / BASE;
    }
    return res;
}

std::vector<uint32_t> HybridInt::sub_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    std::vector<uint32_t> res;
    int32_t borrow = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        int64_t diff = a[i] - borrow - (i < b.size() ? b[i] : 0);
        if (diff < 0) {
            diff += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        res.push_back(diff);
    }
    while (res.size() > 1 && res.back() == 0) res.pop_back();
    return res;
}

int HybridInt::cmp_big_abs(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (int i = (int)a.size() - 1; i >= 0; --i) {
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    }
    return 0;
}

std::vector<uint32_t> HybridInt::mul_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    if (a.empty() || b.empty() || (a.size() == 1 && a[0] == 0) || (b.size() == 1 && b[0] == 0)) {
        return {0};
    }
    std::vector<uint32_t> res(a.size() + b.size(), 0);
    for (size_t i = 0; i < a.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b.size() || carry > 0; ++j) {
            uint64_t cur = res[i + j] + (uint64_t)a[i] * (j < b.size() ? b[j] : 0) + carry;
            res[i + j] = cur % BASE;
            carry = cur / BASE;
        }
    }
    while (res.size() > 1 && res.back() == 0) res.pop_back();
    return res;
}

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> HybridInt::div_mod_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    if (b.empty() || (b.size() == 1 && b[0] == 0)) throw std::runtime_error("Division by zero");
    if (cmp_big_abs(a, b) < 0) return {{0}, a};
    
    std::vector<uint32_t> q, r;
    for (int i = (int)a.size() - 1; i >= 0; --i) {
        r.insert(r.begin(), a[i]);
        while (r.size() > 1 && r.back() == 0) r.pop_back();
        
        uint32_t low = 0, high = BASE, ans = 0;
        while (low <= high) {
            uint32_t mid = low + (high - low) / 2;
            if (cmp_big_abs(mul_big(b, {mid}), r) <= 0) {
                ans = mid;
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }
        q.insert(q.begin(), ans);
        r = sub_big(r, mul_big(b, {ans}));
    }
    while (q.size() > 1 && q.back() == 0) q.pop_back();
    return {q, r};
}

HybridInt HybridInt::operator+(const HybridInt& rhs) const {
    if (!is_big && !rhs.is_big) {
        long long res;
        if (!__builtin_saddll_overflow(small_val, rhs.small_val, &res)) {
            return HybridInt(res);
        }
    }
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    HybridInt res; res.is_big = true;
    if (a.big_sign == b.big_sign) {
        res.big_val = add_big(a.big_val, b.big_val);
        res.big_sign = a.big_sign;
    } else {
        if (cmp_big_abs(a.big_val, b.big_val) >= 0) {
            res.big_val = sub_big(a.big_val, b.big_val);
            res.big_sign = a.big_sign;
        } else {
            res.big_val = sub_big(b.big_val, a.big_val);
            res.big_sign = b.big_sign;
        }
    }
    res.optimize();
    return res;
}

HybridInt HybridInt::operator-(const HybridInt& rhs) const {
    if (!is_big && !rhs.is_big) {
        long long res;
        if (!__builtin_ssubll_overflow(small_val, rhs.small_val, &res)) {
            return HybridInt(res);
        }
    }
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    HybridInt res; res.is_big = true;
    if (a.big_sign != b.big_sign) {
        res.big_val = add_big(a.big_val, b.big_val);
        res.big_sign = a.big_sign;
    } else {
        if (cmp_big_abs(a.big_val, b.big_val) >= 0) {
            res.big_val = sub_big(a.big_val, b.big_val);
            res.big_sign = a.big_sign;
        } else {
            res.big_val = sub_big(b.big_val, a.big_val);
            res.big_sign = !a.big_sign;
        }
    }
    res.optimize();
    return res;
}

HybridInt HybridInt::operator*(const HybridInt& rhs) const {
    if (!is_big && !rhs.is_big) {
        long long res;
        if (!__builtin_smulll_overflow(small_val, rhs.small_val, &res)) {
            return HybridInt(res);
        }
    }
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    HybridInt res; res.is_big = true;
    res.big_val = mul_big(a.big_val, b.big_val);
    res.big_sign = (a.big_sign != b.big_sign);
    res.optimize();
    return res;
}

HybridInt HybridInt::operator/(const HybridInt& rhs) const {
    if (rhs == 0) throw std::runtime_error("Division by zero");
    if (!is_big && !rhs.is_big) {
        if (small_val == INT64_MIN && rhs.small_val == -1) {
            HybridInt a = *this; a.promote_to_big();
            HybridInt b = rhs; b.promote_to_big();
            HybridInt res; res.is_big = true;
            res.big_val = div_mod_big(a.big_val, b.big_val).first;
            res.big_sign = (a.big_sign != b.big_sign);
            res.optimize();
            return res;
        }
        return HybridInt(small_val / rhs.small_val);
    }
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    HybridInt res; res.is_big = true;
    res.big_val = div_mod_big(a.big_val, b.big_val).first;
    res.big_sign = (a.big_sign != b.big_sign);
    res.optimize();
    return res;
}

HybridInt HybridInt::operator%(const HybridInt& rhs) const {
    if (rhs == 0) throw std::runtime_error("Division by zero");
    if (!is_big && !rhs.is_big) {
        if (small_val == INT64_MIN && rhs.small_val == -1) return HybridInt(0);
        return HybridInt(small_val % rhs.small_val);
    }
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    HybridInt res; res.is_big = true;
    res.big_val = div_mod_big(a.big_val, b.big_val).second;
    res.big_sign = a.big_sign;
    res.optimize();
    return res;
}

HybridInt& HybridInt::operator+=(const HybridInt& rhs) { *this = *this + rhs; return *this; }
HybridInt& HybridInt::operator-=(const HybridInt& rhs) { *this = *this - rhs; return *this; }
HybridInt& HybridInt::operator*=(const HybridInt& rhs) { *this = *this * rhs; return *this; }
HybridInt& HybridInt::operator/=(const HybridInt& rhs) { *this = *this / rhs; return *this; }
HybridInt& HybridInt::operator%=(const HybridInt& rhs) { *this = *this % rhs; return *this; }

HybridInt HybridInt::operator-() const {
    HybridInt res = *this;
    if (res.is_big) {
        if (!res.big_val.empty() && !(res.big_val.size() == 1 && res.big_val[0] == 0)) {
            res.big_sign = !res.big_sign;
        }
    } else {
        if (res.small_val == INT64_MIN) {
            res.promote_to_big();
            res.big_sign = !res.big_sign;
            res.optimize();
        } else {
            res.small_val = -res.small_val;
        }
    }
    return res;
}

bool HybridInt::operator==(const HybridInt& rhs) const {
    if (!is_big && !rhs.is_big) return small_val == rhs.small_val;
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    if (a.big_sign != b.big_sign) return false;
    return cmp_big_abs(a.big_val, b.big_val) == 0;
}
bool HybridInt::operator!=(const HybridInt& rhs) const { return !(*this == rhs); }
bool HybridInt::operator<(const HybridInt& rhs) const {
    if (!is_big && !rhs.is_big) return small_val < rhs.small_val;
    HybridInt a = *this; a.promote_to_big();
    HybridInt b = rhs; b.promote_to_big();
    if (a.big_sign != b.big_sign) return a.big_sign; // true if a is negative and b is positive
    int cmp = cmp_big_abs(a.big_val, b.big_val);
    if (a.big_sign) return cmp > 0;
    return cmp < 0;
}
bool HybridInt::operator<=(const HybridInt& rhs) const { return *this < rhs || *this == rhs; }
bool HybridInt::operator>(const HybridInt& rhs) const { return !(*this <= rhs); }
bool HybridInt::operator>=(const HybridInt& rhs) const { return !(*this < rhs); }

bool HybridInt::operator==(long long rhs) const { return *this == HybridInt(rhs); }
bool HybridInt::operator!=(long long rhs) const { return *this != HybridInt(rhs); }
bool HybridInt::operator<(long long rhs) const { return *this < HybridInt(rhs); }
bool HybridInt::operator<=(long long rhs) const { return *this <= HybridInt(rhs); }
bool HybridInt::operator>(long long rhs) const { return *this > HybridInt(rhs); }
bool HybridInt::operator>=(long long rhs) const { return *this >= HybridInt(rhs); }

HybridInt HybridInt::gcd(HybridInt a, HybridInt b) {
    if (a < HybridInt(0)) a = -a;
    if (b < HybridInt(0)) b = -b;
    while (b != HybridInt(0)) {
        HybridInt temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

HybridInt HybridInt::lcm(HybridInt a, HybridInt b) {
    if (a == 0 && b == 0) return HybridInt(0);
    HybridInt g = gcd(a, b);
    HybridInt res = (a / g) * b;
    if (res < HybridInt(0)) res = -res;
    return res;
}

HybridInt HybridInt::pow(HybridInt base, long long exp) {
    if (exp < 0) return HybridInt(0);
    HybridInt res(1);
    HybridInt a = base;
    while (exp > 0) {
        if (exp % 2 == 1) res = res * a;
        a = a * a;
        exp /= 2;
    }
    return res;
}
