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
#include <set>
#include <map>

class Structure;
class Theory;
class Definition;
class Vocabulary;
class PFSymbol;

/*
 * The resulting struct for a definition calculation. It contains:
 *
 *  - A bool to indicate whether the total definition calculation resulted
 *    in a two-valued structure.
 *  - The resulting structure.
 *  - The calculated definitions. The user is responsible for deleting these
 *    properly, as they will no longer be present in the theory.
 *
 *  If not all definitions could be calculated (one of them resulted in an
 *  inconsistent result), the structure in this struct is the one used for
 *  the unsuccessful definition calculation and the set of definitions
 *  contains all (up to that point) successfully calculated definitions.
 */
struct DefinitionCalculationResult {
	bool _hasModel;
	Structure* _calculated_model;

	DefinitionCalculationResult(Structure* structure) :
		_hasModel(false),
		_calculated_model(structure) {};


};

class CalculateDefinitions {
public:
	/*
	 * !Removes calculated definitions from the theory.
	 * Also modifies the structure. Clone your theory and structure before doing this!
	 *
	 * parameter satdelay:
	 *		If true: allow further code to not calculate the definition if it is too big
	 *
	 * parameter symbolsToQuery:
	 * 		A subset of the defined symbols in the theory that you are interested in.
	 * 		Defined symbols not in this set will not be calculated by IDP.
	 * 		They might be partially evaluated (i.e., by XSB) during evaluation of symbols that are in this set.
	 */
	static DefinitionCalculationResult doCalculateDefinitions(
			Theory* theory, Structure* structure, bool satdelay = false);
	static DefinitionCalculationResult doCalculateDefinitions(
			Theory* theory, Structure* structure,
			Vocabulary* vocabulary, bool satdelay = false);
	
	static DefinitionCalculationResult doCalculateDefinition(
			const Definition* definition, Structure* structure, bool satdelay = false);
	static DefinitionCalculationResult doCalculateDefinition(
			const Definition* definition, Structure* structure,
			Vocabulary* vocabulary, bool satdelay = false);

#ifdef WITHXSB
	static bool determineXSBUsage(const Definition* definition);
#endif

private:
	Theory* _theory;
	Structure* _structure;
	std::set<PFSymbol*> _symbolsToQuery;
	bool _satdelay;
	bool _tooExpensive;
	
	CalculateDefinitions(Theory*, Structure*, Vocabulary*, bool);
	
	DefinitionCalculationResult calculateKnownDefinitions();
	DefinitionCalculationResult calculateDefinition(const Definition* definition);


	/** Splitting of definition may have caused the given set of symbolsToQuery to not be enough:
	 *  E.g. Definition
	 *  { p <- q.
	 *    q <- r. }
	 *  is split into two definitions:
	 *  { p <- q. }
	 *  { q <- r. }
	 *  If the initial symbol to query was only { p }, then we need to add q as well, since we'll be
	 *  querying the second definition and need to query q in order to evaluate it.
	 */
	void updateSymbolsToQuery(std::set<PFSymbol*>& symbolsToQuery, std::vector<Definition*>) const;
	
	// For the current structure, determine which defined symbols can be evaluated (= are input*)
	std::set<PFSymbol*> determineInputStarSymbols(Theory*) const;
	// For the current structure, determine add the input* symbols of the given definition to the set given as second argument
	void addNewInputStar(const Definition*, std::set<PFSymbol*>&) const;

	static void removeNonTotalDefnitions(std::map<Definition*, std::set<PFSymbol*> >& opens);
	bool definitionDoesNotResultInTwovaluedModel(const Definition*) const;
};
