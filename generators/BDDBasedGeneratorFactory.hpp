/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef BDDBASEDGENERATORFACTORY_HPP_
#define BDDBASEDGENERATORFACTORY_HPP_

#include "commontypes.hpp"
#include "generators/GeneratorFactory.hpp"

class FOBDDManager;
class FOBDDVariable;
class FOBDD;
class FOBDDKernel;
class GeneratorNode;
class Term;
class PredForm;
class Variable;
class InstGenerator;
class DomElemContainer;
class AbstractStructure;
class Universe;

struct BddGeneratorData{
	const FOBDD* bdd;
	std::vector<Pattern> pattern;
	std::vector<const DomElemContainer*> vars;
	std::vector<const FOBDDVariable*> bddvars;
	AbstractStructure* structure;
	Universe universe;

	bool check() const {
		return pattern.size()==vars.size() && pattern.size()==bddvars.size() && pattern.size()==universe.tables().size();
	}
};

/**
 * Class to convert a bdd into a generator
 */
class BDDToGenerator {
private:
	FOBDDManager* _manager;

	InstGenerator* createFromPredForm(PredForm*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
				AbstractStructure*, bool, const Universe&);
	InstGenerator* createFromKernel(const FOBDDKernel*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&,
				const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, bool, const Universe&);

	GeneratorNode* createnode(const BddGeneratorData& data);

public:
	BDDToGenerator(FOBDDManager* manager);

	/**
	 * Create a generator which generates all instances for which the formula is CERTAINLY TRUE in the given structure.
	 */
	InstGenerator* create(const BddGeneratorData& data);
};

#endif /* BDDBASEDGENERATORFACTORY_HPP_ */
