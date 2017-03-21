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


std::string* nameOfPFSymbol(PFSymbol* s) {
	std::stringstream ss;
	ss << s->nameNoArity();
	return new std::string(ss.str());
}

std::vector<std::set<Sort*>*>* sortsOfPFSymbol(PFSymbol* s) {
	auto allsorts = s->sorts();
	auto ret = new std::vector<std::set<Sort*>*>();
	for (auto it = allsorts.begin(); it != allsorts.end(); it++) {
		ret->push_back(new std::set<Sort*>({*it}));
	}
	return ret;
}

class getSortsInference: public VocabularyBase {
public:
	getSortsInference()
			: VocabularyBase("gettypes", "Returns a table with all non built-in types in the vocabulary.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto sortsvector = new std::vector<std::set<Sort*>*>();
		for (auto name2sort : get<0>(args)->getSorts()) {
			Sort* s = name2sort.second;
			if (s->builtin()) {
				continue;
			}
			sortsvector->push_back(new std::set<Sort*>({s}));
		}
		return InternalArgument(sortsvector);
	}
};

class getSortNameInference: public SortBase {
public:
	getSortNameInference()
			: SortBase("name", "Returns the name of a given type.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Sort* s = get<0>(args);
		if (s == NULL) {
			Warning::warning("Type was expected as argument, but something else was given.");
			return nilarg();
		} else {
			std::stringstream ss;
			ss << s->name();
			return InternalArgument(new std::string(ss.str()));
		}
	}
};

class getPredicatesInference: public VocabularyBase {
public:
	getPredicatesInference()
			: VocabularyBase("getpredicates", "Returns a table with all the predicate symbols in the vocabulary, excluding type predicates and built-in predicates.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* invoc = get<0>(args);
		auto predvector = new std::vector<std::set<Predicate*>*>();
		for (auto symbol : invoc->getNonBuiltinNonOverloadedSymbols()) {
			if (not symbol->isPredicate()) { 
				continue; 
			}
			Predicate* p = dynamic_cast<Predicate*>(symbol);
			if (PredUtils::isTypePredicate(p)) {
				continue;
			}
			predvector->push_back(new std::set<Predicate*>({p}));
		}
		return InternalArgument(predvector);
	}
};

class getPredicateNameInference: public PredicateBase {
public:
	getPredicateNameInference()
			: PredicateBase("name", "Returns the name of a given predicate symbol.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Predicate* pred = get<0>(args);
		if (pred == NULL) {
			Warning::warning("Predicate symbol was expected as argument, but something else was given.");
			return nilarg();
		} else {
			return InternalArgument(nameOfPFSymbol(pred));
		}
	}
};

class getPredicateSortsInference: public PredicateBase {
public:
	getPredicateSortsInference()
			: PredicateBase("gettyping", "Returns a table containing, in-order, the types of a given predicate symbol.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Predicate* pred = get<0>(args);
		if (pred == NULL) {
			Warning::warning("Predicate symbol was expected as argument, but something else was given.");
			return nilarg();
		} else {
			return InternalArgument(sortsOfPFSymbol(pred));
		}
	}
};

class getFunctionsInference: public VocabularyBase {
public:
	getFunctionsInference()
			: VocabularyBase("getfunctions", "Returns a table with all non built-in function symbols in the vocabulary.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* invoc = get<0>(args);
		auto funcvector = new std::vector<std::set<Function*>*>();
		for (auto symbol : invoc->getNonBuiltinNonOverloadedSymbols()) {
			if (not symbol->isFunction()) { 
				continue; 
			}
			Function* func = dynamic_cast<Function*>(symbol);
			funcvector->push_back(new std::set<Function*>({func}));
		}
		return InternalArgument(funcvector);
	}
};

class getFunctionNameInference: public FunctionBase {
public:
	getFunctionNameInference()
			: FunctionBase("name", "Returns the name of a given function symbol.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Function* func = get<0>(args);
		if (func == NULL) {
			Warning::warning("Function symbol was expected as argument, but something else was given.");
			return nilarg();
		} else {
			return InternalArgument(nameOfPFSymbol(func));
		}
	}
};

class getFunctionSortsInference: public FunctionBase {
public:
	getFunctionSortsInference()
			: FunctionBase("gettyping", "Returns a table containing, in-order, the types of a given function symbol.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Function* func = get<0>(args);
		if (func == NULL) {
			Warning::warning("Function symbol was expected as argument, but something else was given.");
			return nilarg();
		} else {
			return InternalArgument(sortsOfPFSymbol(func));
		}
	}
};
