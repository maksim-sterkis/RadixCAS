#include "../src/Tokenizer.hpp"
#include "../src/Parser.hpp"
#include "../src/Evaluator.hpp"
#include "../src/AST.hpp"
#include "../src/Config.hpp"
#include <iostream>

int main() {
    ParserState state;
    tokenize("x20", state);
    std::cout << "Tokens:\n";
    for(auto t : state.tokens) {
        std::cout << (int)t.type << " : " << t.value << "\n";
    }
    int ast = parse_expression(state, 0);
    ExactValue val = evaluate(state, ast);
    std::cout << "Result: " << to_exact_string(val) << "\n";
    return 0;
}
