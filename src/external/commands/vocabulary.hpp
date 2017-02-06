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

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"


class getSortNamesInference: public VocabularyBase {
public:
	getSortNamesInference()
			: VocabularyBase("getsortnames", "Returns a table with all the names of the non built-in sorts in the vocabulary.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		std::vector<InternalArgument>* ret = new std::vector<InternalArgument>();
		for (auto name2sort : get<0>(args)->getSorts()) {
			Sort* s = name2sort.second;
			if (s->builtin()) {
				continue;
			}
			std::string* strcopy = new std::string(s->name());
			InternalArgument toadd = InternalArgument(strcopy);
			ret->push_back(toadd);
		}
		return InternalArgument(ret);
	}
};

class getPredicateNamesInference: public VocabularyBase {
public:
	getPredicateNamesInference()
			: VocabularyBase("getpredicatenames", "Returns a table with all the names of the predicates in the vocabulary, excluding sort predicates and built-in predicates.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* invoc = get<0>(args);
		std::vector<InternalArgument>* ret = new std::vector<InternalArgument>();
		for (auto name2pred : invoc->getPreds()) {
			Predicate* p = name2pred.second;
			if (invoc->std()->hasPredWithName(p->name()) or invoc->hasSortWithName(p->nameNoArity())) {
				continue;
			}
			std::string* strcopy = new std::string(p->name());
			InternalArgument toadd = InternalArgument(strcopy);
			ret->push_back(toadd);
		}
		return InternalArgument(ret);
	}
};

class getFunctionNamesInference: public VocabularyBase {
public:
	getFunctionNamesInference()
			: VocabularyBase("getfunctionnames", "Returns a table with all the names of the non built-in functions in the vocabulary.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* invoc = get<0>(args);
		std::vector<InternalArgument>* ret = new std::vector<InternalArgument>();
		for (auto name2func : invoc->getFuncs()) {
			Function* f = name2func.second;
			if (invoc->std()->hasFuncWithName(f->name())) {
				continue;
			}
			std::string* strcopy = new std::string(f->name());
			InternalArgument toadd = InternalArgument(strcopy);
			ret->push_back(toadd);
		}
		return InternalArgument(ret);
	}
};

InternalArgument createInternalArgumentVector(const std::vector<Sort*> sorts) {
		std::vector<InternalArgument>* ret = new std::vector<InternalArgument>();
		for (auto sort : sorts) {
			std::string* strcopy = new std::string(sort->name());
			InternalArgument toadd = InternalArgument(strcopy);
			ret->push_back(toadd);
		}
		return InternalArgument(ret);
}

class getTypeNamesInference: public VocabularyStringBase {
public:
	getTypeNamesInference()
			: VocabularyStringBase("gettypesof", "Takes as input a vocabulary and a string representing the name/arity of a predicate or function in that vocabulary. Returns a table with the names of the sorts of the given symbol in the given vocabulary.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* invoc = get<0>(args);
		std::string* inname = get<1>(args);
		const std::vector<Sort*> sorts;
		if (invoc->hasPredWithName(*inname)) {
			Predicate* predicate = invoc->pred(*inname);
			return createInternalArgumentVector(predicate->sorts());
		} else if(invoc->hasFuncWithName(*inname)) {
			Function* function = invoc->func(*inname);
			return createInternalArgumentVector(function->sorts());
		} else {
			Warning::warning("Tried retrieving the sorts of a string that did not represent a predicate or function in the given vocabulary");
			return createInternalArgumentVector(std::vector<Sort*>());
		}
	}
};
