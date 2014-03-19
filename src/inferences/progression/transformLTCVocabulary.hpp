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

#include <vector>
#include <map>
#include "data/StateVocInfo.hpp"
#include "data/LTCData.hpp"

class Vocabulary;
class Sort;
class PFSymbol;
class Predicate;
class Function;
struct LTCInputData;


class LTCVocabularyTransformer {
	const Vocabulary* _ltcVoc;
	LTCInputData _inputData;

	LTCVocInfo* _result;

public:
	static LTCVocInfo* TransformVocabulary(const Vocabulary* ltcVoc, LTCInputData inputData){
		auto g = LTCVocabularyTransformer(ltcVoc, inputData);
		return g.transform();
	}
private:

	LTCVocabularyTransformer(const Vocabulary* ltcVoc, LTCInputData inputData);
	~LTCVocabularyTransformer();

	LTCVocInfo* transform();

	std::vector<PFSymbol*> projectSymbol(PFSymbol* p);
	std::vector<Sort*> projectSorts(PFSymbol* p);
	void projectAndAddSymbol(PFSymbol* pred);
};



