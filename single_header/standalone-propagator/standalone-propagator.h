#ifndef STANDALONE_PROPAGATOR_STANDALONE_PROPAGATOR_H_INCLUDED_
#define STANDALONE_PROPAGATOR_STANDALONE_PROPAGATOR_H_INCLUDED_

/// DO NOT EDIT THIS AUTO-GENERATED FILE

/// Standard library includes
#include <format>
#include <sstream>
#include <cassert>
#include <ranges>
#include <concepts>
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <optional>
#include <cstddef>
#include <exception>
#include <utility>
#include <cmath>
#include <limits>
#include <cstdint>
#include <string>

/// Project headers concatenated into a single header
/// Original header: #include "types.h"
#ifndef SP_TYPES_H_INCLUDED_
#define SP_TYPES_H_INCLUDED_


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
/// End original header: 'types.h'

/// Original header: #include "reason.h"
#ifndef SP_REASON_H_INCLUDED_
#define SP_REASON_H_INCLUDED_


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
/// End original header: 'reason.h'

/// Original header: #include "unsat_exception.h"
#ifndef SP_UNSAT_EXCEPTION_H_INCLUDED_
#define SP_UNSAT_EXCEPTION_H_INCLUDED_


namespace sprop {

class UNSATException : public std::exception {
    public:
        UNSATException() = default;
    
        const char* what() const noexcept override {
            return "UNSAT";
        }
};

}

#endif
/// End original header: 'unsat_exception.h'

/// Original header: #include "stamp_set.h"
#ifndef SP_STAMP_SET_H_INCLUDED_
#define SP_STAMP_SET_H_INCLUDED_


namespace sprop {

/**
 * @brief A set of integer values implemented using a stamp value.
 */
template<typename ValueType, typename StampType = std::uint32_t>
requires std::is_integral_v<ValueType> && std::is_unsigned_v<ValueType> &&
         std::is_integral_v<StampType> && std::is_unsigned_v<StampType>
class StampSet
{
  private:
    std::vector<StampType> m_stamps;
    StampType m_current_stamp;

  public:
    explicit StampSet(ValueType universe_size) :
        m_stamps(universe_size, StampType(0)),
        m_current_stamp(1)
    {}

    StampSet(const StampSet&) = default;
    StampSet &operator=(const StampSet&) = default;
    StampSet(StampSet&&) noexcept = default;
    StampSet &operator=(StampSet&&) noexcept = default;

    std::size_t universe_size() const noexcept {
        return m_stamps.size();
    }

    void clear() noexcept {
        if(++m_current_stamp == 0) {
            std::fill(m_stamps.begin(), m_stamps.end(), StampType(0));
            m_current_stamp = 1;
        }
    }

    template<typename ForwardIterator>
    void assign(ForwardIterator begin, ForwardIterator end) noexcept {
        clear();
        insert(begin, end);
    }

    template<typename ForwardIterator>
    void insert(ForwardIterator begin, ForwardIterator end) noexcept {
        std::for_each(begin, end, [&] (ValueType l) { insert(l); });
    }

    void insert(ValueType v) noexcept {
        m_stamps[v] = m_current_stamp;
    }

    void erase(ValueType v) noexcept {
        m_stamps[v] = 0;
    }

    bool check_insert(ValueType v) noexcept {
        StampType& s = m_stamps[v];
        bool result = (s != m_current_stamp);
        s = m_current_stamp;
        return result;
    }

    bool check_erase(ValueType v) noexcept {
        StampType& s = m_stamps[v];
        bool result = (s == m_current_stamp);
        s = 0;
        return result;
    }

    bool count(ValueType v) const noexcept {
        return m_stamps[v] == m_current_stamp;
    }

    bool contains(ValueType v) const noexcept {
        return count(v);
    }
};

}

#endif
/// End original header: 'stamp_set.h'

/// Original header: #include "literal_ops.h"
#ifndef SP_LITERAL_OPS_H_INCLUDED_
#define SP_LITERAL_OPS_H_INCLUDED_


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
/// End original header: 'literal_ops.h'

/// Original header: #include "eliminate_subsumed.h"
#ifndef SP_ELIMINATE_SUBSUMED_H_INCLUDED_
#define SP_ELIMINATE_SUBSUMED_H_INCLUDED_


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
/// End original header: 'eliminate_subsumed.h'

/// Original header: #include "model_builder.h"
#ifndef SP_MODEL_BUILDER_H_INCLUDED_
#define SP_MODEL_BUILDER_H_INCLUDED_



namespace sprop {

/**
 * @brief A class that helps building a SAT formula.
 *        Is used to initialize a propagator.
 */
class ModelBuilder {
  public:
    friend class Propagator;
    
    ModelBuilder() = default;

    /**
     * @brief Add a new variable to the model.
     * Manually adding variables is not necessary;
     * adding a clause will automatically increase the number of variables
     * to the largest used variable + 1.
     * It may be helpful to keep track of the number of variables.
     */
    Lit add_variable() noexcept {
        Lit result = m_current_lit;
        m_current_lit += 2;
        return result;
    }

    /**
     * @brief Ensure that the model as at least n variables.
     */
    void reserve_variables(Lit n) noexcept {
        Lit new_max = 2 * n;
        m_current_lit = (std::max)(m_current_lit, new_max);
    }

    /**
     * @brief Get the number of variables in the model.
     */
    Var num_vars() const noexcept {
        return lit::var(m_current_lit);
    }

    /**
     * Add a range of literals to the current clause.
     * Then add that clause to the model.
     */
    template<std::ranges::range Range>
    void add_clause(const Range& clause) {
        m_current_clause_buffer.insert(m_current_clause_buffer.end(), std::begin(clause), std::end(clause));
        p_add();
    }

    /**
     * Add some fixed number of literals to the current clause.
     * Then add that clause to the model.
     */
    template<typename... LitArgs> requires (std::convertible_to<LitArgs, Lit> && ...)
    void add_clause(LitArgs... args) {
        (m_current_clause_buffer.push_back(args), ...);
        p_add();
    }

    /**
     * Add some fixed number of literals to the current clause.
     */
    template<typename... LitArgs> requires (std::convertible_to<LitArgs, Lit> && ...)
    void add_literals(LitArgs... args) {
        (m_current_clause_buffer.push_back(args), ...);
    }

    /**
     * Add a single literal to the current clause.
     */
    void add_literal(Lit l) {
        m_current_clause_buffer.push_back(l);
    }

    /**
     * Finalize the current clause, and add it to the model.
     */
    void finalize_clause() {
        p_add();
    }

    /**
     * Verify that the given trail is a valid assignment for the model.
     */
    std::optional<std::string> verify_trail(const std::vector<Lit>& full_trail) {
        auto n = lit::var(m_current_lit);
        if(full_trail.size() != n) {
            return std::format("Trail has wrong length: expected {}, got {}", n, full_trail.size());
        }
        std::vector<bool> seen(n, false);
        std::vector<bool> assignment(n, false);
        for(Lit l : full_trail) {
            if(lit::var(l) >= n) {
                return std::format("Trail contains variable {} which is not in the model", lit::var(l));
            }
            if(seen[lit::var(l)]) {
                return std::format("Trail contains variable {} multiple times", lit::var(l));
            }
            seen[lit::var(l)] = true;
            if(lit::positive(l)) {
                assignment[lit::var(l)] = true;   
            }
        }
        return verify_assignment(assignment);
    }

    /**
     * Verify that the given assignment is a valid assignment for the model.
     */
    std::optional<std::string> verify_assignment(const std::vector<bool>& assignment) {
        auto n = lit::var(m_current_lit);
        if(assignment.size() != n) {
            return std::format("Assignment has wrong length: expected {}, got {}", n, assignment.size());
        }
        for(Lit l : m_unary_clauses) {
            Var v = lit::var(l);
            if(assignment[v] != lit::positive(l)) {
                return std::format("Unary clause {} is not satisfied in assignment", l);
            }
        }
        for(Lit l1 = 0; l1 < m_binary_clauses.size(); ++l1) {
            Var v1 = lit::var(l1);
            if(assignment[v1] == lit::positive(l1)) continue;
            for(Lit l2 : m_binary_clauses[l1]) {
                Var v2 = lit::var(l2);
                if(assignment[v2] != lit::positive(l2)) {
                    return std::format("Binary clause {} {} is not satisfied in assignment", l1, l2);
                }
            }
        }
        auto satisfies = [&assignment] (Lit l) -> bool {
            return assignment[lit::var(l)] == lit::positive(l); 
        };
        for(const auto& clause : m_longer_clauses) {
            bool satisfied = std::any_of(clause.begin(), clause.end(), satisfies);
            if(!satisfied) {
                // how is fmt::join and formatting of vectors not part of C++20's std::format?
                std::stringstream ss;
                ss << "Longer clause ";
                std::copy(clause.begin(), clause.end(), std::ostream_iterator<Lit>(ss, " "));
                ss << "is not satisfied in assignment";
                return ss.str();
            }
        }
        return std::nullopt;
    }

  private:
    /**
     * Add the current clause to the model and clear it.
     */
    void p_add() {
        if(m_current_clause_buffer.empty()) {
            throw UNSATException();
        }

        std::sort(m_current_clause_buffer.begin(), m_current_clause_buffer.end());
        auto new_end = std::unique(m_current_clause_buffer.begin(), m_current_clause_buffer.end());
        m_current_clause_buffer.erase(new_end, m_current_clause_buffer.end());
        Lit prev = m_current_clause_buffer.front();
        for (Lit cur : std::ranges::subrange(m_current_clause_buffer.begin() + 1, m_current_clause_buffer.end())) {
            if(lit::negate(prev) == cur) {
                m_current_clause_buffer.clear();
                return; // clause is tautology.
            }
        }
        if(m_current_clause_buffer.back() >= m_current_lit) {
            m_current_lit = lit::absolute(m_current_clause_buffer.back()) + 2;
        }
        switch(m_current_clause_buffer.size()) {
            case 1:
                m_unary_clauses.push_back(m_current_clause_buffer.front());
                break;

            case 2:
                p_add_binary(m_current_clause_buffer[0], m_current_clause_buffer[1]);
                break;

            default:
                m_longer_clauses.push_back(m_current_clause_buffer);
                break;
        }
        m_current_clause_buffer.clear();
    }

    void p_add_binary(Lit l1, Lit l2) {
        // avoid resize/reserve to maintain the exponential growth of the vector
        while(m_binary_clauses.size() < m_current_lit) {
            m_binary_clauses.emplace_back();
        }
        m_binary_clauses[l1].push_back(l2);
        m_binary_clauses[l2].push_back(l1);
    }

    Lit m_current_lit = 0;
    std::vector<Lit> m_unary_clauses;
    std::vector<std::vector<Lit>> m_binary_clauses;
    std::vector<std::vector<Lit>> m_longer_clauses;
    std::vector<Lit> m_current_clause_buffer;
};

}

#endif
/// End original header: 'model_builder.h'

/// Original header: #include "propagator.h"
#ifndef SP_PROPAGATOR_H_INCLUDED_
#define SP_PROPAGATOR_H_INCLUDED_


namespace sprop {
namespace detail {

/**
 * @brief The state of a variable in the propagator.
 */
class VariableState {
    std::int32_t value_code{-1};
    std::uint32_t stamp{0};
    std::uint32_t trail_pos{NIL};

  public:
    /**
     * @brief Get the trail position of the variable.
     */
    std::uint32_t get_trail_pos() const noexcept { return trail_pos; }

    /**
     * @brief Get the current stamp value of the variable.
     */
    std::uint32_t get_stamp() const noexcept { return stamp; }

    /**
     * @brief Stamp the variable with a new value.
     */
    void stamp_with(std::uint32_t v) noexcept { stamp = v; }

    /**
     * @brief Assign a value to the variable at a certain level.
     */
    void assign(std::uint32_t tpos, Lit ltrue, std::int32_t level) {
        value_code = (level << 1) + std::int32_t(ltrue & 1);
        trail_pos = tpos;
    }

    /**
     * @brief Get the level at which the variable is assigned.
     */
    std::int32_t level() const noexcept { return value_code >> 1; }

    /**
     * @brief Get the state of the given literal.
     * Essentially, 0, 1 or -1 (unassigned).
     */
    std::int32_t state(Lit l) const noexcept {
        return (value_code >> 31) | (1 & (~std::int32_t(l) ^ value_code));
    }

    /**
     * @brief Set the variable to open (i.e., unassigned).
     */
    void make_open() noexcept { value_code = -1; }

    /**
     * @brief Check if the variable is open.
     */
    bool is_open() const noexcept { return value_code < 0; }

    /**
     * @brief Check if the variable is false.
     */
    bool is_false() const noexcept { return (value_code << 31) & ~value_code; }

    /**
     * @brief Check if the variable is true.
     */
    bool is_true() const noexcept { return !(value_code & 1); }

    /**
     * @brief Check if the given literal is false.
     */
    bool is_false(Lit literal) const noexcept {
        // literal is true if its last bit
        // matches the value code's last bit
        // and its not open
        return ~(value_code >> 31) & 1 & (std::int32_t(literal) ^ value_code);
    }

    /**
     * @brief Check if the given literal is true.
     */
    bool is_true(Lit literal) const noexcept {
        // literal is true if its last bit
        // matches the value code's last bit
        // and its not open
        return ~(value_code >> 31) & 1 & (~std::int32_t(literal) ^ value_code);
    }

    /**
     * @brief Check if the given literal is open or true.
     */
    bool is_open_or_true(Lit literal) const noexcept {
        return ((value_code >> 31) | (~std::int32_t(literal) ^ value_code)) & 1;
    }
};

/**
 * @brief Information about a decision level in the propagator.
 */
class LevelInfo {
    std::uint32_t trail_pos;
    std::uint32_t stamp{0};

  public:
    std::uint32_t get_stamp() const noexcept { return stamp; }

    void stamp_with(std::uint32_t v) noexcept { stamp = v; }

    std::uint32_t level_begin() const noexcept { return trail_pos; }

    explicit LevelInfo(std::uint32_t trail_pos) noexcept
        : trail_pos(trail_pos) {}
};

} // namespace detail

/**
 * @brief A propagator for a SAT formula.
 */
class Propagator {
    using VariableState = detail::VariableState;
    using LevelInfo = detail::LevelInfo;

    /**
     * A watcher for a clause.
     * Each clause watches two literals;
     * these are rearranged so that the watched
     * literals are the first two literals in the clause.
     * The blocker is a watched literal and helps us
     * avoid accesses into the large clause array.
     */
    struct Watcher {
        Lit blocker;
        ClauseRef clause;
    };

    using WatchList = std::vector<Watcher>;
    using WatchIter = WatchList::iterator;
  
  public:
    // -------- CONSTRUCTION --------
    /**
     * Create a new propagator without any clauses or variables.
     */
    inline explicit Propagator();

    /**
     * Create a new propagator from a model/formula.
     */
    inline explicit Propagator(const ModelBuilder& model);

    /**
     * Propagators are copyable/movable.
     * Copies are essentially linear in the size of the entire structure.
     */
    Propagator(const Propagator&) = default;
    Propagator(Propagator&&) = default;
    Propagator& operator=(const Propagator&) = default;
    Propagator& operator=(Propagator&&) = default;

    /// -------- ACCESS OF CLAUSES AND LITERALS --------
    /**
     * @brief For a clause longer than 2, find the ClauseRef based
     * on the range of its literals.
     */
    ClauseRef cref_of(ClausePtrRange lits) const noexcept {
        assert(std::distance(lits.begin(), lits.end()) > 2);
        return ClauseRef(lits.begin() - m_large_clause_db.data());
    }

    /**
     * @brief Get the literals of a clause longer than 2.
     */
    ClausePtrRange lits_of(ClauseRef clause) const noexcept {
        const Lit* begin = m_large_clause_db.data() + clause;
        return std::ranges::subrange(begin, begin + begin[-1]);
    }

    /**
     * @brief Get the length of a clause (in literals).
     */
    ClauseLen clause_length(ClauseRef clause) const noexcept {
        return m_large_clause_db[clause - 1];
    }

    /**
     * @brief From a clause reference, get the next higher clause reference.
     */
    ClauseRef next_clause(ClauseRef clause) const noexcept {
        return clause + m_large_clause_db[clause - 1] + 1;
    }

    /**
     * @brief Get a range of all literals in unary clauses.
     */
    const std::vector<Lit>& unary_clauses() const noexcept {
        return m_unary_clauses;
    }

    /**
     * @brief Get a range of all literals that occur together with the given literal in a binary clause.
     */
    const std::vector<Lit>& binary_partners_of(Lit lit) const noexcept {
        return m_binary_clauses[lit];
    }

    /**
     * @brief Get the ClauseRef of the first clause of length > 2.
     */
    ClauseRef first_longer_clause() const noexcept {
        return 1;
    }

    /**
     * @brief Get the ClauseRef of the clause one past the last clause,
     * i.e., what would be returned by next_clause(last_clause).
     */
    ClauseRef longer_clause_end() const noexcept {
        return m_large_clause_db.size() + 1;
    }

    /**
     * @brief Get a range over all possible literals (2n many).
     */
    auto all_literals() const noexcept {
        return std::views::iota(Lit(0), m_num_vars * 2);
    }

    /**
     * @brief Get the number of variables in the formula.
     */
    Var num_vars() const noexcept { return m_num_vars; }

    // -------- STATE QUERY --------
    /**
     * @brief Get the truth value of the given literal in the current trail.
     * If the literal is open, returns std::nullopt.
     */
    std::optional<bool> value_of(Lit literal) const noexcept {
        Var v = lit::var(literal);
        auto s = variables[v].state(literal);
        if(s == 0) return false;
        if(s == 1) return true;
        return std::nullopt;
    }

    /**
     * @brief Check if the given literal is assigned to true in the current
     * trail.
     *
     * @param literal
     * @return true
     * @return false
     */
    bool is_true(Lit literal) const noexcept {
        Var v = lit::var(literal);
        return variables[v].is_true(literal);
    }

    /**
     * @brief Check if the given literal is assigned to false in the current
     * trail.
     *
     * @param literal
     * @return true
     * @return false
     */
    bool is_false(Lit literal) const noexcept {
        Var v = lit::var(literal);
        return variables[v].is_false(literal);
    }

    /**
     * @brief Check if the given literal is assigned to true in the current
     * trail.
     *
     * @param literal
     * @return true
     * @return false
     */
    bool is_open_or_true(Lit literal) const noexcept {
        Var v = lit::var(literal);
        return variables[v].is_open_or_true(literal);
    }

    /**
     * @brief Check if the given literal is unassigned/open in the current
     * trail.
     *
     * @param literal
     * @return true
     * @return false
     */
    bool is_open(Lit literal) const noexcept {
        Var v = lit::var(literal);
        return variables[v].is_open();
    }

    /**
     * @brief Get the list of literals that are currently assigned to true.
     *
     * @return const std::vector<Lit>&
     */
    const std::vector<Lit>& get_trail() const noexcept { 
        return trail_lits; 
    }

    /**
     * @brief Check whether we currently have a conflict.
     * If this is the case after construction or otherwise at level 0,
     * the formula is unsatisfiable.
     *
     * @return true
     * @return false
     */
    bool is_conflicting() const noexcept {
        return conflicting; 
    }

    // -------- ADVANCED STATE QUERY --------
    /**
     * @brief Check if the given non-open literal was assigned as a decision.
     *
     * @param literal
     * @return true
     * @return false
     */
    bool is_decision(Lit literal) const noexcept {
        assert(!is_open(literal));
        auto tpos = variables[lit::var(literal)].get_trail_pos();
        return trail_reasons[tpos].reason_length == 0;
    }

    /**
     * @brief Get the decision level of the literal if it is in the trail.
     * @return The decision level, or a negative value if the literal is open.
     */
    std::int32_t get_decision_level(Lit literal) const noexcept {
        return variables[lit::var(literal)].level();
    }

    /**
     * @brief Get the reason for the literal if it is in the trail;
     *        otherwise, causes undefined behavior.
     */
    Reason get_reason(Lit literal) const noexcept {
        auto tpos = variables[lit::var(literal)].get_trail_pos();
        return trail_reasons[tpos];
    }

    /**
     * @brief Get the list of reasons for the literals that are currently assigned to true.
     */
    const std::vector<Reason>&  get_reasons() const noexcept {
        return trail_reasons; 
    }

    /**
     * @brief Get a list of all decision literals on the trail.
     *        Creates a new vector (unlike get_trail, which returns
     *        a reference to an internal structure).
     *
     * @return std::vector<Lit>
     */
    inline std::vector<Lit> get_decisions() const;

    /**
     * @brief Get the current decision level.
     */
    std::int32_t get_current_level() const noexcept {
        return std::int32_t(levels.size() - 1);
    }

    /**
     * @brief Get an iterator to the beginning of the current level in the trail.
     */
    std::vector<Lit>::const_iterator current_level_begin() const noexcept {
        return trail_lits.begin() + levels.back().level_begin();
    }

    /**
     * @brief Get an iterator to the beginning of the current level in the trail reasons.
     */
    std::vector<Reason>::const_iterator current_level_reasons_begin() const noexcept {
        return trail_reasons.begin() + levels.back().level_begin();
    }

    /**
     * @brief Get an iterator to the beginning of the given level in the trail.
     *
     * @param level
     * @return std::vector<Lit>::const_iterator
     */
    std::vector<Lit>::const_iterator level_begin(std::uint32_t level) const {
        return trail_lits.begin() + levels[level].level_begin();
    }

    /**
     * @brief Get an iterator to the end of the given level in the trail.
     *
     * @param level
     * @return std::vector<Lit>::const_iterator
     */
    std::vector<Lit>::const_iterator level_end(std::uint32_t level) const {
        if (level >= levels.size() - 1)
            return trail_lits.end();
        return trail_lits.begin() + levels[level + 1].level_begin();
    }

    /**
     * Get the index in the trail of the given literal.
     * Undefined behaviour if lit is open.
     */
    std::size_t get_trail_index(Lit lit) const noexcept {
        return variables[lit::var(lit)].get_trail_pos();
    }

    // -------- CONFLICT INFORMATION --------
    /**
     * @brief Get the conflict literal and reason.
     * @return std::pair<Lit, Reason>
     */
    std::pair<Lit, Reason> get_conflict() const noexcept {
        return {conflict_lit, conflict_reason};
    }

    /**
     * Compute a list of [Level, Literal] pairs of decisions
     * that ultimately led to including l in the trail.
     */
    inline const std::vector<std::pair<std::int32_t,Lit>> &decisions_leading_to(Lit l);

    /*
     * Compute a list of [Level, Literal] pairs of decisions
     * that ultimately led to the current conflict.
     */
    inline const std::vector<std::pair<std::int32_t,Lit>>& decisions_leading_to_conflict();

    // -------- PROPAGATION --------
    /**
     * @brief Trigger propagation; it should not be necessary to call this
     * manually.
     *
     * @return false iff a conflict was found.
     */
    inline bool propagate();

    // -------- MAKING AND UNDOING DECISIONS --------
    /**
     * @brief Push a decision literal and create a new decision level.
     * Automatically propagates the decision and all consequences.
     * Throws an error if the decision literal is already assigned (true or
     * false).
     *
     * @param decision
     * @return true If the decision did not result in a conflict.
     * @return false If the decision resulted in a conflict.
     */
    inline bool push_level(Lit decision);

    /**
     * @brief Pop the highest decision level without learning.
     * There is no need for a conflict to use this method.
     * On conflict, it should be preferred to call resolve_conflicts instead,
     * but pop_level will also clear the conflict.
     */
    inline void pop_level();

    /**
     * Reset propagator to level 0.
     */
    void reset_to_zero() noexcept {
        while(get_current_level() > 0) {
            pop_level();
        }
    }

    /**
     * @brief Resolve a conflict by learning a clause and jumping back
     * to the appropriate decision level (at least one level down).
     * At least one assignment is forced on the target decision level
     * from the conflict clause, which is added to the formula.
     * Undone and forced new assignments are reported to the given AssignmentHandler,
     * which should provide two methods (see resolve_conflicts() for an example).
     *
     * In any case, all assignments on the current level are undone because
     * its decision led to a conflict; this is *NOT* reported to the given
     * AssignmentHandler.
     *
     * However, all assignments on lower levels that are undone or forced by
     * this action *ARE* reported. After learning a conflict clause and jumping
     * back, we continue propagation. It is possible that another conflict
     * occurs. This conflict is also handled recursively (and now all undone
     * assignments are reported). This is repeated until we reach a state
     * without conflicts (we return true), or we reach a conflict on level 0 (we
     * return false and the formula is UNSAT).
     *
     * @tparam AssignmentHandler A type that implements methods
     * assignment_undone(Lit) and assignment_forced(Lit).
     * @param assignments The AssignmentHandler that is notified of changes.
     * @return true if at some level, we ended in a non-conflicting state.
     * @return false if we encountered a conflict at level 0, indicating
     * infeasibility.
     */
    template <typename AssignmentHandler>
    bool resolve_conflicts(AssignmentHandler& assignments);

    /**
     * @brief Resolve conflicts without handler.
     *
     * @return true we reached a non-conflicting state.
     * @return false the formula is UNSAT (conflict at level 0).
     */
    inline bool resolve_conflicts();

    /**
     * @brief Resolve conflicts without handler.
     * @throws UNSATException if the conflict cannot be resolved (the formula is UNSAT).
     */
    void resolve_or_throw() {
        if(!resolve_conflicts()) throw UNSATException();
    }

    // -------- RESULT EXTRACTION --------
    /**
     * @brief Extract an assignment as bit-vector, where result[i] == true means
     *        that variable i (internal 0-based indexing) is set to true.
     */
    inline std::vector<bool> extract_assignment() const;

  private:
    // -------- FORMULA DATA --------
    std::vector<Lit> m_unary_clauses;
    std::vector<std::vector<Lit>> m_binary_clauses;
    std::vector<Lit> m_large_clause_db;
    Var m_num_vars;

    // -------- VARIABLE/LITERAL STATE --------
    // The state of our variables.
    std::vector<VariableState> variables;
    // For each literal, a list of watchers.
    std::vector<WatchList> watchers;

    // -------- TRAIL --------
    // The literals on the trail.
    std::vector<Lit> trail_lits;
    // The reasons on the trail.
    std::vector<Reason> trail_reasons;
    // The levels of the trail.
    std::vector<LevelInfo> levels;
    // The index of the next literal to propagate on.
    std::size_t trail_queue_head{0};

    // -------- CONFLICT INFORMATION --------
    // The reason for the current conflict.
    Reason conflict_reason{Reason::Decision{}};
    // The conflict literal of the current conflict 
    // (its negation is in the trail).
    Lit conflict_lit{NIL};
    // A stamp counter for temporarily marking variables 
    // during (conflict resolution/redundancy information).
    std::uint32_t stamp_counter{0};
    // Whether we have a current conflict.
    bool conflicting{false};

    // -------- AUXILIARY BUFFERS --------
    // Buffer for building a learnt clause
    std::vector<Lit> learn_buffer;
    // Buffer for supporting decisions of a literal.
    std::vector<std::pair<std::int32_t,Lit>> supporting_decision_buffer;


    // ----------------- IMPLEMENTATION BEYOND THIS POINT -----------------
    /**
     * @brief Get the literals of a clause longer than 2.
     */
    MutClausePtrRange mut_lits_of(ClauseRef clause) noexcept {
        Lit* begin = m_large_clause_db.data() + clause;
        return std::ranges::subrange(begin, begin + begin[-1]);
    }

    /**
     * Assign the given literal to true at decision level 0.
     * Return false if this leads to a conflict.
     */
    bool p_assign_at_0(Lit forced_true) {
        VariableState& vstate = variables[lit::var(forced_true)];
        if (vstate.is_open()) {
            vstate.assign(trail_lits.size(), forced_true, 0);
            trail_lits.push_back(forced_true);
            trail_reasons.push_back(Reason::Unary{forced_true});
        } else {
            if (vstate.is_false(forced_true)) {
                conflicting = true;
                return false;
            }
        }
        return true;
    }

    /**
     * Assign the given literal to true at the given level,
     * using the given reason.
     */
    template <typename ReasonType>
    void p_assign_at(VariableState& vstate, std::int32_t level, 
                     Lit literal, ReasonType&& reason) 
    {
        vstate.assign(trail_lits.size(), literal, level);
        trail_lits.push_back(literal);
        trail_reasons.emplace_back(std::forward<ReasonType>(reason));
    }

    /**
     * Initialize level 0 with the unary clauses.
     */
    void p_init_unaries() {
        for(Lit forced_true : m_unary_clauses) {
            if(!p_assign_at_0(forced_true)) {
                conflicting = true;
                return;
            }
        }
    }

    /**
     * Initialize the watches for binary clauses (handle differently compared to
     * larger clauses).
     */
    void p_init_binary_watches() {
        for(Lit l : all_literals()) {
            VariableState& v1 = variables[lit::var(l)];
            bool is_false1 = v1.is_false(l);
            if(is_false1) {
                for(Lit partner : binary_partners_of(l)) {
                    m_unary_clauses.push_back(partner);
                    if(!p_assign_at_0(partner)) return;
                }
            }
        }
    }

    /**
     * Insert a new long clause into the watch lists,
     * unless it is satisfied, forcing or conflicting at level 0.
     */
    void p_new_long_clause_on_construction(ClauseRef ref, MutClausePtrRange literals) {
        Lit *new_first[2];
        std::int32_t nws = 0;
        Lit* lit_array = literals.begin();
        for(Lit* current = lit_array, *end = literals.end(); current != end; ++current) {
            Lit l = *current;
            VariableState& vstate = variables[lit::var(l)];
            auto s = vstate.state(l);
            if(s == -1) {
                if(nws < 2) {
                    new_first[nws++] = current;
                }
            } else if(s == 1) {
                nws = -1;
                break;
            }
        }
        if(nws == -1) {
            // satisfied at level 0 - ignored, not watched
            return;
        }
        if(nws == 0) { 
            // violated at level 0 - conflict, UNSAT
            conflicting = true;
            conflict_reason = Reason::Clause{ClauseLen(literals.size()), ref};
            return;
        }
        if(nws == 1) {
            // forcing at level 0 - add unary, do not watch
            Lit forced_true = *new_first[0];
            m_unary_clauses.push_back(forced_true);
            p_assign_at_0(forced_true);
            return;
        }
        // move the watched literals to the front
        std::swap(*new_first[0], lit_array[0]);
        std::swap(*new_first[1], lit_array[1]);
        // install watchers
        Lit w1 = lit_array[0], w2 = lit_array[1];
        watchers[w1].push_back(Watcher{w2, ref});
        watchers[w2].push_back(Watcher{w1, ref});
    }

    /**
     * Initialize the watches for everything.
     */
    void p_init_watches() {
        p_init_unaries();
        if(conflicting) return;
        watchers.resize(2 * m_num_vars);
        for(ClauseRef ref = first_longer_clause(); ref < longer_clause_end(); ref = next_clause(ref)) {
            MutClausePtrRange literals = mut_lits_of(ref);
            p_new_long_clause_on_construction(ref, literals);
            if(conflicting) return;
        }
        p_init_binary_watches();
    }

    /**
     * Process the short clauses.
     */
    void p_process_short_clauses() {
        for(auto& list : m_binary_clauses) {
            std::sort(list.begin(), list.end());
            list.erase(std::unique(list.begin(), list.end()), list.end());
        }
        m_binary_clauses.resize(m_num_vars * 2);
    }

    /**
     * Import the longer clauses.
     */
    void p_import_large_clauses(const std::vector<std::vector<Lit>>& clauses) {
        std::size_t total_size = 0;
        for(const auto& clause : clauses) {
            total_size += clause.size() + 1;
        }
        m_large_clause_db.reserve(std::size_t(std::round(total_size * 1.5)));
        for(const auto& clause : clauses) {
            m_large_clause_db.push_back(ClauseLen(clause.size()));
            m_large_clause_db.insert(m_large_clause_db.end(), clause.begin(), clause.end());
        }
    }

    /**
     * Roll back the current decision level; if report is set,
     * report undone assignments to the given handler.
     */
    template <typename AssignmentHandler>
    void p_rollback_level(AssignmentHandler& handler, bool report) {
        if (levels.back().level_begin() == 0) {
            for (auto i = trail_lits.rbegin(), e = trail_lits.rend(); i != e; ++i) {
                Lit l = *i;
                if (report)
                    handler.assignment_undone(l);
                variables[lit::var(l)].make_open();
            }
            trail_lits.clear();
            trail_reasons.clear();
        } else {
            auto current_end =
                trail_lits.begin() + (levels.back().level_begin() - 1);
            auto current_begin = trail_lits.end() - 1;
            for (; current_begin != current_end; --current_begin) {
                trail_reasons.pop_back();
                Lit l = *current_begin;
                if (report)
                    handler.assignment_undone(l);
                variables[lit::var(l)].make_open();
            }
            trail_lits.erase(current_end + 1, trail_lits.end());
        }
        levels.pop_back();
    }

    /**
     * Reset the conflict information to 'no conflict'.
     */
    void p_reset_conflict() noexcept {
        conflicting = false;
        conflict_lit = NIL;
        conflict_reason = Reason::Decision{};
    }

    /**
     * Propagate using a binary clause.
     * Return false on conflict.
     */
    bool p_propagate_binary(Lit lfalse, Lit other, std::int32_t level) {
        Lit v = lit::var(other);
        VariableState& vs = variables[v];
        if (vs.is_open()) {
            p_assign_at(vs, level, other, Reason::Binary{lfalse, other});
        } else {
            if (vs.is_false(other)) {
                conflicting = true;
                conflict_reason = Reason::Binary{lfalse, other};
                conflict_lit = other;
                return false;
            }
        }
        return true;
    }

    /**
     * Propagate a new decision or consequence through binary clauses.
     */
    bool p_propagate_through_binaries(Lit ltrue) {
        Lit lfalse = lit::negate(ltrue);
        auto level = std::int32_t(levels.size() - 1);
        for (Lit other : binary_partners_of(lfalse)) {
            if(!p_propagate_binary(lfalse, other, level))
                return false;
        }
        return true;
    }

    /**
     * Check if *watcher_in has a true blocker.
     * In that case, copy it to *watcher_out and advance both.
     * Otherwise, simply return false.
     */
    bool p_has_true_blocker(WatchIter& watcher_in, WatchIter& watcher_out) {
        if (is_true(watcher_in->blocker)) {
            *watcher_out++ = *watcher_in++;
            return true;
        }
        return false;
    }

    /**
     * Propagate a new decision or consequence through longer clauses.
     * Return false on conflict.
     * When we call this, lfalse (negation of ltrue) is made false.
     * We need to check all clauses that watch lfalse; in such clauses,
     * we have to make sure that they are either satisfied or that
     * we can find another open literal to watch to replace the now false lfalse.
     * To check for satisfiedness, we first check the blocker, so in the
     * ideal case we want long-living true literals as blockers to
     * avoid having to go to the clause literal array.
     */
    bool p_propagate_through_longer(Lit ltrue) {
        Lit lfalse = lit::negate(ltrue);
        auto level = std::int32_t(levels.size() - 1);
        WatchList& ws = watchers[lfalse];
        WatchIter watcher_in = ws.begin();
        WatchIter watcher_out = ws.begin(); 
        WatchIter watcher_end = ws.end();

        // the main loop is essentially a big std::remove_if cleaning
        // up the watchers list 
        // but needs more guarantees than that std::algorithm provides
        while(watcher_in != watcher_end) {
            if(p_has_true_blocker(watcher_in, watcher_out)) continue;
            ClauseRef clause = watcher_in->clause;
            MutClausePtrRange lits = mut_lits_of(clause);
            Lit* lit_array = lits.begin();
            Lit* lit_end = lits.end();
            if(lit_array[0] == lfalse) {
                // make it so lfalse is in lit_array[1]
                std::swap(lit_array[0], lit_array[1]);
            }
            // check the other watched literal (if it is not the blocker) as new blocker;
            // also unconditionally advance watcher_in
            Lit first = lit_array[0];
            Watcher new_watcher{first, clause};
            VariableState& first_state = variables[lit::var(first)];
            if(first != watcher_in++->blocker && first_state.is_true(first)) {
                *watcher_out++ = new_watcher;
                continue;
            }
            // search the rest of the clause for an open or true literal
            Lit* replacement = std::find_if(lit_array + 2, lit_end, [&] (Lit l) { return is_open_or_true(l); });
            if(replacement != lit_end) {
                // found replacement; move it to lit_array[1] and install new watcher;
                // do not advance watcher_out
                Lit repl = *replacement;
                lit_array[1] = repl;
                *replacement = lfalse;
                watchers[repl].push_back(new_watcher);
            } else {
                // clause is unit
                *watcher_out++ = new_watcher;
                Reason::Clause reason{ClauseLen(lits.size()), clause};
                if(first_state.is_false(first)) {
                    // conflict
                    conflicting = true;
                    conflict_lit = first;
                    conflict_reason = reason;
                    watcher_out = std::copy(watcher_in, watcher_end, watcher_out);
                    break;
                } else {
                    p_assign_at(first_state, level, first, reason);
                }
            }
        }
        ws.erase(watcher_out, watcher_end);
        return !conflicting;
    }

    bool p_propagate(Lit ltrue) {
        if (!p_propagate_through_binaries(ltrue))
            return false;
        return p_propagate_through_longer(ltrue);
    }

    std::uint32_t p_increase_stamp() noexcept {
        if (stamp_counter >= std::numeric_limits<std::uint32_t>::max() - 6) {
            for (VariableState& vs : variables) {
                vs.stamp_with(0);
            }
            for (LevelInfo& lvl : levels) {
                lvl.stamp_with(0);
            }
            stamp_counter = 0;
        }
        stamp_counter += 3;
        return stamp_counter;
    }

    void p_bfs_reasons(std::uint32_t current_stamp) {
        std::size_t lbpos = 0;
        while(lbpos < learn_buffer.size()) {
            Lit next = learn_buffer[lbpos++];
            std::size_t tindex = get_trail_index(next);
            if(trail_reasons[tindex].reason_length == 0) {
                supporting_decision_buffer.emplace_back(get_decision_level(next), next);
            } else {
                Reason reason = trail_reasons[tindex];
                for(Lit lr : reason.lits(*this)) {
                    if(lr != next) {
                        Var v = lit::var(lr);
                        if(variables[v].get_stamp() != current_stamp) {
                            variables[v].stamp_with(current_stamp);
                            learn_buffer.push_back(lit::negate(lr));
                        }
                    }
                }
            }
        }
    }

    void p_stamp_level(std::int32_t level) {
        LevelInfo& li = levels[level];
        if (li.get_stamp() < stamp_counter) {
            li.stamp_with(stamp_counter);
        } else {
            li.stamp_with(stamp_counter + 1);
        }
    }

    std::uint32_t p_stamp_and_count(std::int32_t level, ClausePtrRange literals) {
        std::uint32_t count = 0;
        for (Lit l : literals) {
            Lit v = lit::var(l);
            VariableState& vs = variables[v];
            std::int32_t vlvl = vs.level();
            if (vlvl >= level) {
                if (vs.get_stamp() >= stamp_counter)
                    continue;
                ++count;
                vs.stamp_with(stamp_counter);
            } else {
                if (vlvl <= 0)
                    continue;
                std::uint32_t vstamp = vs.get_stamp();
                if (vstamp < stamp_counter) {
                    p_stamp_level(vlvl);
                    learn_buffer.push_back(l);
                    vs.stamp_with(stamp_counter);
                }
            }
        }
        return count;
    }

    std::uint32_t p_stamp_and_count(std::int32_t level, Reason reason) {
        return p_stamp_and_count(level, reason.lits(*this));
    }

    /**
     * Check if the given literal is redundant in the current conflict clause.
     */
    bool p_is_redundant(Lit v) {
        auto& vs = variables[v];
        auto s = vs.get_stamp();
        if (s == stamp_counter + 1)
            return true;
        if (s == stamp_counter + 2)
            return false;
        auto tloc = variables[v].get_trail_pos();
        const Reason& r = trail_reasons[tloc];
        if (r.reason_length == 0) {
            vs.stamp_with(stamp_counter + 2);
            return false;
        }
        auto reason_lits = r.lits(*this);
        for (Lit rl : reason_lits) {
            Lit rv = lit::var(rl);
            if (rv == v)
                continue;
            auto rlvl = variables[rv].level();
            if (rlvl == 0)
                continue;
            auto rvs = variables[rv];
            auto rs = rvs.get_stamp();
            if (rs == stamp_counter + 2)
                return false;
            if (rs < stamp_counter) {
                if (levels[rlvl].get_stamp() < stamp_counter ||
                    !p_is_redundant(rv))
                {
                    rvs.stamp_with(stamp_counter + 2);
                    return false;
                }
            }
        }
        vs.stamp_with(stamp_counter + 1);
        return true;
    }

    /**
     * Strengthen the learnt clause by removing redundant literals.
     */
    void p_filter_redundancies() {
        // move the conflict-level literal to the front
        std::swap(learn_buffer.back(), learn_buffer.front());
        auto filter = [&] (Lit l) -> bool {
            Lit v = lit::var(l);
            auto vlvl = variables[v].level();
            if (vlvl == 0)
                return true;
            if (levels[vlvl].get_stamp() != stamp_counter + 1)
                return false;
            return p_is_redundant(v);
        };
        learn_buffer.erase(std::remove_if(learn_buffer.begin() + 1,
                                          learn_buffer.end(), filter),
                           learn_buffer.end());
    }

    /**
     * Compute and reduce a conflict clause;
     * it will be stored in learn_buffer.
     */
    void p_compute_conflict_clause() {
        p_increase_stamp();
        std::int32_t level(levels.size() - 1);
        std::uint32_t on_current_level =
            p_stamp_and_count(level, conflict_reason);
        auto trail_lit_iter = trail_lits.end() - 1;
        auto trail_reason_iter = trail_reasons.end() - 1;
        while (on_current_level > 1) {
            Lit l = *trail_lit_iter;
            Lit v = lit::var(l);
            if (variables[v].get_stamp() >= stamp_counter) {
                on_current_level +=
                    p_stamp_and_count(level, *trail_reason_iter);
                --on_current_level;
            }
            --trail_lit_iter;
            --trail_reason_iter;
        }
        for (;;) {
            Lit l = *trail_lit_iter;
            Lit v = lit::var(l);
            if (variables[v].get_stamp() >= stamp_counter)
                break;
            --trail_lit_iter;
            --trail_reason_iter;
        }
        learn_buffer.push_back(lit::negate(*trail_lit_iter));
        p_filter_redundancies();
    }

    /**
     * Compute the target level after backjumping.
     * Assumes that the conflict literal is the first literal in learn_buffer.
     */
    std::pair<std::int32_t, Lit> p_target_level() {
        std::int32_t target_level = 0;
        Lit target_lit = learn_buffer.front();
        for (auto i = learn_buffer.begin() + 1, e = learn_buffer.end(); i != e; ++i) {
            Lit l = *i;
            Lit v = lit::var(l);
            auto lvl = variables[v].level();
            if (lvl > target_level) {
                target_level = lvl;
                target_lit = l;
            }
        }
        return {target_level, target_lit};
    }

    /**
     * Handle jumping back to the target level.
     */
    template <typename AssignmentHandler>
    std::pair<std::int32_t, Lit> p_jumpback_to_target(AssignmentHandler& handler) 
    {
        auto [tlvl, tlit] = p_target_level();
        p_rollback_level(handler, false);
        while (levels.size() > std::size_t(tlvl + 1)) {
            p_rollback_level(handler, true);
        }
        trail_queue_head = trail_lits.size();
        return {tlvl, tlit};
    }

    ClauseRef p_insert_conflict_clause() {
        switch(learn_buffer.size()) {
            case 1: {
                m_unary_clauses.push_back(learn_buffer.front());
                return NIL;
            }
            case 2: {
                Lit l1 = learn_buffer[0], l2 = learn_buffer[1];
                m_binary_clauses[l1].push_back(l2);
                m_binary_clauses[l2].push_back(l1);
                return NIL;
            }
            default: {
                ClauseRef ref(m_large_clause_db.size() + 1);
                m_large_clause_db.push_back(ClauseLen(learn_buffer.size()));
                m_large_clause_db.insert(m_large_clause_db.end(), learn_buffer.begin(), learn_buffer.end());
                return ref;
            }
        }
    }

    void p_new_watch(Lit learnt, Lit target_lit, ClauseRef clause) {
        assert(clause != NIL);
        MutClausePtrRange lits = mut_lits_of(clause);
        Lit* lit_array = lits.begin();
        assert(lit_array[0] == learnt);
        auto other = std::find(lits.begin() + 1, lits.end(), target_lit);
        assert(other != lits.end());
        std::swap(lit_array[1], *other);
        watchers[learnt].push_back(Watcher{target_lit, clause});
        watchers[target_lit].push_back(Watcher{learnt, clause});
    }

    template <typename AssignmentHandler>
    void p_handle_conflict_clause(AssignmentHandler& handler) {
        ClauseRef cref_if_long = p_insert_conflict_clause();
        auto [tlvl, tlit] = p_jumpback_to_target(handler);
        Lit learned = learn_buffer.front();
        Lit lv = lit::var(learned);
        std::uint32_t len = learn_buffer.size();
        switch (len) {
        case 1:
            p_assign_at(variables[lv], tlvl, learned, Reason::Unary{learned});
            break;
        case 2:
            p_assign_at(variables[lv], tlvl, learned, Reason::Binary{learned, learn_buffer[1]});
            break;
        default: 
            p_assign_at(variables[lv], tlvl, learned, Reason::Clause{len, cref_if_long});
            p_new_watch(learned, tlit, cref_if_long);
            break;
        }
        learn_buffer.clear();
    }
};

Propagator::Propagator() :
    m_num_vars(0),
    levels{{LevelInfo{0}}}
{}

Propagator::Propagator(const ModelBuilder& model) :
    m_unary_clauses(model.m_unary_clauses),
    m_binary_clauses(model.m_binary_clauses),
    m_num_vars(model.m_current_lit / 2),
    variables(m_num_vars),
    levels{{LevelInfo{0}}}
{
    p_process_short_clauses();
    p_import_large_clauses(model.m_longer_clauses);
    p_init_watches();
    if(!conflicting) {
        propagate();
    }
}

bool Propagator::propagate() {
    if (conflicting)
        return false;
    while (trail_queue_head < trail_lits.size()) {
        Lit prop = trail_lits[trail_queue_head++];
        if (!p_propagate(prop))
            return false;
    }
    return true;
}

std::vector<Lit> Propagator::get_decisions() const {
    std::vector<Lit> result;
    result.reserve(levels.size() - 1);
    for (auto ilvl = levels.begin() + 1, iend = levels.end(); ilvl != iend;
            ++ilvl)
    {
        result.push_back(trail_lits[ilvl->level_begin()]);
    }
    return result;
}

bool Propagator::resolve_conflicts() {
    struct TrivialAssignmentHandler {
        void assignment_undone(Lit) const noexcept {}
        void assignment_forced(Lit) const noexcept {}
    };
    TrivialAssignmentHandler handler;
    return resolve_conflicts(handler);
}

bool Propagator::push_level(Lit decision) {
    Lit dvar = lit::var(decision);
    VariableState& vstate = variables[dvar];
    if(conflicting) {
        throw std::invalid_argument(
            "The propagator is already conflicting!");
    }
    if (!vstate.is_open()) {
        throw std::invalid_argument(
            "The given decision literal was already assigned!");
    }
    std::uint32_t tpos = trail_lits.size();
    std::int32_t new_level = levels.size();
    levels.emplace_back(tpos);
    p_assign_at(vstate, new_level, decision, Reason::Decision{});
    return propagate();
}

void Propagator::pop_level() {
    if (levels.size() == 1) {
        throw std::invalid_argument(
            "Trying to pop level from propagator at level 0!");
    }
    struct TrivialHandler {
        void assignment_undone(Lit) {}
    } handler;
    p_rollback_level(handler, false);
    trail_queue_head = trail_lits.size();
    if (conflicting)
        p_reset_conflict();
}

const std::vector<std::pair<std::int32_t,Lit>> &Propagator::decisions_leading_to(Lit l) {
    if(conflicting) throw std::logic_error("decisions_leading_to called on propagator with conflict!");
    if(is_open(l)) throw std::logic_error("decisions_leading_to called with open literal!");
    supporting_decision_buffer.clear();
    std::size_t tindex = get_trail_index(l);
    if(trail_reasons[tindex].reason_length == 0) {
        supporting_decision_buffer.emplace_back(get_decision_level(l), l);
        return supporting_decision_buffer;
    }
    auto current = p_increase_stamp();
    Reason reason = trail_reasons[tindex];
    for(Lit lr : reason.lits(*this)) {
        if(lr != l) {
            variables[lit::var(lr)].stamp_with(current);
            learn_buffer.push_back(lit::negate(lr));
        }
    }
    p_bfs_reasons(current);
    learn_buffer.clear();
    return supporting_decision_buffer;
}

const std::vector<std::pair<std::int32_t,Lit>>& Propagator::decisions_leading_to_conflict() {
    if(!conflicting) 
        throw std::logic_error("decisions_leading_to_conflict called on non-conflicting propagator!");

    supporting_decision_buffer.clear();
    auto current = p_increase_stamp();
    for(Lit lr : conflict_reason.lits(*this)) {
        if(lr != conflict_lit) {
            variables[lit::var(lr)].stamp_with(current);
            learn_buffer.push_back(lit::negate(lr));
        }
    }
    variables[lit::var(conflict_lit)].stamp_with(current);
    Lit lc = lit::negate(conflict_lit);
    for(Lit lr : get_reason(lc).lits(*this))  {
        if(variables[lit::var(lr)].get_stamp() != current) {
            variables[lit::var(lr)].stamp_with(current);
            learn_buffer.push_back(lit::negate(lr));
        }
    }
    p_bfs_reasons(current);
    learn_buffer.clear();
    return supporting_decision_buffer;
}

template <typename AssignmentHandler>
bool Propagator::resolve_conflicts(AssignmentHandler& assignments) {
    if (!conflicting)
        return true;
    if (levels.size() == 1)
        return false;
    p_compute_conflict_clause();
    p_handle_conflict_clause(assignments);
    p_reset_conflict();
    std::size_t tsize = trail_queue_head;
    std::size_t lbegin = levels.back().level_begin();
    if (!propagate()) {
        for (std::size_t cpos = tsize - 1; cpos != lbegin - 1; --cpos) {
            assignments.assignment_undone(trail_lits[cpos]);
        }
        return resolve_conflicts(assignments);
    } else {
        for (auto i = trail_lits.begin() + tsize, e = trail_lits.end(); i != e; ++i) {
            assignments.assignment_forced(*i);
        }
        return true;
    }
}

std::vector<bool> Propagator::extract_assignment() const {
    const Var nv = m_num_vars;
    if(get_trail().size() != nv) {
        throw std::logic_error("Trail incomplete in extract_assignment!");
    }
    std::vector<bool> result(nv, false);
    for(Lit l : get_trail()) {
        if(!lit::negative(l)) {
            result[lit::var(l)] = true;
        }
    }
    return result;
}

}

#endif
/// End original header: 'propagator.h'

/// Original header: #include "extract_reduced_partial.h"
#ifndef SP_EXTRACT_REDUCED_PARTIAL_H_INCLUDED_
#define SP_EXTRACT_REDUCED_PARTIAL_H_INCLUDED_


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
/// End original header: 'extract_reduced_partial.h'

#endif // STANDALONE_PROPAGATOR_STANDALONE_PROPAGATOR_H_INCLUDED_
