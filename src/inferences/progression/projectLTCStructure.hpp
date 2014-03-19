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
struct LTCVocInfo;
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

	bool _shouldUseStart; //Whether or not the initialise inference will use the interpretation of structures at time point "Start"
	bool _forceIgnoreStart; //Whether or not the user has explicitely asked to ignore info on time point Start
	const DomainElement* _startDomElem;

	Structure* _result;

public:
	/**Projects the input structure. If ignoreStart holds: all info about Start will be ignored. Otherwise, info about start will be used for projecting onto initial state*/
	static Structure* projectStructure(const Structure* inputStructure, bool ignoreStart = false) {
		auto g = LTCStructureProjector();
		g.init(inputStructure, ignoreStart);
		return g.run();
	}
private:
	void init(const Structure* input, bool ignoreStart);
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

