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
		for (auto symbol : invoc->getNonBuiltinNonOverloadedSymbols()) {
			if (not symbol->isPredicate()) { 
				continue; 
			}
			Predicate* p = dynamic_cast<Predicate*>(symbol);
			if (PredUtils::isTypePredicate(p)) {
				continue;
			}
			std::stringstream ss;
			ss << p->nameNoArity() << "(";
			for (int i = 0; i < p->sorts().size(); i++) {
				ss << p->sorts()[i]->name();
				if (i < p->sorts().size() - 1) {
					ss << ",";
				}
			}
			ss << ")";
			std::string* strcopy = new std::string(ss.str());
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
		for (auto symbol : invoc->getNonBuiltinNonOverloadedSymbols()) {
			if (not symbol->isFunction()) { 
				continue; 
			}
			Function* f = dynamic_cast<Function*>(symbol);
			std::stringstream ss;
			ss << f->nameNoArity() << "(";
			for (int i = 0; i < f->sorts().size() - 1; i++) {
				ss << f->sorts()[i]->name();
				if (i < f->sorts().size() -2) {
					ss << ",";
				}
			}
			ss << "):" << f->sorts().back()->name();
			std::string* strcopy = new std::string(ss.str());
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
