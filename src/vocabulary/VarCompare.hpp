#pragma once

#include <set>
#include <utils/ListUtils.hpp>

class Variable;

// NOTE: if the set contains variables with the same name, the order is still runtime-dependent.
struct VarCompare{
	bool operator()(const Variable* lhs, const Variable* rhs) const;
};

typedef std::set<Variable*, VarCompare> varset;

template<class L>
varset getVarSet(const L& vars){
	return getSet<Variable*, VarCompare>(vars);
}
