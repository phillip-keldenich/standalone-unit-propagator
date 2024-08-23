#include <standalone-propagator/propagator.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <ranges>
#include <vector>
#include <cstddef>


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
