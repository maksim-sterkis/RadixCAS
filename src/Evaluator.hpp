#pragma once
#include "AST.hpp"

ExactValue substitute_variables(const ExactValue &ev, ParserState &state);
ExactValue lookup_trig(const std::string &func, int ref, bool positive,
                       ParserState &state);
int simplify_ast(ParserState &state, int node_idx);
ExactValue evaluate(ParserState &state, int node_idx);
