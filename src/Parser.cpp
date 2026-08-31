#include "Parser.hpp"
#include "Config.hpp"
#include "Engine.hpp"
#include <numbers>

int get_precedence(TokenType type) {
  switch (type) {
  case TokenType::EQUALS:
  case TokenType::LESS_THAN:
  case TokenType::GREATER_THAN:
  case TokenType::LESS_EQUALS:
  case TokenType::GREATER_EQUALS:
    return 5;
  case TokenType::PLUS:
  case TokenType::MINUS:
    return 10;
  case TokenType::MULTIPLY:
  case TokenType::DIVIDE:
    return 20;
  case TokenType::POWER:
    return 30;
  case TokenType::FACTORIAL:
    return 40;
  default:
    return 0;
  }
}

int parse_expression(ParserState &state, int binding_power);
int parse_nud(ParserState &state);
int parse_led(ParserState &state, int left_idx, TokenType op_type);

int parse_expression(ParserState &state, int binding_power) {
  if (state.error != ParseError::NONE)
    return -1;
  int left_idx = parse_nud(state);
  if (state.error != ParseError::NONE)
    return -1;

  while (state.token_idx < state.tokens.size() &&
         state.error == ParseError::NONE) {
    TokenType next_type = state.tokens[state.token_idx].type;
    if (next_type == TokenType::END ||
        binding_power >= get_precedence(next_type))
      break;
    state.token_idx++;
    left_idx = parse_led(state, left_idx, next_type);
  }
  return left_idx;
}

int parse_nud(ParserState &state) {
  if (state.error != ParseError::NONE)
    return -1;
  if (state.token_idx >= state.tokens.size()) {
    state.error = ParseError::UNEXPECTED_END;
    return -1;
  }

  Token tok = state.tokens[state.token_idx++];

  if (tok.type == TokenType::NUMBER || tok.type == TokenType::DECIMAL_LITERAL) {
    ASTNode node;
    node.type = ASTNodeType::LITERAL;
    node.value = parse_number_string(tok.value);
    state.ast_pool.push_back(node);
    return static_cast<int>(state.ast_pool.size() - 1);
  }

  if (tok.type == TokenType::IDENTIFIER) {
    if (state.token_idx >= state.tokens.size() ||
        state.tokens[state.token_idx].type != TokenType::LPAREN) {
      ASTNode node;
      node.type = ASTNodeType::LITERAL;
      if (tok.value == "e") {
        ExactValue ev;
        ExactTerm t;
        t.a = 1;
        t.b = 1;
        t.c = 1;
        t.root_degree = 2;
        t.vars.push_back({"e", 1});
        ev.terms.push_back(t);
        ev.cached_double = 2.718281828459045;
        ev.simplify();
        node.value = ev;
      } else if (tok.value == "pi" || tok.value == "PI" || tok.value == "π") {
        ExactValue ev;
        ExactTerm t;
        t.a = 1;
        t.b = 1;
        t.c = 1;
        t.root_degree = 2;
        t.vars.push_back({get_pi_symbol(), 1});
        ev.terms.push_back(t);
        ev.cached_double = std::numbers::pi;
        ev.simplify();
        node.value = ev;
      } else if (tok.value == "i") {
        ExactValue ev;
        ExactTerm t;
        t.a = 1;
        t.b = 1;
        t.c = 1;
        t.root_degree = 2;
        t.is_imaginary = true;
        ev.terms.push_back(t);
        ev.cached_imag = 1.0;
        ev.simplify();
        node.value = ev;
      } else {
        ExactValue ev;
        ExactTerm t;
        t.a = 1;
        t.b = 1;
        t.c = 1;
        t.root_degree = 2;
        t.vars.push_back({tok.value, 1});
        ev.terms.push_back(t);
        if (tok.value == "pi") ev.cached_double = std::numbers::pi;
        else if (tok.value == "e") ev.cached_double = std::numbers::e;
        else ev.cached_double = 1.0;
        ev.simplify();
        node.value = ev;
      }
      state.ast_pool.push_back(node);
      return static_cast<int>(state.ast_pool.size() - 1);
    }

    state.token_idx++;
    std::vector<int> args;
    while (true) {
      int arg_idx = parse_expression(state, 0);
      if (state.error != ParseError::NONE)
        return -1;
      args.push_back(arg_idx);

      if (state.token_idx < state.tokens.size() &&
          state.tokens[state.token_idx].type == TokenType::RPAREN) {
        state.token_idx++;
        break;
      } else if (state.token_idx < state.tokens.size() &&
                 state.tokens[state.token_idx].type == TokenType::COMMA) {
        state.token_idx++;
      } else {
        state.error = ParseError::EXPECTED_RPAREN;
        return -1;
      }
    }

    ASTNode node;
    node.type = ASTNodeType::FUNCTION;
    node.func_name = tok.value;
    node.args = args;
    if (args.size() == 1)
      node.right_idx = args[0];
    else if (args.size() >= 2) {
      node.left_idx = args[0];
      node.right_idx = args[1];
    }

    state.ast_pool.push_back(node);
    return static_cast<int>(state.ast_pool.size() - 1);
  }

  if (tok.type == TokenType::MINUS) {
    int right_idx = parse_expression(state, 25);
    ASTNode node;
    node.type = ASTNodeType::UNARY;
    node.op = '-';
    node.right_idx = right_idx;
    state.ast_pool.push_back(node);
    return static_cast<int>(state.ast_pool.size() - 1);
  }

  if (tok.type == TokenType::LPAREN) {
    int inner_idx = parse_expression(state, 0);
    if (state.error != ParseError::NONE)
      return -1;
    if (state.token_idx >= state.tokens.size() ||
        state.tokens[state.token_idx].type != TokenType::RPAREN) {
      state.error = ParseError::EXPECTED_RPAREN;
      return -1;
    }
    state.token_idx++;
    return inner_idx;
  }

  if (tok.type == TokenType::LBRACKET) {
    ASTNode node;
    node.type = ASTNodeType::ARRAY;
    if (state.token_idx < state.tokens.size() &&
        state.tokens[state.token_idx].type != TokenType::RBRACKET) {
      while (true) {
        node.args.push_back(parse_expression(state, 0));
        if (state.error != ParseError::NONE)
          return -1;
        if (state.token_idx < state.tokens.size() &&
            state.tokens[state.token_idx].type == TokenType::COMMA) {
          state.token_idx++;
        } else {
          break;
        }
      }
    }
    if (state.token_idx >= state.tokens.size() ||
        state.tokens[state.token_idx].type != TokenType::RBRACKET) {
      state.error = ParseError::UNEXPECTED_TOKEN;
      state.error_extra = "expected ']'";
      return -1;
    }
    state.token_idx++;
    state.ast_pool.push_back(node);
    return static_cast<int>(state.ast_pool.size() - 1);
  }

  state.error = ParseError::UNEXPECTED_TOKEN;
  state.error_extra = tok.value;
  return -1;
}

int parse_led(ParserState &state, int left_idx, TokenType op_type) {
  if (state.error != ParseError::NONE)
    return -1;
  ASTNode node;
  node.type = ASTNodeType::BINARY;
  node.left_idx = left_idx;

  if (op_type == TokenType::PLUS)
    node.op = '+';
  else if (op_type == TokenType::MINUS)
    node.op = '-';
  else if (op_type == TokenType::MULTIPLY)
    node.op = '*';
  else if (op_type == TokenType::DIVIDE)
    node.op = '/';
  else if (op_type == TokenType::POWER)
    node.op = '^';
  else if (op_type == TokenType::EQUALS)
    node.op = '=';
  else if (op_type == TokenType::LESS_THAN)
    node.op = '<';
  else if (op_type == TokenType::GREATER_THAN)
    node.op = '>';
  else if (op_type == TokenType::LESS_EQUALS)
    node.op = 'L'; // 'L' for <=
  else if (op_type == TokenType::GREATER_EQUALS)
    node.op = 'G'; // 'G' for >=
  else if (op_type == TokenType::FACTORIAL)
    node.op = '!';

  if (op_type == TokenType::FACTORIAL) {
    node.type = ASTNodeType::UNARY;
    state.ast_pool.push_back(node);
    return static_cast<int>(state.ast_pool.size() - 1);
  }

  int p = get_precedence(op_type);
  if (op_type == TokenType::POWER)
    p--;

  node.right_idx = parse_expression(state, p);
  state.ast_pool.push_back(node);
  return static_cast<int>(state.ast_pool.size() - 1);
}
