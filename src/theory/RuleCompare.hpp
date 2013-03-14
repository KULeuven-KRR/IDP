#pragma once

#include <set>

class Rule;

// NOTE: if the set contains rules with the same name, the order is still runtime-dependent.
struct RuleCompare{
	bool operator()(const Rule* lhs, const Rule* rhs) const;
};

typedef std::set<Rule*, RuleCompare> ruleset;
