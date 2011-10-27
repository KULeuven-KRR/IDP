/************************************
 SetGrounders.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/SetGrounders.hpp"

#include <cassert>
#include "grounders/TermGrounders.hpp"
#include "grounders/FormulaGrounders.hpp"
#include "common.hpp"
#include "generators/InstGenerator.hpp"

using namespace std;

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder, vector<int>& literals, vector<double>& weights, vector<double>& trueweights, InstChecker& checker){
	Lit l;
	if(checker.check()){
		l = _true;
	}else{
		l = sublitgrounder.run();
	}
	if(l==_false){
		return;
	}
	const auto& groundweight = subtermgrounder.run();
	assert(not groundweight._isvarid);
	const auto& d = groundweight._domelement;
	double w = d->type() == DET_INT ? (double) d->value()._int : d->value()._double;
	if(l == _true){
		trueweights.push_back(w);
	} else {
		weights.push_back(w);
		literals.push_back(l);
	}
}

int EnumSetGrounder::run() const {
	vector<int>	literals;
	vector<double> weights;
	vector<double> trueweights;
	InstChecker* checker = new TrueInstanceChecker();
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		groundSetLiteral(*_subgrounders[n], *_subtermgrounders[n], literals, weights, trueweights, *checker);
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

int QuantSetGrounder::run() const {
	vector<int> literals;
	vector<double> weights;
	vector<double> trueweights;
	for(_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()){
		groundSetLiteral(*_subgrounder, *_weightgrounder, literals, weights, trueweights, *_checker);
	}
	Lit s = _translator->translateSet(literals,weights,trueweights);
	return s;
}
