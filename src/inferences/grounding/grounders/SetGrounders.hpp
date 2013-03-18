/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef SETGROUNDERS_HPP_
#define SETGROUNDERS_HPP_

#include "inferences/grounding/GroundUtils.hpp"
#include "theory/ecnf.hpp"
#include "theory/Sets.hpp"

class GroundTranslator;
class InstGenerator;
class InstChecker;
class TermGrounder;
class FormulaGrounder;

class SetGrounder {
protected:
	int id;
	std::vector<const DomElemContainer*> _freevarcontainers;
	var2dommap _varmap;
	GroundTranslator* _translator;
public:
	SetGrounder(std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt);
	virtual ~SetGrounder() {
	}
	virtual SetId run() = 0;
	virtual SetId runAndRewriteUnknowns() = 0; //XXX Needs a better name?

	virtual bool hasSet() const = 0;
	virtual SetExpr* getSet() const = 0;
	const var2dommap& getVarmapping() const{
		return _varmap;
	}
};

class QuantSetGrounder;

class EnumSetGrounder: public SetGrounder {
private:
	EnumSetExpr* _set;
	std::vector<QuantSetGrounder*> _subgrounders;
public:
	EnumSetGrounder(std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt, const std::vector<QuantSetGrounder*>& subgrounders);
	~EnumSetGrounder();
	SetId run();
	SetId runAndRewriteUnknowns();

	virtual bool hasSet() const{
			return _set!=NULL;
	};
	virtual EnumSetExpr* getSet() const {
		Assert(hasSet());
		return _set;
	}
};

class QuantSetGrounder: public SetGrounder {
private:
	QuantSetExpr* _set;
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator;
	InstChecker* _checker;
	TermGrounder* _weightgrounder;
public:
	QuantSetGrounder(QuantSetExpr* expr, std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, InstChecker* checker, TermGrounder* w);
	~QuantSetGrounder();
	SetId run();
	SetId runAndRewriteUnknowns();

	virtual bool hasSet() const{
		return _set!=NULL;
	}
	virtual QuantSetExpr* getSet() const {
		Assert(hasSet());
		return _set;
	}
protected:
	void run(litlist& literals, weightlist& weights, weightlist& trueweights);
	void run(weightlist& trueweights, litlist& conditions, termlist& cpterms);
	friend class EnumSetGrounder;
};

#endif /* SETGROUNDERS_HPP_ */
