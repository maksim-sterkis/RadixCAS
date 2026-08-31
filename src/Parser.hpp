#pragma once
#include "AST.hpp"

int get_precedence(TokenType type);
int parse_expression(ParserState &state, int binding_power);
int parse_nud(ParserState &state);
int parse_led(ParserState &state, int left_idx, TokenType op_type);
