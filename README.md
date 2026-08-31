# RadixCAS - Algebraic Computer Algebra System

RadixCAS is a powerful, custom-built C++ Computer Algebra System (CAS) capable of exact radical arithmetic, exact fractional math, complex number evaluation, multivariate polynomial expansion, and equation solving.

Unlike standard calculators that rely strictly on floating-point decimal approximations (which inherently lose precision), RadixCAS calculates everything using a custom symbolic algebra engine, ensuring roots and fractions are preserved flawlessly.

## Output Modes

When you launch the calculator, you can select one of three output modes:

1. **[A] Auto Mode**: The default and recommended mode. It displays clean whole numbers or decimals when appropriate (for things like trigonometric evaluation or constants), but falls back to exact fractional and radical formats when evaluating algebraic equations so precision is never lost.
2. **[B] Decimal Mode**: Forces all outputs to be evaluated as a floating-point decimal (e.g., `1/3` outputs `0.3333333333`).
3. **[C] Fraction Mode**: Forces all outputs to remain in their most precise mathematical structure (e.g., leaving answers as `root(2)` or `1/3` unconditionally).

## Core Capabilities

### Basic Arithmetic & Algebra
- Native fraction simplification: `1/2 + 1/3 - 1/6` -> `2/3`
- Implicit multiplication and binomial expansion: `(x + y)^2 - 2xy` -> `x^2 + y^2`
- Imaginary Numbers (`i`): `(2 + 3i) * (2 - 3i)` -> `13`
- Roots and Radicals: `root(18) + root(8)` -> `5root(2)`
- Factorials (`!`): `5!` -> `120`
- Polynomial Division: `(x^2 - 4) / (x - 1)` -> `x + 1 - 3/(x - 1)`

### Built-In Constants
- **`pi`** (or `PI`, `π`): Evaluates symbolically, or approximately if inside trig/approx functions.
- **`e`**: Euler's number.
- **`i`**: The imaginary unit (`root(-1)`).

## Built-In Functions

### 1. `solve(equation, target_variable)`
The crown jewel of RadixCAS. Solves for a specified variable.
- **Linear Equations:** `solve(5x - 7 = 3x + 9, x)`
- **Quadratic Equations:** `solve(x^2 - 4x + 1 = 0, x)` (Natively applies the quadratic formula)
- **Higher-Degree Polynomials:** `solve(x^3 - 6x^2 + 11x - 6 = 0, x)` (Uses the Durand-Kerner numerical method to compute all complex roots)
- **Radical Equations:** `solve(root(x) + 5 = 8, x)`
- **Trigonometric Substitution:** `solve(sin(x)^2 + cos(x) = 1, x)`
- **Inequalities:** `solve(x^2 - 4 > 0, x)` -> `x < -2 or x > 2`

### 2. System `solve(equations_array, variables_array)`
Solves systems of equations natively.
- **Linear Systems:** `solve([2x + 3y = 12, x - y = 1], [x, y])` (Uses Matrix RREF)
- **Non-Linear Polynomial Systems:** `solve([x^2 + y^2 = 25, y = x - 1], [x, y])` (Uses Buchberger's algorithm to compute the Gröbner Basis natively!)

### 3. Transcendental & Trigonometry Functions
- **Trig:** `sin(x)`, `cos(x)`, `tan(x)`, `csc(x)`, `sec(x)`, `cot(x)`
  - Trigonometric inputs are evaluated natively and mapped to exact known values where possible (e.g., `sin(pi/6)` -> `1/2`).
- **Logarithms:** `ln(x)`, `log(base, x)`
  - Applies logarithmic rules implicitly: `ln(e^5) - ln(e^2)` -> `3`.

### 4. Calculus (Derivatives, Integrals, Limits)
- **`derivative(expression, variable)`**: Computes the exact symbolic derivative of the expression using the power, product, quotient, and chain rules natively on the Abstract Syntax Tree.
  - Example: `derivative(x^3 * sin(x), x)` -> `3sin(x)x^2 + cos(x)x^3`
- **`integral(expression, variable)`**: Computes the exact indefinite integral of a standard algebraic polynomial.
  - Example: `integral(4x^3 - 6x^2 + 2x - 5, x)` -> `x^4 - 2x^3 + x^2 - 5x`
- **`limit(expression, variable, value)`**: Computes the exact limit of a function as it approaches a value, utilizing L'Hôpital's rule where indeterminate forms occur.
  - Example: `limit(sin(x) / x, x, 0)` -> `1`

### 5. Iterations (Sums & Series)
- **`sum(expression, variable, start, end)`**: Evaluates an exact mathematical summation across a range (capped at 1000 iterations).
  - Example: `sum(x^2, x, 1, 5)` -> `55`
- **`taylor(expression, variable, center, degree)`**: Computes the exact symbolic Taylor Series polynomial for a function up to a certain degree (capped at 25).
  - Example: `taylor(sin(x), x, 0, 5)` -> `x - (x^3)/6 + (x^5)/120`

### 6. Linear Algebra
- **`det(matrix)`**: Computes the determinant of a matrix.
  - Example: `det([[1, 2], [3, 4]])` -> `-2`
- **`invert(matrix)`**: Computes the inverse of an invertible matrix using adjugate methods.
  - Example: `invert([[1, 2], [3, 4]])` -> `[[-2, 1], [3/2, -1/2]]`

### 7. Mathematical Modifiers
- **`approx(expression)`**: Forces the internal expression to evaluate as a decimal floating point, ignoring the global output mode setting. Example: `approx(pi * e)`.
- **`round(expression, decimal_places)`**: Rounds the decimal output of an expression to a set number of places. Example: `round(pi, 2)` -> `3.14`.
- **`root(expression)`**: Standard square root. Equivalent to `expression^(1/2)`.
- **`croot(expression)`**: Standard cube root. Equivalent to `expression^(1/3)`.

---

## How to Build and Run (VS Code)

This project is built using C++ and CMake. Assuming you have cloned this repository and opened the folder in **Visual Studio Code**:

1. Open the Integrated Terminal in VS Code (`Ctrl` + `` ` `` or `Cmd` + `` ` ``).
2. Run the following commands to generate the build files and compile the engine:

```bash
# 1. Generate the build files (only needed the first time)
cmake -S . -B build

# 2. Compile the project
cmake --build build
```

3. **Run the executable:**
Depending on your operating system, the compiled executable will be located in slightly different places.

**For Mac and Linux:**
```bash
./build/calc
```

**For Windows:**
```cmd
.\build\Debug\calc.exe
```
*(If you are using MinGW or Ninja on Windows instead of the default MSVC, it will just be `.\build\calc.exe`)*
