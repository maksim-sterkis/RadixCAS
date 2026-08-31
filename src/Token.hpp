#pragma once
#include <string>

enum class TokenType {
  NUMBER,
  DECIMAL_LITERAL,
  IDENTIFIER,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  POWER,
  EQUALS,
  LESS_THAN,
  GREATER_THAN,
  LESS_EQUALS,
  GREATER_EQUALS,
  LPAREN,
  RPAREN,
  LBRACKET,
  RBRACKET,
  COMMA,
  FACTORIAL,
  END
};

struct Token {
  TokenType type;
  std::string value;
};

enum class OutputMode { AUTO, DECIMAL, FRACTION };
