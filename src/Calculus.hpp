#pragma once
#include "AST.hpp"
#include <string>

int differentiate_ast(ParserState &state, int node_idx,
                      const std::string &target_var);
ExactValue integrate_polynomial(const ExactValue &expr,
                                const std::string &target_var,
                                ParserState &state);
ExactValue differentiate_polynomial(const ExactValue &expr,
                                    const std::string &target_var,
                                    ParserState &state);
