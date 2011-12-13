#ifndef GROUND_HPP_
#define GROUND_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "options.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"

class GroundInference: public Inference {
private:
	AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure) const {
		// TODO bugged! auto symstructure = generateApproxBounds(theory, structure);
		auto symstructure = generateNaiveApproxBounds(theory, structure);
		GrounderFactory factory(structure,symstructure);
		auto grounder = factory.create(theory);
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();
		//DEBUG CODE: std::clog <<toString(grounding) <<"\n";
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
		GlobalData::instance()->setOptions(args[2].options());
		AbstractTheory* grounding = ground(args[0].theory(),args[1].structure());
		return InternalArgument(grounding);
	}
};

// TODO add an option to write to a file instead to stdout.

class GroundAndPrintInference: public Inference {
private:
	AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, InteractivePrintMonitor* monitor) const {
		auto symstructure = generateNaiveApproxBounds(theory, structure);
		GrounderFactory factory(structure, symstructure);
		auto grounder = factory.create(theory,monitor);
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();
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
		GlobalData::instance()->setOptions(args[2].options());
		auto grounding = ground(args[0].theory(),args[1].structure(), printmonitor());
		return InternalArgument(grounding);
	}
};

#endif /* GROUND_HPP_ */
