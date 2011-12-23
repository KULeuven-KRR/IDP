/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef TWOVALUEDEXTENSIONSINFERENCE_HPP_
#define TWOVALUEDEXTENSIONSINFERENCE_HPP_

#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"

class TwoValuedExtensionsOfStructureInference: public StructureBase {
public:
	TwoValuedExtensionsOfStructureInference() :
		StructureBase("alltwovaluedextensions", "Generate all two-valued extensions of the given structure.") {
	}

	InternalArgument execute(const std::vector<InternalArgument> & args) const {
		auto result = new std::vector<InternalArgument>();
		auto s = get<0>(args);
		addAllMorePreciseToResult(s, result);
		addToGarbageCollection(result);
		return InternalArgument(result);
	}

	void addAllMorePreciseToResult(AbstractStructure *s, std::vector<InternalArgument> *& result) const {
		if (not s->approxTwoValued()) {
			auto extensions = generateAllTwoValuedExtensions(s);
			result->insert(result->end(), extensions.begin(), extensions.end());
			for(auto i=extensions.cbegin(); i<extensions.cend(); ++i){
				addToGarbageCollection(*i);
			}
		} else {
			result->push_back(s);
		}
	}
};

typedef TypedInference<LIST(std::vector<InternalArgument>*)> TwoValuedExtensionsOfTableInferenceBase;
class TwoValuedExtensionsOfTableInference: public TwoValuedExtensionsOfTableInferenceBase {
public:
	TwoValuedExtensionsOfTableInference() :
		TwoValuedExtensionsOfTableInferenceBase("alltwovaluedextensions", "Generate all two-valued extensions of all of the given structures.") {
	}

	InternalArgument execute(const std::vector<InternalArgument> & args) const {
		auto result = new std::vector<InternalArgument>();
		auto table = get<0>(args);
		for (auto it = table->cbegin(); it != table->cend(); ++it) {
			addAllMorePreciseToResult((*it).structure(), result);
		}
		addToGarbageCollection(result);
		return InternalArgument(result);
	}

	void addAllMorePreciseToResult(AbstractStructure *s, std::vector<InternalArgument> *& result) const {
		if (not s->approxTwoValued()) {
			auto extensions = generateAllTwoValuedExtensions(s);
			result->insert(result->end(), extensions.begin(), extensions.end());
			for(auto i=extensions.cbegin(); i<extensions.cend(); ++i){
				addToGarbageCollection(*i);
			}
		} else {
			result->push_back(s);
		}
	}
};

#endif /* TWOVALUEDEXTENSIONSINFERENCE_HPP_ */
