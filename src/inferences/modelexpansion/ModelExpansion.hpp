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
#include <cstdlib>
#include <memory>

class Structure;
class AbstractTheory;
class Theory;
class TraceMonitor;
class Term;
class Vocabulary;
class PredForm;
class Predicate;
class PFSymbol;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

struct DomainAtom{
	PFSymbol* symbol;
	ElementTuple args;
};

struct MXResult{
	std::vector<Structure*> _models;
	int _optimalvalue; //Only relevant when minimizing. This equals the optimal value.
	bool _optimumfound = false; //Only relevant when minimizing. If this bool is true, all returned models are optimal. If false, nothing is guaranteed.
	bool unsat = false;
	bool _interrupted = false;
	std::vector<DomainAtom> unsat_in_function_of_ct_lits;
};

struct MXAssumptions{
	std::vector<DomainAtom> assumeFalse;
	std::vector<Predicate*> assumeAllFalse;

	MXAssumptions(const std::vector<DomainAtom>& assumeFalse = {}, const std::vector<Predicate*>& assumeAllFalse = {}): assumeFalse(assumeFalse), assumeAllFalse(assumeAllFalse){

	}
};

/**
 * Does model expansion or optimization over:
 * 	- a theory and a structure, with the same vocabulary
 * 	- a minimization term
 * 	- returns a trace of decision, propagations and backtracks if requested
 * 	- if an outputvocabulary, a subvoc of the theory voc, is provided, models only have to be two-valued on the output voc
 */
class ModelExpansion {
public:
	static MXResult doModelExpansion(AbstractTheory* theory, Structure* structure, Vocabulary* outputvocabulary = NULL, TraceMonitor* tracemonitor = NULL, const MXAssumptions& assumeFalse = {});
	static MXResult doMinimization(AbstractTheory* theory, Structure* structure, Term* term, Vocabulary* outputvocabulary = NULL, TraceMonitor* tracemonitor = NULL, const MXAssumptions& assumeFalse = {});

private:
	Theory* _theory;
	Structure* _structure;
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done

	Vocabulary* _outputvoc; // if not NULL, mx is allowed to return models which are only two-valued on the outputvoc.
	MXAssumptions _assumeFalse;

	static std::shared_ptr<ModelExpansion> createMX(AbstractTheory* theory, Structure* structure, Term* term, Vocabulary* outputvoc,TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse = {});
	ModelExpansion(Theory* theory, Structure* structure, Term* minimize, TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse = {});

	void setOutputVocabulary(Vocabulary* v);

	MXResult expand() const;
};
