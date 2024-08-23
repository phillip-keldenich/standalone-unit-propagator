#ifndef STANDALONE_PROPAGATOR_SINGLE_HEADER
#include <standalone-propagator/propagator.h>
#include <standalone-propagator/eliminate_subsumed.h>
#include <standalone-propagator/extract_reduced_partial.h>
#else
#include <standalone-propagator/standalone-propagator.h>
#endif

// no standard include headers before this point!
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <ranges>
#include <vector>
#include <cstddef>
#include <random>
#include <set>


TEST_CASE("[empty] Ensure C++20 works as far as we need it") {
    std::size_t n = 5;
    std::vector<std::size_t> result;
    std::vector<std::size_t> expected{0, 1, 2, 3, 4};
    std::ranges::copy(std::views::iota(std::size_t(0), n), std::back_inserter(result));
    CHECK(result == expected);
}


std::pair<std::vector<sprop::Lit>, sprop::ModelBuilder>  waerden33(int n) {
    auto lnot = sprop::lit::negate;
    sprop::ModelBuilder builder;
    std::vector<sprop::Lit> variables(1, sprop::NIL);
    for(int i = 1; i <= n; ++i) {
        variables.push_back(builder.add_variable());
    }
    for(int distance = 1; 2 * distance <= n - 1; ++distance) {
        for(int i = 1; i + 2 * distance <= n; ++i) {
            builder.add_clause(variables[i], variables[i + distance], variables[i + 2 * distance]);
            builder.add_clause(lnot(variables[i]), lnot(variables[i + distance]), lnot(variables[i + 2 * distance]));
        }
    }
    return {std::move(variables), std::move(builder)};
}


TEST_CASE("[Propagator] Basic CDCL SAT solver test case: waerden(3, 3; 8)") {
    using namespace sprop;
    auto lnot = lit::negate;
    auto [vars, model] = waerden33(8);
    Propagator propagator(model);
    REQUIRE(propagator.get_trail().empty());
    REQUIRE(propagator.get_current_level() == 0);
    REQUIRE(propagator.push_level(vars[1]));
    REQUIRE(propagator.get_current_level() == 1);
    REQUIRE(propagator.get_trail().size() == 1);
    REQUIRE(propagator.get_trail()[0] == vars[1]);
    REQUIRE(propagator.is_decision(vars[1]));
    REQUIRE(propagator.push_level(vars[2]));
    REQUIRE(propagator.get_trail().size() == 3);
    REQUIRE(propagator.get_trail()[0] == vars[1]);
    REQUIRE(propagator.get_trail()[1] == vars[2]);
    REQUIRE(propagator.get_trail()[2] == lnot(vars[3]));
    REQUIRE(propagator.is_decision(vars[2]));
    REQUIRE(propagator.is_decision(vars[1]));
    REQUIRE(!propagator.is_decision(lnot(vars[3])));
    REQUIRE(propagator.get_current_level() == 2);
    REQUIRE(!propagator.push_level(vars[4]));
    REQUIRE(propagator.is_conflicting());
    REQUIRE(propagator.get_current_level() == 3);
    REQUIRE(propagator.resolve_conflicts());
    REQUIRE(propagator.get_current_level() == 2);
    REQUIRE(!propagator.is_conflicting());
    REQUIRE(propagator.get_trail().size() == 8); // found solution!
    auto assignment = propagator.extract_assignment();
    CHECK(assignment == (std::vector<bool>{1, 1, 0, 0, 1, 1, 0, 0}));
    REQUIRE(!model.verify_trail(propagator.get_trail()));
    REQUIRE(!model.verify_assignment(assignment));
}


TEST_CASE("[Propagator] Basic CDCL SAT solver test case: waerden(3, 3; 9)") {
    using namespace sprop;
    auto lnot = lit::negate;
    auto [vars, model] = waerden33(9);
    model.add_clause(lnot(vars[1])); // symmetry breaking
    Propagator propagator(model);
    REQUIRE(propagator.get_trail().size() == 1);
    REQUIRE(propagator.get_current_level() == 0);
    REQUIRE(propagator.get_trail()[0] == lnot(vars[1]));
    REQUIRE(propagator.push_level(lnot(vars[2])));
    REQUIRE(propagator.get_trail() == std::vector<Lit>{lnot(vars[1]), lnot(vars[2]), vars[3]});
    REQUIRE(!propagator.push_level(lnot(vars[4])));
    REQUIRE(propagator.is_conflicting());
    REQUIRE(propagator.get_current_level() == 2);
    REQUIRE(propagator.resolve_conflicts());
    REQUIRE(propagator.get_current_level() == 0);
    REQUIRE(!propagator.is_conflicting());
    REQUIRE(propagator.get_trail().size() == 2);
    REQUIRE(propagator.get_trail()[0] == lnot(vars[1]));
    REQUIRE(propagator.get_trail()[1] == vars[2]);
    REQUIRE(!propagator.push_level(lnot(vars[7])));
    REQUIRE(propagator.is_conflicting());
    REQUIRE(propagator.get_current_level() == 1);
    REQUIRE(propagator.resolve_conflicts());
    REQUIRE(propagator.get_current_level() == 0);
    REQUIRE(!propagator.is_conflicting());
    REQUIRE(propagator.get_trail() == std::vector<Lit>{lnot(vars[1]), vars[2], vars[7]});
    REQUIRE(!propagator.push_level(vars[6]));
    REQUIRE(propagator.is_conflicting());
    REQUIRE(propagator.resolve_conflicts());
    REQUIRE(propagator.get_current_level() == 0);
    REQUIRE(!propagator.is_conflicting());
    REQUIRE(propagator.get_trail() == std::vector<Lit>{lnot(vars[1]), vars[2], vars[7], lnot(vars[6])});
    REQUIRE(!propagator.push_level(vars[5]));
    REQUIRE(propagator.is_conflicting());
    REQUIRE(!propagator.resolve_conflicts()); // UNSAT proof!
    REQUIRE(propagator.is_conflicting());
    REQUIRE(propagator.get_current_level() == 0);
}


TEST_CASE("[eliminate_subsumed] Test eliminate subsumed - no subsumed") {
    using namespace sprop;
    auto [vars, model] = waerden33(9);
    Propagator propagator(model);
    ReducedPartialExtractor extractor;
    extractor.extract(propagator);
    auto clause_list = extractor.reduced_clauses();
    REQUIRE((std::ranges::all_of(clause_list, [] (const auto& cl) { return cl.size() == 3; })));
    REQUIRE(extractor.reduced_num_vars() == 9);
    REQUIRE((std::ranges::all_of(propagator.all_literals(), 
             [&] (Lit l) { return extractor.translate_to_new(l) == l; })));
    REQUIRE(clause_list.size() == 32);
    eliminate_subsumed(clause_list, 9);
    REQUIRE(clause_list.size() == 32);
}


TEST_CASE("[eliminate_subsumed] Test eliminate subsumed - corner cases") {
    using namespace sprop;
    std::vector<std::vector<Lit>> clauses{
        {0}, {2}, {2},
        {2, 4}, {2, 5}, {0},
        {0, 3}, {3, 6}, {1, 3, 5}
    };
    eliminate_subsumed(clauses, 4);
    REQUIRE(clauses.size() == 4);
    REQUIRE(std::ranges::count(clauses, std::vector<Lit>{0}) == 1);
    REQUIRE(std::ranges::count(clauses, std::vector<Lit>{2}) == 1);
    REQUIRE(std::ranges::count(clauses, std::vector<Lit>{3, 6}) == 1);
    REQUIRE(std::ranges::count(clauses, std::vector<Lit>{1, 3, 5}) == 1);
}


void no_duplicates_in(const std::vector<sprop::Lit>& eliminated) {
    using namespace sprop;
    std::set<std::vector<Lit>> seen;
}


void validate_subsumed(std::vector<std::vector<sprop::Lit>> original,
                       std::vector<std::vector<sprop::Lit>> eliminated,
                       sprop::Lit num_vars)
{
    using namespace sprop;
    CHECK(original.size() >= eliminated.size());
    // verification step 1: eliminate duplicate clauses from original, 
    // check that eliminated has no duplicates
    auto eliminate_duplicates = [] (auto& clauses) {
        std::set<std::vector<Lit>> seen(clauses.begin(), clauses.end());
        clauses.assign(seen.begin(), seen.end());
    };
    eliminate_duplicates(original);
    std::size_t old_size = eliminated.size();
    eliminate_duplicates(eliminated);
    CHECK(eliminated.size() == old_size);

    // verification step 2: check that all clauses in eliminated are also in original
    for(const auto& cl : eliminated) {
        CHECK(std::ranges::find(original, cl) != original.end());
    }

    // verification step 3: check that all clauses in original are a superset of some clause in eliminated
    for(auto& c : original) std::ranges::sort(c);
    for(auto& c : eliminated) std::ranges::sort(c);
    for(const auto& cl : original) {
        auto is_superset = [&] (const auto& a) {
            return std::ranges::includes(cl, a);
        };
        CHECK(std::ranges::any_of(eliminated, is_superset));
    }

    // verification step 4: check that all clauses in eliminated are not a superset of any other 
    // clause in eliminated, except for themselves
    for(const auto& cl : eliminated) {
        auto is_superset = [&] (const auto& a) {
            return std::ranges::includes(cl, a);
        };
        CHECK(std::ranges::count_if(eliminated, is_superset) == 1);
    }
}


TEST_CASE("[eliminate_subsumed] Test eliminate subsumed - random") {
    using namespace sprop;
    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> nvar_dist(10, 20);
    std::size_t num_clauses = 30;
    std::size_t num_rounds = 1000;
    
    for(std::size_t i = 0; i < num_rounds; ++i) {
        std::vector<std::vector<Lit>> clauses;
        int num_vars = nvar_dist(rng);
        std::generate_n(std::back_inserter(clauses), num_clauses, [&] {
            std::vector<bool> vars_used(num_vars, false);
            std::vector<Lit> clause;
            std::uniform_int_distribution<int> clause_len_dist(1, std::min(15, num_vars));
            std::uniform_int_distribution<Lit> lit_dist(0, 2 * num_vars - 1);
            int len = clause_len_dist(rng);
            for(int i = 0; i < len; ++i) {
                Lit l = lit_dist(rng);
                if(vars_used[lit::var(l)]) {
                    continue;
                }
                vars_used[lit::var(l)] = true;
                clause.push_back(l);
            }
            return clause;
        });
        std::vector<std::vector<Lit>> eliminated(clauses);
        eliminate_subsumed(eliminated, num_vars);
        validate_subsumed(clauses, eliminated, num_vars);
    }
}
