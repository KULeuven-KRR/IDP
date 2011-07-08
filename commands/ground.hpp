/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP_
#define GROUND_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "options.hpp"

class GroundInference: public Inference {
private:
	AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options) const {
		GrounderFactory factory(structure,options);
		TopLevelGrounder* grounder = factory.create(theory);
		grounder->run();
		AbstractGroundTheory* grounding = grounder->grounding();
		grounding->addFuncConstraints();
		delete(grounder);
		return grounding;
	}

public:
	GroundInference(): Inference("ground") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* grounding = ground(args[0].theory(),args[1].structure(),args[2].options());
		return InternalArgument(grounding);
	}
};

class GroundAndPrintInference: public Inference {
private:
	AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options, InteractivePrintMonitor* monitor) const {
		GrounderFactory factory(structure,options);
		TopLevelGrounder* grounder = factory.create(theory, monitor, options);
		grounder->run();
		AbstractGroundTheory* grounding = grounder->grounding();
		grounding->addFuncConstraints();
		delete(grounder);
		monitor->flush();
		return grounding;
	}

public:
	GroundAndPrintInference(): Inference("printgrounding", true) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* grounding = ground(args[0].theory(),args[1].structure(),args[2].options(), printmonitor());
		return InternalArgument(grounding);
	}
};

#endif /* GROUND_HPP_ */
