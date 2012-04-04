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

#include "GroundUtils.hpp"

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
};

class EnumSetGrounder: public SetGrounder {
private:
	std::vector<FormulaGrounder*> _subgrounders;
	std::vector<TermGrounder*> _subtermgrounders;
public:
	EnumSetGrounder(GroundTranslator* gt, const std::vector<FormulaGrounder*>& subgr, const std::vector<TermGrounder*>& subtgr)
			: SetGrounder(gt), _subgrounders(subgr), _subtermgrounders(subtgr) {
	}
	SetId run() const;
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
	SetId run() const;
};

#endif /* SETGROUNDERS_HPP_ */
