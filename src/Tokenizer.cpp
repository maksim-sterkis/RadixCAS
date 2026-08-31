#include "Tokenizer.hpp"
#include "Config.hpp"
#include <cctype>

void tokenize(std::string_view input, ParserState &state) {
  std::vector<Token> raw_tokens;
  size_t pos = 0;
  while (pos < input.length()) {
    while (pos < input.length() && std::isspace(input[pos]))
      pos++;
    if (pos >= input.length())
      break;

    char c = input[pos];
    if (std::isalpha(c)) {
      std::string ident = "";
      while (pos < input.length() && std::isalnum(input[pos]))
        ident += input[pos++];

      // RadixCAS Alphabetic Splitter to enable implicit multiplication &
      // keyword protection
      size_t idx = 0;
      while (idx < ident.length()) {
        bool matched = false;
        static const std::vector<std::string> known_words = {
            "approx", "croot",  "round", "solve",  "root",       "sin",
            "cos",    "tan",    "csc",   "sec",    "cot",        "pi",
            "ln",     "asin",   "acos",  "atan",   "derivative", "integral",
            "sum",    "taylor", "det",   "invert", "limit"};

        // Prioritize numeric base logarithms like log2 or log10
        if (ident.substr(idx).starts_with("log")) {
          size_t len = 3;
          while (idx + len < ident.length() && std::isdigit(ident[idx + len])) {
            len++;
          }
          raw_tokens.push_back(
              {TokenType::IDENTIFIER, std::string(ident.substr(idx, len))});
          idx += len;
          matched = true;
        }

        if (!matched) {
          for (const auto &word : known_words) {
            if (ident.substr(idx).starts_with(word)) {
              raw_tokens.push_back({TokenType::IDENTIFIER, word});
              idx += word.length();
              matched = true;
              break;
            }
          }
        }
        
        if (!matched && std::isdigit(ident[idx])) {
          size_t len = 0;
          while (idx + len < ident.length() && std::isdigit(ident[idx + len])) {
            len++;
          }
          raw_tokens.push_back({TokenType::NUMBER, std::string(ident.substr(idx, len))});
          idx += len;
          matched = true;
        }

        if (!matched) {
          raw_tokens.push_back(
              {TokenType::IDENTIFIER, std::string(1, ident[idx])});
          idx++;
        }
      }
      continue;
    }

    if (std::isdigit(c) || c == '.') {
      std::string val = "";
      bool has_dot = false;
      if (c == '.') {
        if (pos + 1 < input.length() && std::isdigit(input[pos + 1])) {
          val += c;
          pos++;
          has_dot = true;
        } else {
          state.error = ParseError::UNKNOWN_CHAR;
          state.error_extra = ".";
          return;
        }
      } else {
        val += c;
        pos++;
      }

      while (pos < input.length() &&
             (std::isdigit(input[pos]) || input[pos] == '.')) {
        if (input[pos] == '.') {
          if (has_dot) {
            state.error = ParseError::UNKNOWN_CHAR;
            state.error_extra = ".";
            return;
          }
          has_dot = true;
        }
        val += input[pos++];
      }
      raw_tokens.push_back(
          {has_dot ? TokenType::DECIMAL_LITERAL : TokenType::NUMBER, val});
      continue;
    }

    pos++;
    switch (c) {
    case '=':
      raw_tokens.push_back({TokenType::EQUALS, "="});
      break;
    case '<':
      if (pos < input.length() && input[pos] == '=') {
        pos++;
        raw_tokens.push_back({TokenType::LESS_EQUALS, "<="});
      } else {
        raw_tokens.push_back({TokenType::LESS_THAN, "<"});
      }
      break;
    case '>':
      if (pos < input.length() && input[pos] == '=') {
        pos++;
        raw_tokens.push_back({TokenType::GREATER_EQUALS, ">="});
      } else {
        raw_tokens.push_back({TokenType::GREATER_THAN, ">"});
      }
      break;
    case '+':
      raw_tokens.push_back({TokenType::PLUS, "+"});
      break;
    case '-':
      raw_tokens.push_back({TokenType::MINUS, "-"});
      break;
    case '*':
      raw_tokens.push_back({TokenType::MULTIPLY, "*"});
      break;
    case '/':
      raw_tokens.push_back({TokenType::DIVIDE, "/"});
      break;
    case '^':
      raw_tokens.push_back({TokenType::POWER, "^"});
      break;
    case '(':
      raw_tokens.push_back({TokenType::LPAREN, "("});
      break;
    case '[':
      raw_tokens.push_back({TokenType::LBRACKET, "["});
      break;
    case ']':
      raw_tokens.push_back({TokenType::RBRACKET, "]"});
      break;
    case ',':
      raw_tokens.push_back({TokenType::COMMA, ","});
      break;
    case ')':
      raw_tokens.push_back({TokenType::RPAREN, ")"});
      break;
    case '!':
      raw_tokens.push_back({TokenType::FACTORIAL, "!"});
      break;

    default:
      state.error = ParseError::UNKNOWN_CHAR;
      state.error_extra = std::string(1, c);
      return;
    }
  }
  raw_tokens.push_back({TokenType::END, ""});

  if (state.error != ParseError::NONE)
    return;

  // Inject implicit multiply tokens (e.g., 2x -> 2 * x)
  for (size_t i = 0; i < raw_tokens.size() - 1; ++i) {
    state.tokens.push_back(raw_tokens[i]);

    TokenType curr = raw_tokens[i].type;
    TokenType next = raw_tokens[i + 1].type;

    bool curr_can_implicitly_multiply =
        (curr == TokenType::NUMBER || curr == TokenType::DECIMAL_LITERAL ||
         curr == TokenType::IDENTIFIER || curr == TokenType::RPAREN);

    bool next_can_implicitly_multiply =
        (next == TokenType::NUMBER || next == TokenType::DECIMAL_LITERAL ||
         next == TokenType::IDENTIFIER || next == TokenType::LPAREN);

    if (curr_can_implicitly_multiply && next_can_implicitly_multiply) {
      if (curr == TokenType::IDENTIFIER && is_function(raw_tokens[i].value) &&
          next == TokenType::LPAREN) {
        continue;
      }
      state.tokens.push_back({TokenType::MULTIPLY, "*"});
    }
  }
  state.tokens.push_back(raw_tokens.back());
}
