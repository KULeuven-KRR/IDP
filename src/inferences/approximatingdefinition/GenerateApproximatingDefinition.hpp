#ifndef APPROXIMATINGDEFINITIONGENERATION_HPP_
#define APPROXIMATINGDEFINITIONGENERATION_HPP_

#include "common.hpp"
#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

struct ApproxData {
	std::map<const Formula*, PredForm*> formula2ct;
	std::map<const Formula*, PredForm*> formula2cf;
	std::set<PFSymbol*> actions;

	ApproxData(const std::set<PFSymbol*>& actions)
			: actions(actions) {
	}
};

class GenerateApproximatingDefinition {
private:
	std::map<Formula*, Predicate*> formula2tseitin;
	ApproxData* data;
	std::vector<Formula*> constraints, assertions; // Sentences!

public:
	GenerateApproximatingDefinition(const std::vector<Formula*>& constraints, const std::vector<Formula*>& assertions, const std::set<PFSymbol*> actions)
			: 	data(new ApproxData(actions)),
				constraints(constraints),
				assertions(assertions) {
		// TODO do transformations on the sentences!
	}
	~GenerateApproximatingDefinition() {
		delete (data);
	}

	enum class Direction {
		UP, DOWN, BOTH
	};

	Definition* getallRules(const std::vector<Formula*>& constraintsentences, const std::vector<Formula*>& assertsentences, Direction dir);

private:
	std::vector<Rule*> getallDownRules(const std::vector<Formula*>& sentences);
	std::vector<Rule*> getallUpRules(const std::vector<Formula*>& sentences);
};

#endif /* APPROXIMATINGDEFINITIONGENERATION_HPP_ */
