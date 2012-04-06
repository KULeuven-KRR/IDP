/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDTERMTRANSLATOR_HPP_
#define GROUNDTERMTRANSLATOR_HPP_

#include "Utils.hpp"

class Function;
class AbstractStructure;
class GroundTerm;
class CPTsBody;
class SortTable;
class CPTerm;

typedef size_t SymbolOffset;

class GroundTermTranslator {
private:
	AbstractStructure* _structure;

	std::vector<std::map<std::vector<GroundTerm>, VarId> > _functerm2varid_table; //!< map function term to CP variable identifier
	std::vector<Function*> _varid2function; //!< map CP varid to the symbol of its corresponding term
	std::vector<std::vector<GroundTerm> > _varid2args; //!< map CP varid to the terms of its corresponding term

	std::vector<Function*> _offset2function;
	std::map<Function*, SymbolOffset> _function2offset;

	std::map<VarId, CPTsBody*> _varid2cprelation;

	std::vector<SortTable*> _varid2domain;

public:
	GroundTermTranslator(AbstractStructure* str)
			: _structure(str), _varid2function(1), _varid2args(1), _varid2domain(1) {
	}
	~GroundTermTranslator() {
		//Note: Does not delete anything!
	}

	// Methods for translating terms to variable identifiers
	VarId translate(SymbolOffset offset, const std::vector<GroundTerm>&);
	VarId translate(Function*, const std::vector<GroundTerm>&);
	VarId translate(CPTerm*, SortTable*);
	VarId translate(const DomainElement*);

	// Adding variable identifiers and functions
	VarId nextNumber();
	SymbolOffset addFunction(Function*);

	// Methods for translating variable identifiers to terms
	Function* function(const VarId& varid) const {
		return _varid2function[varid];
	}
	const std::vector<GroundTerm>& args(const VarId& varid) const {
		return _varid2args[varid];
	}
	CPTsBody* cprelation(const VarId& varid) const {
		return _varid2cprelation.find(varid)->second;
	}
	SortTable* domain(const VarId& varid) const {
		return _varid2domain[varid];
	}

	size_t nrOffsets() const {
		return _offset2function.size();
	}
	SymbolOffset getOffset(Function* func) const {
		return _function2offset.at(func);
	}
	const Function* getFunction(SymbolOffset offset) const {
		return _offset2function[offset];
	}

	std::string printTerm(const VarId&) const;
};

#endif /* GROUNDTERMTRANSLATOR_HPP_ */
