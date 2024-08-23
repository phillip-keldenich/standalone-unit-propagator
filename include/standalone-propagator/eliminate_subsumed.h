#ifndef SP_ELIMINATE_SUBSUMED_H_INCLUDED_
#define SP_ELIMINATE_SUBSUMED_H_INCLUDED_

#include "stamp_set.h"
#include "types.h"

namespace sprop {

/**
 * @brief Class to implement elimination of subsumed clauses
 * using a 2-watch scheme.
 */
template<typename ClauseType>
class SubsumptionChecker {
  public:
    SubsumptionChecker(std::vector<ClauseType>& clauses, Var n_all) :
        m_nv(n_all),
        m_nl(2 * n_all),
        m_clauses(clauses),
        m_in_clause(m_nl),
        m_watching_clauses(m_nl)
    {
        p_init_watches();
    }

    void remove_subsumed() {
        for(ClauseRef c = 0, n = m_clauses.size(); c < n; ++c) {
            p_empty_if_subsumed(c);
        }
        auto deleted_begin = std::remove_if(m_clauses.begin(), m_clauses.end(), 
                                            [] (const ClauseType& cl) { return cl.empty(); });
        m_clauses.erase(deleted_begin, m_clauses.end());
    }

  private:
    bool p_walk_watch_list(ClauseRef index, Lit l) {
        auto& watch_list = m_watching_clauses[l];
        auto end = watch_list.end();
        auto out = watch_list.begin();
        bool subsumed = false;
        for(auto in = watch_list.begin(); in != end; ++in) {
            ClauseRef cother = *in;
            // we cannot subsume ourself. stay in the watch list.
            if(cother == index) { *out++ = cother; continue; }
            const ClauseType& other_lits = m_clauses[cother];
            // subsumed clauses do not participate in subsumption anymore;
            // they are dropped from watch lists without replacement when we
            // encounter them here.
            if(other_lits.empty()) { continue; }
            // find replacement watch (must not be in the current clause).
            auto replacement = std::find_if(other_lits.begin(), other_lits.end(), [&] (Lit l) {
                return !m_in_clause.count(l);
            });
            if(replacement == other_lits.end()) {
                // cother subsumes us.
                subsumed = true;
                // copy remaining watching clauses.
                out = std::copy(in, end, out);
                break;
            } else {
                // cother does not subsume us.
                m_watching_clauses[*replacement].push_back(cother);
            }
        }
        // trim watch list
        watch_list.erase(out, end);
        return subsumed;
    }

    void p_empty_if_subsumed(ClauseRef index) {
        ClauseType& clause = m_clauses[index];
        m_in_clause.assign(clause.begin(), clause.end());
        for(Lit l : clause) {
            if(p_walk_watch_list(index, l)) {
                clause.clear();
                return;
            }
        }
    }

    void p_init_watches() {
        for(std::size_t ci = 0, cn = m_clauses.size(); ci < cn; ++ci) {
            const auto& cl = m_clauses[ci];
            m_watching_clauses[cl[0]].push_back(ClauseRef(ci));
        }
    }

    Var m_nv;
    Lit m_nl;
    std::vector<ClauseType>& m_clauses;
    StampSet<Lit, std::uint16_t> m_in_clause;
    std::vector<std::vector<ClauseRef>> m_watching_clauses;
};

/**
 * @brief Eliminate subsumed clauses from a vector of clauses.
 */
template<typename ClauseType>
inline void eliminate_subsumed(std::vector<ClauseType>& clauses, Var n_all) {
    SubsumptionChecker<ClauseType> subsumption_checker{clauses, n_all};
    subsumption_checker.remove_subsumed();
}

}

#endif
