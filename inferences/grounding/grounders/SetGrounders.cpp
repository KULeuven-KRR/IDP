/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "SetGrounders.hpp"

#include "common.hpp"
#include "TermGrounders.hpp"
#include "FormulaGrounders.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/BasicCheckers.hpp"
#include "ecnf.hpp"
#include "structure.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder, vector<int>& literals, vector<double>& weights, vector<double>& trueweights, InstChecker& checker){
	Lit l;
	if(checker.check()){
		l = _true;
	}else{
		l = sublitgrounder.groundAndReturnLit();
	}
	if(l==_false){
		return;
	}
	const auto& groundweight = subtermgrounder.run();
	Assert(not groundweight.isVariable);
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
	InstChecker* checker = new FalseInstChecker();
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
