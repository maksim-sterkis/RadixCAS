#pragma once
#include "ExactValue.hpp"
#include "Token.hpp"
#include <map>
#include <vector>

extern std::map<std::string, ExactValue> global_variables;
extern std::string current_target_var;

struct RationalValue {
  ExactValue num;
  ExactValue den;
  bool is_rational = false;
};

enum class ASTNodeType { LITERAL, UNARY, BINARY, FUNCTION, ARRAY };

struct ASTNode {
  ASTNodeType type;
  char op = 0;
  ExactValue value;
  int left_idx = -1;
  int right_idx = -1;
  std::vector<int> args;
  std::string func_name = "";
};

enum class ParseError {
  NONE,
  UNKNOWN_CHAR,
  UNEXPECTED_END,
  UNEXPECTED_TOKEN,
  EXPECTED_RPAREN,
  DIVIDE_BY_ZERO,
  UNSUPPORTED_OPERATION
};

struct ParserState {
  std::vector<Token> tokens;
  std::vector<ASTNode> ast_pool;
  size_t token_idx = 0;
  ParseError error = ParseError::NONE;
  std::string error_extra = "";
};
