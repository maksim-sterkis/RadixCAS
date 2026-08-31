#include "Diagnostics.hpp"
#include <iostream>

void print_diagnostic(const ParserState &state) {
  switch (state.error) {
  case ParseError::UNKNOWN_CHAR:
    std::cout << "Error: Unknown character '" << state.error_extra << "'"
              << std::endl;
    break;
  case ParseError::UNEXPECTED_END:
    std::cout << "Error: Equation ended abruptly. Missing an operand!"
              << std::endl;
    break;
  case ParseError::UNEXPECTED_TOKEN:
    std::cout << "Error: Unexpected token '" << state.error_extra
              << "' encountered." << std::endl;
    break;
  case ParseError::EXPECTED_RPAREN:
    std::cout << "Error: Missing a closing parenthesis ')'" << std::endl;
    break;
  case ParseError::DIVIDE_BY_ZERO:
    std::cout << "Error: Cannot divide by 0." << std::endl;
    break;
  case ParseError::UNSUPPORTED_OPERATION:
    std::cout << "Error: Unsupported operation (" << state.error_extra << ")."
              << std::endl;
    break;
  default:
    std::cout << "Error: Invalid syntax." << std::endl;
    break;
  }
}
