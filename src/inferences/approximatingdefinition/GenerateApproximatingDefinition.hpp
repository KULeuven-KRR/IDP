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
	std::vector<Formula*> _sentences;

public:
	enum class Direction {
		UP, DOWN, BOTH
	};

	static Definition* doGenerateApproximatingDefinition(const std::vector<Formula*>& sentences, const std::set<PFSymbol*>& freesymbols, Direction dir){
		auto g = GenerateApproximatingDefinition(sentences, freesymbols);
		// FIXME what with new vocabulary?
		return g.getallRules(dir);
	}

private:
	GenerateApproximatingDefinition(const std::vector<Formula*>& sentences, const std::set<PFSymbol*>& actions)
			: 	data(new ApproxData(actions)) {
		// TODO do transformations on the sentences
		// TODO do tseitin introduction + generate new vocabulary
		_sentences = sentences;
	}
	~GenerateApproximatingDefinition() {
		delete (data);
	}

	Definition* getallRules(Direction dir);

	std::vector<Rule*> getallDownRules();
	std::vector<Rule*> getallUpRules();
};

#endif /* APPROXIMATINGDEFINITIONGENERATION_HPP_ */
