#ifndef SP_MODEL_BUILDER_H_INCLUDED_
#define SP_MODEL_BUILDER_H_INCLUDED_

#include "types.h"
#include "unsat_exception.h"
#include "literal_ops.h"
#include <algorithm>
#include <string>
#include <format>
#include <sstream>


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
