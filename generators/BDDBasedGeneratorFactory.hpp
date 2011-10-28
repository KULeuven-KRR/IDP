/************************************
 BBD2BDDBasedGeneratorFactory.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef BDDBASEDGENERATORFACTORY_HPP_
#define BDDBASEDGENERATORFACTORY_HPP_

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

/**
 * Class to convert a bdd into a generator
 */
class BDDToGenerator {
	private:
		FOBDDManager*	_manager;

		Term* solve(PredForm* atom, Variable* var);
	public:
		BDDToGenerator(FOBDDManager* manager);

		InstGenerator* create(PredForm*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&, AbstractStructure*, bool, const Universe&);
		InstGenerator* create(const FOBDD*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, const Universe&);
		InstGenerator* create(const FOBDDKernel*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, bool, const Universe&);

		GeneratorNode* createnode(const FOBDD*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, const Universe&);
};

#endif /* BDDBASEDGENERATORFACTORY_HPP_ */
