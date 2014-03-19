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

class Structure;
class Structure;
class AbstractTheory;
class Sort;
class Function;
class Vocabulary;
struct LTCInputData;
class Theory;
struct LTCVocInfo;

struct initData {
	std::vector<Structure*> _models;
	Theory* _initTheo;
	Theory* _bistateTheo;
	Vocabulary* _onestateVoc;
	Vocabulary* _bistateVoc;
};

class InitialiseInference {
	const AbstractTheory* _ltcTheo;
	const Structure* _inputStruc;
	Structure* _projectedStructure;

	const Sort* _timeInput;
	Function* _startInput;
	Function* _nextInput;

	const LTCVocInfo* _vocInfo;

public:
	static initData doInitialisation(const AbstractTheory* ltcTheo, const Structure* str, const Sort* Time, Function* Start, Function* Next);
private:

	InitialiseInference(const AbstractTheory* ltcTheo, const Structure* str, const Sort* Time, Function* Start, Function* Next);
	~InitialiseInference();
	void prepareVocabulary();
	initData init();

	void prepareStructure();

};

class ProgressionInference {
private:
	const AbstractTheory* _ltcTheo;
	const Structure* _stateBefore;

public:

	static std::vector<Structure*> doProgression(const AbstractTheory* ltcTheo, const Structure* stateBefore);

private:

	ProgressionInference(const AbstractTheory* ltcTheo, const Structure* stateBefore);
	~ProgressionInference();

	std::vector<Structure*> progress();
	void postprocess(std::vector<Structure*>&);

};

