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
#include "generator.hpp"

using namespace std;

int EnumSetGrounder::run() const {
	vector<int>	literals;
	vector<double> weights;
	vector<double> trueweights;
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(l != _false) {
			const GroundTerm& groundweight = _subtermgrounders[n]->run();
			assert(not groundweight._isvarid);
			const DomainElement* d = groundweight._domelement;
			double w = d->type() == DET_INT ? (double) d->value()._int : d->value()._double;
			if(l == _true) trueweights.push_back(w);
			else {
				weights.push_back(w);
				literals.push_back(l);
			}
		}
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

int QuantSetGrounder::run() const {
	vector<int> literals;
	vector<double> weights;
	vector<double> trueweights;
	if(_generator->first()) {
		int l;
		do {
			if(_checker->first()) l = _true;
			else l = _subgrounder->run();
			if(l != _false) {
				const GroundTerm& groundweight = _weightgrounder->run();
				assert(not groundweight._isvarid);
				const DomainElement* weight = groundweight._domelement;
				double w = weight->type() == DET_INT ? (double) weight->value()._int : weight->value()._double;
				if(l == _true) trueweights.push_back(w);
				else {
					weights.push_back(w);
					literals.push_back(l);
				}
			}
		}while(_generator->next());
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}
