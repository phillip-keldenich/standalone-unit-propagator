#ifndef SP_LITERAL_OPS_H_INCLUDED_
#define SP_LITERAL_OPS_H_INCLUDED_

#include "types.h"
#include <vector>

namespace sprop {
namespace lit {

/**
 * @brief Internal literal negation.
 *
 * @param l
 * @return constexpr Lit
 */
static inline constexpr Lit negate(Lit l) noexcept {
    return l ^ Lit(1);
}

/**
 * @brief Extract the variable from a literal.
 *
 * @param l
 * @return constexpr Lit
 */
static inline constexpr Var var(Lit l) noexcept {
    return l >> 1;
}

/**
 * @brief Turn a variable into its positive literal.
 */
static inline constexpr Lit positive_lit(Var v) noexcept {
    return v << 1;
}

/**
 * @brief Turn a variable into its negative literal.
 */
static inline constexpr Lit negative_lit(Var v) noexcept {
    return (v << 1) + 1;
}

/**
 * @brief Check for negative literal.
 *
 * @param l
 * @return true if the literal is negative.
 * @return false if the literal is positive.
 */
static inline constexpr bool positive(Lit l) noexcept {
    return !(l & Lit(1));
}

/**
 * @brief Check for negative literal.
 *
 * @param l
 * @return true if the literal is negative.
 * @return false if the literal is positive.
 */
static inline constexpr bool negative(Lit l) noexcept {
    return l & Lit(1);
}

/**
 * @brief Turn a literal into its positive version.
 */
static inline constexpr Lit absolute(Lit l) noexcept {
    return l & ~Lit(1);
}

/**
 * @brief Check if a literal is true in a given assignment.
 */
template<typename BitsetType>
static inline bool is_true_in(Lit l, const BitsetType& assignment) noexcept
{
    bool value(assignment[var(l)]);
    return negative(l) ? !value : value;
}

/**
 * @brief Check if a literal is false in a given assignment.
 */
template<typename BitsetType>
static inline bool is_false_in(Lit l, const BitsetType& assignment) noexcept
{
    return !is_true_in(l, assignment);
}

}
}

#endif
