/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef MODELEXPANSION_HPP_
#define MODELEXPANSION_HPP_

#include <vector>
#include <cstdlib>
#include <memory>


class AbstractStructure;
class AbstractTheory;
class Theory;
class TraceMonitor;
class Term;
class Vocabulary;

/**
 * Does model expansion or optimization over:
 * 	- a theory and a structure, with the same vocabulary
 * 	- a minimization term
 * 	- returns a trace of decision, propagations and backtracks if requested
 * 	- if an outputvocabulary, a subvoc of the theory voc, is provided, models only have to be two-valued on the output voc
 */
class ModelExpansion {
public:
	static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, Vocabulary* outputvocabulary = NULL, TraceMonitor* tracemonitor = NULL);
	static std::vector<AbstractStructure*> doMinimization(AbstractTheory* theory, AbstractStructure* structure, Term* term, Vocabulary* outputvocabulary = NULL, TraceMonitor* tracemonitor = NULL);

private:
	Theory* _theory;
	AbstractStructure* _structure;
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done

	Vocabulary* _outputvoc; // if not NULL, mx is allowed to return models which are only two-valued on the outputvoc.

	static std::shared_ptr<ModelExpansion> createMX(AbstractTheory* theory, AbstractStructure* structure, Term* term, Vocabulary* outputvoc,TraceMonitor* tracemonitor);
	ModelExpansion(Theory* theory, AbstractStructure* structure, Term* minimize, TraceMonitor* tracemonitor);

	void setOutputVocabulary(Vocabulary* v);

	std::vector<AbstractStructure*> expand() const;
};
#endif //MODELEXPANSION_HPP_
