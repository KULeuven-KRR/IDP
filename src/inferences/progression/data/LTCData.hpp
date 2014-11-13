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
#include <vector>

struct LTCVocInfo;
class Vocabulary;
class Sort;
class PFSymbol;
class Function;
class AbstractTheory;
struct SplitLTCTheory;

struct LTCInputData {
	const Sort* time;
	Function* start;
	Function* next;
};

class LTCData: public DeleteMe {
private:
	std::map<const Vocabulary*, const LTCVocInfo*> _LTCVocData;
	std::map<const AbstractTheory*, const SplitLTCTheory*> _LTCTheoData;
	std::map<const PFSymbol*, std::vector<PFSymbol*>> _projectedSymbols;
public:
	static LTCData* instance();
	~LTCData();

	bool hasBeenProjected(PFSymbol* pf);
	std::vector<PFSymbol*> getProjection(PFSymbol* pf);
	void setProjection(PFSymbol* pf,std::vector<PFSymbol*>);

	bool hasBeenTransformed(const Vocabulary*);
	const LTCVocInfo* getStateVocInfo(const Vocabulary*);
	const LTCVocInfo* getStateVocInfo(const Vocabulary*, LTCInputData symbols);
	void registerTransformation(const Vocabulary*, const LTCVocInfo*);

	bool hasBeenSplit(const AbstractTheory*);
	const SplitLTCTheory* getSplitTheory(const AbstractTheory*);
private:
	LTCInputData collectLTCSortAndFunctions(const Vocabulary* ltcVoc) const;
	void verify(const LTCInputData& data) const;


};



