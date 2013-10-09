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

#pragma once
#include "commontypes.hpp"

class Structure;
class Structure;
class Vocabulary;
class LTCVocInfo;
class Sort;
class Function;
class DomainElement;
class PFSymbol;


class LTCStructureProjector {
private:
	const Structure* _inputStruc;
	const Vocabulary* _ltcVoc;
	const LTCVocInfo* _vocInfo;
	Vocabulary* _stateVoc;

	const Sort* _time;
	const Function* _start;
	const Function* _next;

	bool _interpretsStart;
	const DomainElement* _startDomElem;

	Structure* _result;

public:
	static Structure* projectStructure(const Structure* inputStructure) {
		auto g = LTCStructureProjector();
		g.init(inputStructure);
		return g.run();
	}
private:
	void init(const Structure* input);
	Structure* run();

	void setSorts();
	void projectPredicates();
	void projectFunctions();

	template<class T>
	void projectSymbol(T* s);

	template<class T>
	void cloneAndSetInter(T* object);

	void projectAndSetInter(PFSymbol* object);
	/**
	 * Returns an identical tuple, with the element at place i left out
	 */
	const ElementTuple projectTuple(const ElementTuple& tuple, size_t i);
};

