#pragma once

#include <set>

class Variable;

// NOTE: if the set contains variables with the same name, the order is still runtime-dependent.
struct VarCompare{
	bool operator()(const Variable* lhs, const Variable* rhs) const;
};

typedef std::set<Variable*, VarCompare> varset;
