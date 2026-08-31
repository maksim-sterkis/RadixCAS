#include "Config.hpp"

// Set this to false if your online browser compiler shows corrupted text (like
// ￢ﾈﾚ). Set this to true if your terminal environment supports raw UTF-8
// characters.
constexpr bool USE_UNICODE = false;

std::string get_root_symbol(long long degree) {
  if (USE_UNICODE) {
    if (degree == 2)
      return "√";
    if (degree == 3)
      return "³√";
    return "[" + std::to_string(degree) + "]√";
  } else {
    if (degree == 2)
      return "root";
    if (degree == 3)
      return "croot";
    return "root" + std::to_string(degree);
  }
}

std::string get_pi_symbol() { return USE_UNICODE ? "π" : "pi"; }

bool is_function(const std::string &name) {
  return name == "sin" || name == "cos" || name == "tan" || name == "csc" ||
         name == "sec" || name == "cot" || name == "log" || name == "ln" ||
         name == "root" || name == "croot" || name == "approx" ||
         name == "round" || name == "solve" || name == "derivative" ||
         name == "integral" || name == "sum" || name == "taylor" || 
         name == "det" || name == "invert" || name == "limit";
}
