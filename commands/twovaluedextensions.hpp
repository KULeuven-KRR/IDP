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

#include <vector>
#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"

class TwoValuedExtensionsInference: public Inference {
private:
	bool _typeIsTable;
public:
	TwoValuedExtensionsInference() :
			Inference("alltwovaluedextensions"), _typeIsTable(false) {
		add(AT_STRUCTURE);
	}
	TwoValuedExtensionsInference(ArgType type) :
			Inference("alltwovaluedextensions"), _typeIsTable(type == ArgType::AT_TABLE) {
		Assert(type==ArgType::AT_STRUCTURE||type == ArgType::AT_TABLE);
		add(type);
	}

	void addAllMorePreciseToResult(AbstractStructure *s, std::vector<InternalArgument> *& result) const {
		if (!s->approxTwoValued()) {
			auto extensions = s->generateAllTwoValuedExtensions();
			result->insert(result->end(), extensions.begin(), extensions.end());

		} else {
			result->push_back(s);
		}
	}

	InternalArgument execute(const std::vector<InternalArgument> & args) const {
		std::vector<InternalArgument> *result = new std::vector<InternalArgument>();

		if (not _typeIsTable) {
			AbstractStructure *s = args[0].structure();
			addAllMorePreciseToResult(s, result);
		} else {
			std::vector<InternalArgument>* table = args[0]._value._table;
			for (auto it = table->cbegin(); it != table->cend(); ++it) {
				addAllMorePreciseToResult((*it).structure(), result);
			}
		}
		return InternalArgument(result);

	}
};

#endif /* TWOVALUEDEXTENSIONSINFERENCE_HPP_ */
