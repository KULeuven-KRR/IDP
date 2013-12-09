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
#include "GlobalData.hpp"
#include <map>

class LTCVocInfo;
class Vocabulary;
class Sort;
class Function;
class AbstractTheory;
class SplitLTCTheory;

class LTCData: public DeleteMe {
private:
	std::map<const Vocabulary*, const LTCVocInfo*> _LTCVocData;
	std::map<const AbstractTheory*, const SplitLTCTheory*> _LTCTheoData;
public:
	static LTCData* instance();
	~LTCData();

	bool hasBeenTransformed(const Vocabulary*);
	const LTCVocInfo* getStateVocInfo(const Vocabulary*);
	void registerTransformation(const Vocabulary*, const LTCVocInfo*);

	bool hasBeenSplit(const AbstractTheory*);
	const SplitLTCTheory* getSplitTheory(const AbstractTheory*);

};

struct LTCInputData {
	const Sort* time;
	Function* start;
	Function* next;
};

