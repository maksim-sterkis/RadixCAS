#pragma once
#include <string>

extern const bool USE_UNICODE;
std::string get_root_symbol(long long degree);
std::string get_pi_symbol();
bool is_function(const std::string &name);
