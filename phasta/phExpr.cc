#include "phExpr.h"
#include <pcu_util.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace ph {

/* ---------------------------------------------------------------------------
 * Recursive-descent expression parser
 *
 * Grammar:
 *   expr    := term (('+' | '-') term)*
 *   term    := unary (('*' | '/') unary)*
 *   unary   := '-' unary | power
 *   power   := primary (('^' | '**') unary)?   (right-associative)
 *   primary := number | 'x' | 'y' | 'z'
 *            | func '(' expr (',' expr)* ')'
 *            | '(' expr ')'
 *   func    := 'sqrt' | 'abs' | 'min' | 'max'
 *
 * The expression is compiled once into a nested std::function so that
 * repeated evaluation (one call per mesh vertex) does not re-parse.
 * --------------------------------------------------------------------------*/

namespace {

struct ExprParser {
  const char* p;

  ExprParser(const char* expr) : p(expr) {}

  LevelSetFn compile() {
    LevelSetFn f = parseExpr();
    skipWS();
    PCU_ALWAYS_ASSERT_VERBOSE(*p == '\0',
        "levelSetExpr: unexpected character — check expression syntax\n");
    return f;
  }

  void skipWS() {
    while (*p == ' ' || *p == '\t') p++;
  }

  LevelSetFn parseExpr() {
    LevelSetFn left = parseTerm();
    skipWS();
    while (*p == '+' || *p == '-') {
      char op = *p++;
      LevelSetFn right = parseTerm();
      if (op == '+')
        left = [left, right](double x, double y, double z) {
          return left(x, y, z) + right(x, y, z);
        };
      else
        left = [left, right](double x, double y, double z) {
          return left(x, y, z) - right(x, y, z);
        };
      skipWS();
    }
    return left;
  }

  LevelSetFn parseTerm() {
    LevelSetFn left = parseUnary();
    skipWS();
    while (*p == '*' || *p == '/') {
      char op = *p++;
      LevelSetFn right = parseUnary();
      if (op == '*')
        left = [left, right](double x, double y, double z) {
          return left(x, y, z) * right(x, y, z);
        };
      else
        left = [left, right](double x, double y, double z) {
          return left(x, y, z) / right(x, y, z);
        };
      skipWS();
    }
    return left;
  }

  LevelSetFn parseUnary() {
    skipWS();
    if (*p == '-') {
      p++;
      LevelSetFn f = parsePower();
      return [f](double x, double y, double z) { return -f(x, y, z); };
    }
    return parsePower();
  }

  LevelSetFn parsePower() {
    LevelSetFn base = parsePrimary();
    skipWS();
    // accept '^' or '**'
    bool isCaret = (*p == '^');
    bool isStarStar = (*p == '*' && *(p+1) == '*');
    if (isCaret || isStarStar) {
      p += isCaret ? 1 : 2;
      LevelSetFn exp = parseUnary();  // right-associative: recurse into unary
      return [base, exp](double x, double y, double z) {
        return pow(base(x, y, z), exp(x, y, z));
      };
    }
    return base;
  }

  LevelSetFn parsePrimary() {
    skipWS();
    if (*p == '(') {
      p++;
      LevelSetFn f = parseExpr();
      skipWS();
      PCU_ALWAYS_ASSERT_VERBOSE(*p == ')',
          "levelSetExpr: expected ')'\n");
      p++;
      return f;
    }
    // numeric literal (handles scientific notation via strtod)
    if (isdigit((unsigned char)*p) || *p == '.') {
      char* end;
      double val = strtod(p, &end);
      PCU_ALWAYS_ASSERT_VERBOSE(end != p,
          "levelSetExpr: invalid numeric literal\n");
      p = end;
      return [val](double, double, double) { return val; };
    }
    // identifier: variable (x, y, z) or function call
    if (isalpha((unsigned char)*p) || *p == '_') {
      const char* nameStart = p;
      while (isalnum((unsigned char)*p) || *p == '_') p++;
      std::string name(nameStart, p);
      skipWS();
      if (*p == '(') {
        p++;  // consume '('
        return parseCall(name);
      }
      if (name == "x") return [](double x, double, double) { return x; };
      if (name == "y") return [](double, double y, double) { return y; };
      if (name == "z") return [](double, double, double z) { return z; };
      PCU_ALWAYS_ASSERT_VERBOSE(0,
          "levelSetExpr: unknown variable — use x, y, or z\n");
    }
    PCU_ALWAYS_ASSERT_VERBOSE(0,
        "levelSetExpr: unexpected character in expression\n");
    return nullptr;  // unreachable
  }

  LevelSetFn parseCall(const std::string& name) {
    // opening '(' already consumed; parse comma-separated argument list
    std::vector<LevelSetFn> args;
    skipWS();
    if (*p != ')') {
      args.push_back(parseExpr());
      skipWS();
      while (*p == ',') {
        p++;
        args.push_back(parseExpr());
        skipWS();
      }
    }
    PCU_ALWAYS_ASSERT_VERBOSE(*p == ')',
        "levelSetExpr: expected ')' after function arguments\n");
    p++;

    if (name == "sqrt") {
      PCU_ALWAYS_ASSERT_VERBOSE(args.size() == 1,
          "levelSetExpr: sqrt takes exactly 1 argument\n");
      LevelSetFn a = args[0];
      return [a](double x, double y, double z) { return sqrt(a(x, y, z)); };
    }
    if (name == "abs") {
      PCU_ALWAYS_ASSERT_VERBOSE(args.size() == 1,
          "levelSetExpr: abs takes exactly 1 argument\n");
      LevelSetFn a = args[0];
      return [a](double x, double y, double z) { return fabs(a(x, y, z)); };
    }
    if (name == "min") {
      PCU_ALWAYS_ASSERT_VERBOSE(args.size() == 2,
          "levelSetExpr: min takes exactly 2 arguments\n");
      LevelSetFn a = args[0], b = args[1];
      return [a, b](double x, double y, double z) {
        return std::min(a(x, y, z), b(x, y, z));
      };
    }
    if (name == "max") {
      PCU_ALWAYS_ASSERT_VERBOSE(args.size() == 2,
          "levelSetExpr: max takes exactly 2 arguments\n");
      LevelSetFn a = args[0], b = args[1];
      return [a, b](double x, double y, double z) {
        return std::max(a(x, y, z), b(x, y, z));
      };
    }
    PCU_ALWAYS_ASSERT_VERBOSE(0,
        "levelSetExpr: unknown function — use sqrt, abs, min, or max\n");
    return nullptr;  // unreachable
  }
};

}  // anonymous namespace

LevelSetFn compileLevelSetExpr(const std::string& expr) {
  ExprParser parser(expr.c_str());
  return parser.compile();
}

}  // namespace ph
