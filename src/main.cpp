#include "AST.hpp"
#include "Config.hpp"
#include "Diagnostics.hpp"
#include "Engine.hpp"
#include "Evaluator.hpp"
#include "ExactValue.hpp"
#include "Parser.hpp"
#include "Token.hpp"
#include "Tokenizer.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

// ── Matrix shorthand pre-processor ───────────────────────────────────────────
// Converts natural syntax into the canonical [[r1c1, r1c2], [r2c1, r2c2]] form
// before tokenisation.
//
// Rules (only applied to simple bracket groups with NO nested '['):
//   - Spaces separate columns within a row
//   - Commas separate rows
//
// Examples:
//   [1 2 3]       ->  [[1, 2, 3]]          (1×3 row vector)
//   [1 2, 3 4]    ->  [[1, 2], [3, 4]]     (2×2 matrix)
//   [1 2 3, 4 5 6] -> [[1, 2, 3], [4, 5, 6]] (2×3)
//   [[1,2],[3,4]] ->  unchanged (already canonical)
static std::string preprocess_matrix_syntax(const std::string &input) {
  std::string result;
  result.reserve(input.size() * 2);
  size_t i = 0;
  while (i < input.size()) {
    if (input[i] == '[') {
      // Peek ahead: if the very next non-space char is '[', it's already
      // canonical — copy the entire outer bracket verbatim.
      size_t j = i + 1;
      while (j < input.size() && input[j] == ' ') ++j;
      if (j < input.size() && input[j] == '[') {
        // Already canonical: copy until matching ']'
        int depth = 0;
        while (i < input.size()) {
          if (input[i] == '[') ++depth;
          else if (input[i] == ']') --depth;
          result += input[i++];
          if (depth == 0) break;
        }
        continue;
      }
      // Simple bracket group — collect everything up to the matching ']'
      ++i;
      std::string inner;
      int depth = 1;
      while (i < input.size() && depth > 0) {
        if (input[i] == '[') ++depth;
        else if (input[i] == ']') { --depth; if (depth == 0) { ++i; break; } }
        inner += input[i++];
      }
      // Split inner by commas to get rows
      std::vector<std::string> rows;
      std::string cur_row;
      for (char c : inner) {
        if (c == ',') { rows.push_back(cur_row); cur_row.clear(); }
        else cur_row += c;
      }
      rows.push_back(cur_row);
      // For each row, split by whitespace to get individual tokens (columns)
      result += '[';
      for (size_t r = 0; r < rows.size(); ++r) {
        std::istringstream iss(rows[r]);
        std::vector<std::string> cols;
        std::string tok;
        while (iss >> tok) cols.push_back(tok);
        result += '[';
        for (size_t c = 0; c < cols.size(); ++c) {
          result += cols[c];
          if (c + 1 < cols.size()) result += ", ";
        }
        result += ']';
        if (r + 1 < rows.size()) result += ", ";
      }
      result += ']';
      continue;
    }
    result += input[i++];
  }
  return result;
}

int main() {
  std::cout << "=== RadixCAS (Algebraic Computer Algebra System) ==="
            << std::endl;

  OutputMode active_mode = OutputMode::AUTO;
  while (true) {
    std::cout << "Please select an output mode:\n";
    std::cout << "  [A] Auto (clean whole numbers/decimals, fallback to exact "
                 "fractions)\n";
    std::cout << "  [B] Decimal (always approximate values)\n";
    std::cout << "  [C] Fraction (always exact fraction/radical format)\n";
    std::cout << "Your selection (A/B/C, or 'exit'): ";

    std::string mode_choice;
    if (!std::getline(std::cin, mode_choice)) {
      return 0;
    }

    if (mode_choice.empty())
      continue;

    std::string lower_choice = mode_choice;
    std::transform(lower_choice.begin(), lower_choice.end(),
                   lower_choice.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (lower_choice == "exit") {
      std::cout << "Goodbye from RadixCAS!\n";
      return 0;
    }

    char choice = std::toupper(mode_choice[0]);
    if (choice == 'A') {
      active_mode = OutputMode::AUTO;
      std::cout << ">> Output Mode set to AUTO.\n" << std::endl;
      break;
    } else if (choice == 'B') {
      active_mode = OutputMode::DECIMAL;
      std::cout << ">> Output Mode set to DECIMAL.\n" << std::endl;
      break;
    } else if (choice == 'C') {
      active_mode = OutputMode::FRACTION;
      std::cout << ">> Output Mode set to FRACTION.\n" << std::endl;
      break;
    } else {
      std::cout << "Invalid choice. Please select A, B, C, or 'exit'.\n"
                << std::endl;
    }
  }

  std::cout << "Type 'exit' to close.\n" << std::endl;
  std::string expr;
  ParserState state;

  while (true) {
    std::cout << "Enter expression: ";

    if (std::cin.fail())
      std::cin.clear();
    if (!std::getline(std::cin, expr))
      break;
    if (expr == "exit" || expr == "quit")
      break;

    if (expr.empty())
      continue;

    state.tokens.clear();
    state.ast_pool.clear();
    state.token_idx = 0;
    state.error = ParseError::NONE;
    state.error_extra = "";

    // Pre-process natural matrix shorthand: [1 2, 3 4] -> [[1, 2], [3, 4]]
    std::string processed_expr = preprocess_matrix_syntax(expr);
    tokenize(processed_expr, state);

    if (state.error == ParseError::NONE) {
      // Variable Assignment checking (e.g. x = 5)
      if (state.tokens.size() >= 3 &&
          state.tokens[0].type == TokenType::IDENTIFIER &&
          state.tokens[1].type == TokenType::EQUALS) {

        std::string var_name = state.tokens[0].value;
        if (is_function(var_name) || var_name == "e" || var_name == "pi" ||
            var_name == "i") {
          std::cout << "Error: Cannot assign to protected CAS keyword '"
                    << var_name << "'" << std::endl
                    << std::endl;
          continue;
        }

        // Parse the expression to the right of '='
        ParserState sub_state;
        for (size_t i = 2; i < state.tokens.size(); ++i) {
          sub_state.tokens.push_back(state.tokens[i]);
        }

        int root_idx = parse_expression(sub_state, 0);
        if (sub_state.error == ParseError::NONE &&
            sub_state.token_idx < sub_state.tokens.size()) {
          Token extra = sub_state.tokens[sub_state.token_idx];
          if (extra.type != TokenType::END) {
            sub_state.error = ParseError::UNEXPECTED_TOKEN;
            sub_state.error_extra = extra.value;
          }
        }

        if (sub_state.error == ParseError::NONE) {
          ExactValue val = evaluate(sub_state, root_idx);
          if (sub_state.error == ParseError::NONE) {
            global_variables[var_name] = val;
            std::cout << "Stored: " << var_name << " = "
                      << format_output(val, active_mode) << std::endl
                      << std::endl;
            continue;
          }
        }

        print_diagnostic(sub_state);
        std::cout << std::endl;
        continue;
      }

      // Standard Expression parsing and Evaluation
      int root_idx = parse_expression(state, 0);

      if (state.error == ParseError::NONE &&
          state.token_idx < state.tokens.size()) {
        Token extra = state.tokens[state.token_idx];
        if (extra.type != TokenType::END) {
          state.error = ParseError::UNEXPECTED_TOKEN;
          state.error_extra = extra.value;
        }
      }

      if (state.error == ParseError::NONE) {
        root_idx = simplify_ast(state, root_idx);
        ExactValue result = evaluate(state, root_idx);

        if (state.error == ParseError::NONE) {
          std::cout << "Result: " << format_output(result, active_mode) << "\n"
                    << std::endl;
          continue;
        }
      }
    }

    print_diagnostic(state);
    std::cout << std::endl;
  }

  std::cout << "Goodbye from RadixCAS!" << std::endl;
  return 0;
}