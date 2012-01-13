/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "GroundTermTranslator.hpp"
#include "ecnf.hpp"
#include "structure.hpp"
#include "vocabulary.hpp"

using namespace std;

VarId GroundTermTranslator::translate(size_t offset, const vector<GroundTerm>& args) {
	map<vector<GroundTerm>, VarId>::iterator it = _functerm2varid_table[offset].lower_bound(args);
	if (it != _functerm2varid_table[offset].cend() && it->first == args) {
		return it->second;
	} else {
		VarId varid = nextNumber();
		_functerm2varid_table[offset].insert(it, pair<vector<GroundTerm>,VarId>{args, varid});
		_varid2function[varid] = _offset2function[offset];
		_varid2args[varid] = args;
		_varid2domain[varid] = _structure->inter(_offset2function[offset]->outsort());
		return varid;
	}
}

VarId GroundTermTranslator::translate(Function* function, const vector<GroundTerm>& args) {
	size_t offset = addFunction(function);
	return translate(offset, args);
}

VarId GroundTermTranslator::translate(CPTerm* cpterm, SortTable* domain) {
	VarId varid = nextNumber();
	CPBound bound(varid);
	CPTsBody* cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	_varid2cprelation.insert(pair<VarId, CPTsBody*>{varid, cprelation});
	_varid2domain[varid] = domain;
	return varid;
}

VarId GroundTermTranslator::translate(const DomainElement* element) {
	VarId varid = nextNumber();
	// Create a new CP variable term
	CPVarTerm* cpterm = new CPVarTerm(varid);
	// Create a new CP bound based on the domain element
	Assert(element->type() == DET_INT);
	CPBound bound(element->value()._int);
	// Add a new CP constraint
	CPTsBody* cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	_varid2cprelation.insert(pair<VarId, CPTsBody*>{varid, cprelation});
	// Add a new domain containing only the given domain element
	SortTable* domain = new SortTable(new EnumeratedInternalSortTable());
	domain->add(element);
	_varid2domain[varid] = domain;
	// Return the new variable identifier
	return varid;
}

size_t GroundTermTranslator::nextNumber() {
	size_t nr = _varid2function.size();
	_varid2function.push_back(0);
	_varid2args.push_back(vector<GroundTerm>(0));
	_varid2domain.push_back(0);
	return nr;
}

size_t GroundTermTranslator::addFunction(Function* func) {
	map<Function*, size_t>::const_iterator found = _function2offset.find(func);
	if (found != _function2offset.cend()) {
		// Simply return number when function is already known
		return found->second;
	} else {
		// Add function and number when function is unknown
		size_t offset = _offset2function.size();
		_function2offset[func] = offset;
		_offset2function.push_back(func);
		_functerm2varid_table.push_back(map<vector<GroundTerm>, VarId>());
		return offset;
	}
}

string GroundTermTranslator::printTerm(const VarId& varid) const {
	stringstream s;
	if (varid >= _varid2function.size()) {
		return "error";
	}
	const Function* func = function(varid);
	if (func) {
		s << toString(func);
		if (not args(varid).empty()) {
			s << "(";
			for (auto gtit = args(varid).cbegin(); gtit != args(varid).cend(); ++gtit) {
				if ((*gtit).isVariable) {
					s << printTerm((*gtit)._varid);
				} else {
					s << toString((*gtit)._domelement);
				}
				if (gtit != args(varid).cend() - 1) {
					s << ",";
				}
			}
			s << ")";
		}
	} else {
		s << "var_" << varid;
	}
	return s.str();
}
