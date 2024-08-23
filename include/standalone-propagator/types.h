#ifndef SP_TYPES_H_INCLUDED_
#define SP_TYPES_H_INCLUDED_

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <limits>
#include <ranges>
#include <concepts>
#include <utility>

namespace sprop {

/**
 * A literal in the internal sense:
 *  - Even numbers are positive literals,
 *  - Odd numbers are negative literals.
 *  - For variables x, we thus have 2 * x and 2 * x + 1 as literals.
 */
using Lit = std::uint32_t;

/**
 * The length of a trail, i.e., the number of literals on the trail.
 * This has to be large enough to accommodate the number of variables + 1.
 */
using TrailLen = std::uint32_t;

/**
 * An integer that is used to refer to variables.
 */
using Var = Lit;

/**
 * A reference to a clause, i.e., a clause begin index,
 * can be used to uniquely identify a clause in a clause database.
 */
using ClauseRef = Lit;

/**
 * A clause length number type, used to store the length of clauses.
 */
using ClauseLen = Lit;

/**
 * A clause range using const pointers.
 */
using ClausePtrRange = std::remove_cvref_t<
    decltype(std::ranges::subrange(std::declval<const Lit*>(), std::declval<const Lit*>()))>;

/**
 * A clause range using mutable pointers.
 */
using MutClausePtrRange = std::remove_cvref_t<
    decltype(std::ranges::subrange(std::declval<Lit*>(), std::declval<Lit*>()))>;

/**
 * A value that indicates an invalid variable/literal/clause.
 */
static constexpr Lit NIL = std::numeric_limits<Lit>::max();

}

#endif
