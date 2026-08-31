#pragma once
#include "AST.hpp"
#include <string_view>

void tokenize(std::string_view input, ParserState &state);
