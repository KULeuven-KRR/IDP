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

#ifndef BDDBASEDGENERATORFACTORY_HPP_
#define BDDBASEDGENERATORFACTORY_HPP_

#include "commontypes.hpp"
#include "GeneratorFactory.hpp"
#include "fobdds/FoBdd.hpp"

class FOBDDManager;
class FOBDDVariable;
class FOBDDAggKernel;
class FOBDD;
class FOBDDKernel;
class GeneratorNode;
class Term;
class PredForm;
class Variable;
class InstGenerator;
class DomElemContainer;
class Structure;
class Universe;

struct BddGeneratorData {
	const FOBDD* bdd;
	std::vector<Pattern> pattern;
	std::vector<const DomElemContainer*> vars;
	std::vector<const FOBDDVariable*> bddvars;
	const Structure* structure;
	Universe universe;

	BddGeneratorData(): bdd(NULL), structure(NULL){

	}

	bool check() const {
		//std::cerr <<print(bdd) <<"\n\n";
		return bdd!=NULL && structure!=NULL && pattern.size() == vars.size() && pattern.size() == bddvars.size() && pattern.size() == universe.tables().size();
	}
};

enum class BRANCH {
	FALSEBRANCH, TRUEBRANCH
};

std::ostream& operator<<(std::ostream& output, const BRANCH& type);
PRINTTOSTREAM(BRANCH)

/**
 * Class to convert a bdd into a generator
 */
class BDDToGenerator {
private:
	std::shared_ptr<FOBDDManager> _manager;

	/*
	 * Help-method for creating from predform
	 */
	PredForm* smartGraphFunction(PredForm* atom, const std::vector<Pattern>& pattern, const std::vector<Variable*>& atomvars, const Structure* structure);

	/*
	 * Creates an instance generator from a predform (i.e.~an atom kernel).
	 * Can only be used on atoms that contain only var and domain terms (no functions and aggregates)
	 */
	InstGenerator *createFromSimplePredForm(PredForm *atom, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*> & vars,
			const std::vector<Variable*> & atomvars, const Structure *structure, BRANCH branchToGenerate, const Universe & universe);

	/*
	 * Creates an instance generator from a predform (i.e.~an atom kernel).
	 * branchToGenerate determines whether all instances for which the predform evaluates to true
	 * or all instances for which the predform evaluates to false are generated
	 */
	InstGenerator* createFromPredForm(PredForm*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
			const Structure*, BRANCH branchToGenerate, const Universe&);

	/*
	 * Creates an instance generator from a aggform (i.e.~an aggkernel).
	 * branchToGenerate determines whether all instances for which the predform evaluates to true
	 * or all instances for which the aggform evaluates to false are generated
	 * PRECONDITION: the first argument is an aggform of which the left term is no aggregate nor functerm
	 */
	InstGenerator* createFromAggForm(AggForm*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
			const Structure*, BRANCH branchToGenerate, const Universe&);

	/*
	 * Creates an instance generator from a formula
	 * PRECONDITION: f is either a predform or an aggform of which the left term is no
	 * aggregate nor functerm (i.e.~is the direct translation of an atomkernel or an aggkernel)
	 */
	InstGenerator* createFromFormula(Formula* f, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
			const Structure*, BRANCH branchToGenerate, const Universe&);
	/*
	 * Creates an instance generator from a kernel.  branchToGenerate determines whether all instances for which the kernel evaluates to true
	 * or all instances for which the kernel evaluates to false are generated
	 */
	InstGenerator* createFromKernel(const FOBDDKernel*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&,
			const std::vector<const FOBDDVariable*>&, const Structure* structure, BRANCH branchToGenerate, const Universe&);

	/*
	 * Same as createfromKernel, but for aggkernels only.
	 */
	InstGenerator* createFromAggKernel(const FOBDDAggKernel*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&,
			const std::vector<const FOBDDVariable*>&, const Structure* structure, BRANCH branchToGenerate, const Universe&);

	std::vector<InstGenerator*> turnConjunctionIntoGenerators(const std::vector<Pattern> & pattern, const std::vector<const DomElemContainer*> & vars,
			const std::vector<Variable*> & atomvars, const Universe & universe, QuantForm *quantform, const Structure *structure,
			std::vector<Formula*> conjunction, Formula *origatom, BRANCH branchToGenerate);

	/**
	 * The boolean represents whether or not we should optimze the Bdd. In general: we optimize every time we enter a quantkernel (and initially)
	 * --> Every time an extra variable is queried
	 */
	InstGenerator* createFromBDD(const BddGeneratorData& data, bool optimize = false);

public:
	BDDToGenerator(std::shared_ptr<FOBDDManager> manager);

	/**
	 * Create a generator which generates all instances for which the formula is CERTAINLY TRUE in the given structure.
	 */
	InstGenerator* create(const BddGeneratorData& data);
};

#endif /* BDDBASEDGENERATORFACTORY_HPP_ */
