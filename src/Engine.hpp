#pragma once
#include "AST.hpp"

ExactValue add(ExactValue val1, ExactValue val2, ParserState &state);
ExactValue subtract(ExactValue val1, ExactValue val2, ParserState &state);
ExactValue multiply(ExactValue val1, ExactValue val2, ParserState &state);
ExactValue divide(ExactValue val1, ExactValue val2, ParserState &state);
ExactValue power(ExactValue val, int n, ParserState &state);
ExactValue compute_root(ExactValue arg, int degree, ParserState &state);

RationalValue get_rational_form(ExactValue ev, ParserState &state);
std::string format_output(const ExactValue &ev, OutputMode mode);
ExactValue parse_number_string(const std::string &val);
