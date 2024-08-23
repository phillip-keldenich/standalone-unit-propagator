#ifndef SP_REASON_H_INCLUDED_
#define SP_REASON_H_INCLUDED_

#include "types.h"

namespace sprop {

/**
 * @brief A reason for a propagated literal.
 * Its either a decision (reason_length == 0),
 * a unary clause (reason_length == 1, clause in literals[0]),
 * a binary clause (reason_length == 2, clause in literals),
 * or a longer clause (clause referred to by clause).
 */
struct Reason {
    /**
     * @brief Type to create a reason from a decision.
     */
    struct Decision {};

    /**
     * @brief Type to create a reason from a unary clause.
     */
    struct Unary {
        Lit lit;
    };

    /**
     * @brief Type to create a reason from a binary clause.
     */
    struct Binary {
        Lit lit1, lit2;
    };

    /**
     * @brief Type to create a reason from a longer clause.
     */
    struct Clause {
        ClauseLen length;
        ClauseRef clause;
    };

    /* implicit */ Reason(Decision) noexcept : reason_length(0) {}

    /* implicit */ Reason(Unary unary) noexcept : reason_length(1) {
        literals[0] = unary.lit;
    }

    /* implicit */ Reason(Binary b) noexcept : reason_length(2) {
        literals[0] = b.lit1;
        literals[1] = b.lit2;
    }

    /* implicit */ Reason(Clause c) noexcept : reason_length(c.length) {
        clause = c.clause;
    }

    ClauseLen reason_length; //< the length of the reason
    union {
        ClauseRef clause; //< the clause reference
        Lit literals[2];  //< the literals of the reason, if length <= 2
    };

    /**
     * No matter the type of reason, this function returns a range of literals.
     */
    template<typename ClauseContainer>
    ClausePtrRange lits(const ClauseContainer& db) const noexcept {
        switch (reason_length) {
        case 0:
            return {nullptr, nullptr};
        case 1:
            return {+literals, literals + 1};
        case 2:
            return {+literals, literals + 2};
        default:
            return db.lits_of(clause);
        }
    }
};

}

#endif
