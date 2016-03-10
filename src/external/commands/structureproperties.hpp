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
#include "structure/StructureUtils.hpp"

class GetNbOfTwoValuedAtomsInStructure: public StructureBase {
	public:
	GetNbOfTwoValuedAtomsInStructure()
				: StructureBase("nrtwovaluedatoms", "Retrieve the number of two-valued atoms in the given structure.") {
			setNameSpace(getStructureNamespaceName());
		}

		InternalArgument execute(const std::vector<InternalArgument>& args) const {
			InternalArgument ia;
			ia._type = AT_DOUBLE;
			ia._value._double = toDouble(StructureUtils::getNbOfTwoValuedAtoms(get<0>(args)));
			return ia;
		}
};

class IsConsistentInference: public StructureBase {
public:
	IsConsistentInference()
			: StructureBase("isconsistent", "Check whether the structure is consistent.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		InternalArgument ia;
		ia._type = AT_BOOLEAN;
		ia._value._boolean = get<0>(args)->isConsistent();
		return ia;
	}
};

template<class Table>
bool checkEquality(Table one, Table two){
	if(one==two){
		return true;
	}
	if(one->size()!=two->size()){ // TODO infinity?
		return false;
	}
	for(auto i = one->begin(); not i.isAtEnd(); ++i){
		if(not two->contains(*i)){
			return false;
		}
	}
	return true;
}

typedef TypedInference<LIST(Structure*,Structure*)> SSBase;
class StructureEqualityInference: public SSBase {
public:
	StructureEqualityInference()
			: SSBase("equal", "Check whether two structures are equal.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Structure* s1 = get<0>(args);
		Structure* s2 = get<1>(args);

		auto equal = true;

		if(equal){
			equal &= s1!=NULL && s2!=NULL;
		}
		if(equal){
			equal &= s1->vocabulary()==s2->vocabulary();
		}
		if(equal){
			for(auto sort2inter : s1->getSortInters()){
				auto sort = sort2inter.first;
				auto inter = sort2inter.second;
				if(sort->builtin()){
					continue;
				}
				auto inter2 = s2->inter(sort);
				equal &= checkEquality(inter, inter2);
			}
			for(auto pred2inter : s1->getPredInters()){
				auto pred = pred2inter.first;
				auto inter = pred2inter.second;
				if(pred->builtin() || pred->overloaded()){
					continue;
				}
				auto inter2 = s2->inter(pred);
				equal &= inter->ct()->approxEqual(inter2->ct()) || checkEquality(inter->ct(), inter2->ct());
				equal &= inter->cf()->approxEqual(inter2->cf()) || checkEquality(inter->cf(), inter2->cf());
			}
			for(auto func2inter : s1->getFuncInters()){
				auto func = func2inter.first;
				auto inter = func2inter.second->graphInter();
				if(func->builtin() || func->overloaded()){
					continue;
				}
				auto inter2 = s2->inter(func)->graphInter();
				equal &= inter->ct()->approxEqual(inter2->ct()) || checkEquality(inter->ct(), inter2->ct());
				equal &= inter->cf()->approxEqual(inter2->cf()) || checkEquality(inter->cf(), inter2->cf());
			}
		}

		InternalArgument ia;
		ia._type = AT_BOOLEAN;
		ia._value._boolean = equal;
		return ia;
	}
};
