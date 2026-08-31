#pragma once
#include <string>
#include <vector>
#include <cstdint>
class HybridInt {
private:
    bool is_big;
    long long small_val;
    std::vector<uint32_t> big_val; // Base 10^9. Index 0 is least significant limb.
    bool big_sign; // true if negative

    static const uint32_t BASE = 1000000000;

    void promote_to_big();
    void optimize(); // Shrinks back to small if possible

    // BigInt absolute math helpers
    static std::vector<uint32_t> add_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);
    static std::vector<uint32_t> sub_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b); // Assumes a >= b
    static int cmp_big_abs(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);
    static std::vector<uint32_t> mul_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);
    static std::pair<std::vector<uint32_t>, std::vector<uint32_t>> div_mod_big(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b);

public:
    HybridInt();
    HybridInt(long long v);
    HybridInt(int v);
    HybridInt(const std::string& s);
    
    HybridInt(const HybridInt& other) = default;
    HybridInt& operator=(const HybridInt& other) = default;

    std::string to_string() const;
    double to_double() const;

    explicit operator long long() const;
    explicit operator double() const;

    HybridInt operator+(const HybridInt& rhs) const;
    HybridInt operator-(const HybridInt& rhs) const;
    HybridInt operator*(const HybridInt& rhs) const;
    HybridInt operator/(const HybridInt& rhs) const;
    HybridInt operator%(const HybridInt& rhs) const;
    
    HybridInt& operator+=(const HybridInt& rhs);
    HybridInt& operator-=(const HybridInt& rhs);
    HybridInt& operator*=(const HybridInt& rhs);
    HybridInt& operator/=(const HybridInt& rhs);
    HybridInt& operator%=(const HybridInt& rhs);

    HybridInt operator-() const;

    bool operator==(const HybridInt& rhs) const;
    bool operator!=(const HybridInt& rhs) const;
    bool operator<(const HybridInt& rhs) const;
    bool operator<=(const HybridInt& rhs) const;
    bool operator>(const HybridInt& rhs) const;
    bool operator>=(const HybridInt& rhs) const;

    bool operator==(long long rhs) const;
    bool operator!=(long long rhs) const;
    bool operator<(long long rhs) const;
    bool operator<=(long long rhs) const;
    bool operator>(long long rhs) const;
    bool operator>=(long long rhs) const;

    static HybridInt gcd(HybridInt a, HybridInt b);
    static HybridInt lcm(HybridInt a, HybridInt b);
    static HybridInt pow(HybridInt base, long long exp);

    int sign() const;
};
