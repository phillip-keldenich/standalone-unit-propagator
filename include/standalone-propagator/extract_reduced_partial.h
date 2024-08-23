#ifndef SP_EXTRACT_REDUCED_PARTIAL_H_INCLUDED_
#define SP_EXTRACT_REDUCED_PARTIAL_H_INCLUDED_

#include <vector>
#include <cassert>
#include "types.h"
#include "propagator.h"
#include "eliminate_subsumed.h"

namespace sprop {

static inline constexpr Lit FIXED_TRUE = NIL - 1;
static inline constexpr Lit FIXED_FALSE = NIL - 2;

/**
 * @brief Class for extraction of a reduced 
 *        formula/model from a propagator containing a 
 *        non-conflicting partial assignment.
 * The reduced formula represents the problem of finding
 * a satisfying assignment for the original formula that
 * extends the given partial assignment.
 */
class ReducedPartialExtractor {
  public:
    ReducedPartialExtractor() = default;

    /**
     * @brief Extracts the reduced formula from the given propagator.
     */
    inline void extract(const Propagator& propagator);

    /**
     * @brief Returns the reduced clauses.
     */
    const std::vector<std::vector<Lit>> &reduced_clauses() const {
        return m_reduced_clauses;
    }

    /**
     * @brief Number of variables, post-reduction.
     */
    std::size_t reduced_num_vars() const noexcept {
        return m_new_to_old.size() / 2;
    }

    /**
     * @brief Number of clauses, post-reduction.
     */
    std::size_t reduced_num_clauses() const noexcept {
        return m_reduced_clauses.size();
    }

    /**
     * Translate the given post-reduced literal to the
     * corresponding pre-reduced literal.
     */
    Lit translate_to_old(Lit lnew) const {
        return m_new_to_old[lnew];
    }

    /**
     * Translate the given pre-reduced literal to the
     * corresponding post-reduced literal.
     * May return FIXED_TRUE or FIXED_FALSE instead of a real literal.
     */
    Lit translate_to_new(Lit old) const {
        return m_old_to_new[old];
    }

  private:
    // Is the given old literal true?
    std::vector<bool> m_old_lit_is_true;
    
    // Is the given old literal false?
    std::vector<bool> m_old_lit_is_false;
    
    // What pre-reduced literal does the given post-reduced literal represent?
    std::vector<Lit> m_new_to_old;
    
    // What post-reduced literal does the given pre-reduced literal represent?
    // NIL - 1 => fixed true, NIL - 2 => fixed false
    std::vector<Lit> m_old_to_new;

    // The non-satisfied clauses in the reduced formula,
    // with fixed-false literals removed.
    std::vector<std::vector<Lit>> m_reduced_clauses;

    // Buffer for new clauses.
    std::vector<Lit> m_new_clause_buffer;

    void p_init_extraction(const Propagator& propagator) {
        std::size_t nv = propagator.num_vars();
        std::size_t nl = 2 * nv;
        m_old_lit_is_false.assign(nl, false);
        m_old_lit_is_true.assign(nl, false);
        for(Lit l : propagator.get_trail()) {
            m_old_lit_is_true[l] = true;
            m_old_lit_is_false[lit::negate(l)] = true;
        }
        m_new_to_old.clear();
        m_old_to_new.clear();
        m_reduced_clauses.clear();
    }

    void p_make_literal_maps() {
        std::size_t nlold = m_old_lit_is_true.size();
        Lit nlnew = 0;
        for(Lit l = 0; l < nlold; l += 2) {
            assert(lit::positive(l));
            assert(lit::negative(l+1));
            if(m_old_lit_is_true[l]) {
                m_old_to_new.push_back(FIXED_TRUE);
                m_old_to_new.push_back(FIXED_FALSE);
            } else if(m_old_lit_is_false[l]) {
                m_old_to_new.push_back(FIXED_FALSE);
                m_old_to_new.push_back(FIXED_TRUE);
            } else {
                m_old_to_new.push_back(nlnew);
                m_old_to_new.push_back(nlnew + 1);
                m_new_to_old.push_back(l);
                m_new_to_old.push_back(l + 1);
                nlnew += 2;
            }
        }
    }

    void p_translate_binaries(const Propagator& propagator) {
        // translate binaries:
        for(Lit l1 : propagator.all_literals()) {
            if(m_old_lit_is_false[l1]) {
                // the old literal is false; this
                // means that the partner literal
                // is already assigned true.
                continue;
            }
            if(m_old_lit_is_true[l1]) {
                // the old literal is true; this
                // means that the clause is satisfied.
                continue;
            }
            for(Lit l2 : propagator.binary_partners_of(l1)) {
                if(m_old_lit_is_true[l2]) continue;
                if(l1 < l2) {
                    m_reduced_clauses.push_back(std::vector<Lit>{m_old_to_new[l1], m_old_to_new[l2]});
                }
            }
        }
    }

    void p_translate_clause(ClausePtrRange literals) {
        m_new_clause_buffer.clear();
        for(Lit l : literals) {
            if(m_old_lit_is_true[l]) return;
            if(m_old_lit_is_false[l]) continue;
            m_new_clause_buffer.push_back(m_old_to_new[l]);
        }
        assert(m_new_clause_buffer.size() > 1);
        m_reduced_clauses.push_back(m_new_clause_buffer);
    }

    void p_translate_clauses(const Propagator& propagator) {
        // no need to translate unaries!
        p_translate_binaries(propagator);
        // translate longer clauses:
        for(ClauseRef cref = propagator.first_longer_clause(); 
            cref < propagator.longer_clause_end(); cref = propagator.next_clause(cref)) 
        {
            p_translate_clause(propagator.lits_of(cref));
        }
    }
};

void ReducedPartialExtractor::extract(const Propagator& propagator) {
    p_init_extraction(propagator);
    p_make_literal_maps();
    p_translate_clauses(propagator);
    eliminate_subsumed(m_reduced_clauses, reduced_num_vars());
}

}

#endif
