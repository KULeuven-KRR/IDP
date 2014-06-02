/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include <vector>
#include <map>
#include <set>

class Structure;
class Theory;
class Definition;
class PFSymbol;
class PredInter;

struct DefinitionRefiningResult {
	bool _hasModel;
	Structure* _calculated_model;
	std::set<PFSymbol*> _refined_symbols;

	DefinitionRefiningResult(Structure* structure) :
		_hasModel(false),
		_calculated_model(structure),
		_refined_symbols() {};
};

class refineStructureWithDefinitions {
public:
	/*
	 * Refines the interpretation of all defined symbols in the theory as much as
	 * possible using the definition and the structure.
	 *
	 * parameter satdelay:
	 *		If true: allow further code to not calculate the definition if it is too big
	 *
	 * parameter symbolsToQuery:
	 * 		A subset of the defined symbols that you are interested in.
	 * 		Defined symbols not in this set will not be refined.
	 */
	static DefinitionRefiningResult doRefineStructureWithDefinitions(Theory* theory,
			Structure* structure, bool satdelay = false,
			std::set<PFSymbol*> symbolsToQuery = std::set<PFSymbol*>()) {
		refineStructureWithDefinitions r;
		return r.refineDefinedSymbols(theory, structure, satdelay,
				symbolsToQuery);
	}
	static DefinitionRefiningResult doRefineStructureWithDefinitions(Definition* definition,
			Structure* structure, bool satdelay = false,
			std::set<PFSymbol*> symbolsToQuery = std::set<PFSymbol*>()) {
		refineStructureWithDefinitions r;
		return r.refineDefinedSymbols(definition, structure, satdelay,
				symbolsToQuery);
	}

private:
	DefinitionRefiningResult refineDefinedSymbols(Theory* theory,
			Structure* structure, bool satdelay,
			std::set<PFSymbol*> symbolsToQuery) const;

	DefinitionRefiningResult refineDefinedSymbols(Definition* definition,
			Structure* structure, bool satdelay,
			std::set<PFSymbol*> symbolsToQuery) const;

	DefinitionRefiningResult processDefinition(const Definition* definition,
			Structure* structure, bool satdelay,
			std::set<PFSymbol*> symbolsToQuery) const;
	bool postprocess(DefinitionRefiningResult&, std::map<PFSymbol*, PredInter*>&) const;
};
