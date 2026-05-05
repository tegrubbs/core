#ifndef PH_EXPR_H
#define PH_EXPR_H

/** \file phExpr.h
    \brief Compile a coordinate expression string into an evaluable function.
    \details Supports variables x, y, z; operators +, -, *, /, unary -;
             functions sqrt, abs, min, max; and parentheses.
             No spaces are allowed between tokens.
             Example: sqrt(x*x+y*y+z*z)-0.5
*/

#include <functional>
#include <string>

namespace ph {

typedef std::function<double(double, double, double)> LevelSetFn;

/** Compile \a expr into a callable that takes (x,y,z) and returns a double.
    Aborts with a descriptive message on syntax errors. */
LevelSetFn compileLevelSetExpr(const std::string& expr);

}

#endif
