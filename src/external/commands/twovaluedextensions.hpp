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

#ifndef TWOVALUEDEXTENSIONSINFERENCE_HPP_
#define TWOVALUEDEXTENSIONSINFERENCE_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

class TwoValuedExtensionsOfStructureInference: public StructureBase {
public:
	TwoValuedExtensionsOfStructureInference()
			: StructureBase("nbModelsTwoValuedExtensions", "Generate all two-valued extensions of the given structure.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument> & args) const {
		auto structure = get<0>(args);
		auto result = generateEnoughTwoValuedExtensions({structure});
		auto iaresult = new std::vector<InternalArgument>();
		for(auto i=result.cbegin(); i<result.cend(); ++i){
			// TODO
			//			for(auto i=extensions.cbegin(); i<extensions.cend(); ++i){
			//				addToGarbageCollection(*i); (lua?)
			//			}
			iaresult->push_back(InternalArgument(*i));
		}
		addToGarbageCollection(iaresult);
		return InternalArgument(iaresult);
	}
};

typedef TypedInference<LIST(std::vector<InternalArgument>*)> TwoValuedExtensionsOfTableInferenceBase;
class TwoValuedExtensionsOfTableInference: public TwoValuedExtensionsOfTableInferenceBase {
public:
	TwoValuedExtensionsOfTableInference()
			: TwoValuedExtensionsOfTableInferenceBase("nbModelsTwoValuedExtensions", "Generate all two-valued extensions of all of the given structures.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto table = get<0>(args);
		std::vector<Structure*> structures;
		for(auto i=table->cbegin(); i<table->cend(); ++i){
			structures.push_back((*i).structure());
		}
		auto result = generateEnoughTwoValuedExtensions(structures);
		auto iaresult = new std::vector<InternalArgument>();
		for(auto i=result.cbegin(); i<result.cend(); ++i){
			// TODO
			//			for(auto i=extensions.cbegin(); i<extensions.cend(); ++i){
			//				addToGarbageCollection(*i); (lua?)
			//			}
			iaresult->push_back(InternalArgument(*i));
		}
		addToGarbageCollection(iaresult);
		return InternalArgument(iaresult);
	}
};

#endif /* TWOVALUEDEXTENSIONSINFERENCE_HPP_ */
