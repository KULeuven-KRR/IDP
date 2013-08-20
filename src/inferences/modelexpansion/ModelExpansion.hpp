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

struct MXResult{
	std::vector<Structure*> _models;
	int _optimalvalue; //Only relevant when minimizing. This equals the optimal value.
	bool _optimumfound; //Only relevant when minimizing. If this bool is true, all returned models are optimal. If false, nothing is guaranteed.
	bool unsat;
	std::vector<PredForm*> unsat_in_function_of_ct_lits;
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
	static MXResult doModelExpansion(AbstractTheory* theory, Structure* structure, Vocabulary* outputvocabulary = NULL, TraceMonitor* tracemonitor = NULL, const std::vector<Predicate*>& assumeFalse = {});
	static MXResult doMinimization(AbstractTheory* theory, Structure* structure, Term* term, Vocabulary* outputvocabulary = NULL, TraceMonitor* tracemonitor = NULL);

private:
	Theory* _theory;
	Structure* _structure;
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done

	Vocabulary* _outputvoc; // if not NULL, mx is allowed to return models which are only two-valued on the outputvoc.
	std::vector<Predicate*> _assumeFalse;

	static std::shared_ptr<ModelExpansion> createMX(AbstractTheory* theory, Structure* structure, Term* term, Vocabulary* outputvoc,TraceMonitor* tracemonitor, const std::vector<Predicate*>& assumeFalse);
	ModelExpansion(Theory* theory, Structure* structure, Term* minimize, TraceMonitor* tracemonitor, const std::vector<Predicate*>& assumeFalse);

	void setOutputVocabulary(Vocabulary* v);

	MXResult expand() const;
};
