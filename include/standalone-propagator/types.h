#ifndef SP_TYPES_H_INCLUDED_
#define SP_TYPES_H_INCLUDED_

#include <cstdint>
#include <cstddef>
#include <limits>
#include <ranges>
#include <concepts>

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
using ClausePtrRange = std::ranges::subrange<const Lit*>;

/**
 * A clause range using mutable pointers.
 */
using MutClausePtrRange = std::ranges::subrange<Lit*>;

/**
 * A value that indicates an invalid variable/literal/clause.
 */
static constexpr Lit NIL = std::numeric_limits<Lit>::max();

}

#endif
