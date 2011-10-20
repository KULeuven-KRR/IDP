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
void groundSetLiteral(const LitGrounder& lg, const TermGrounder& tg, vector<int>& literals, vector<double>& weights, vector<double>& trueweights, InstGenerator* checker = NULL){
	Lit l;
	if(checker!=NULL && checker->first()){
		l = _true;
	}else{
		l = lg.run();
	}
	if(l==_false){
		return;
	}
	const auto& groundweight = tg.run();
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
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		groundSetLiteral(*_subgrounders[n], *_subtermgrounders[n], literals, weights, trueweights);
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

int QuantSetGrounder::run() const {
	vector<int> literals;
	vector<double> weights;
	vector<double> trueweights;
	if(_generator->first()) {
		do {
			groundSetLiteral(*_subgrounder, *_weightgrounder, literals, weights, trueweights, _checker);
		}while(_generator->next());
	}
	Lit s = _translator->translateSet(literals,weights,trueweights);
	return s;
}
