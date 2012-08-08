/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SETGROUNDERS_HPP_
#define SETGROUNDERS_HPP_

#include "inferences/grounding/GroundUtils.hpp"
#include "theory/ecnf.hpp"

class GroundTranslator;
class InstGenerator;
class InstChecker;
class TermGrounder;
class FormulaGrounder;

class SetGrounder {
protected:
	GroundTranslator* _translator;
public:
	SetGrounder(GroundTranslator* gt)
			: _translator(gt) {
	}
	virtual ~SetGrounder() {
	}
	virtual SetId run() const = 0;
	virtual SetId runAndRewriteUnknowns() const = 0; //XXX Needs a better name?
};

class QuantSetGrounder;

class EnumSetGrounder: public SetGrounder {
private:
	std::vector<QuantSetGrounder*> _subgrounders;
public:
	EnumSetGrounder(GroundTranslator* gt, const std::vector<QuantSetGrounder*>& subgrounders)
			: SetGrounder(gt), _subgrounders(subgrounders) {
	}
	~EnumSetGrounder();
	SetId run() const;
	SetId runAndRewriteUnknowns() const;
};

class QuantSetGrounder: public SetGrounder {
private:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator;
	InstChecker* _checker;
	TermGrounder* _weightgrounder;
public:
	QuantSetGrounder(GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, InstChecker* checker, TermGrounder* w)
			: SetGrounder(gt), _subgrounder(gr), _generator(ig), _checker(checker), _weightgrounder(w) {
	}
	~QuantSetGrounder();
	SetId run() const;
	SetId runAndRewriteUnknowns() const;
protected:
	void run(litlist& literals, weightlist& weights, weightlist& trueweights) const;
	void run(weightlist& trueweights, litlist& conditions, termlist& cpterms) const;
	friend class EnumSetGrounder;
};

#endif /* SETGROUNDERS_HPP_ */
