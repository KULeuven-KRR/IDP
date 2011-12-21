/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef PRINTDOMAINATOM_HPP_
#define PRINTDOMAINATOM_HPP_

#include "commandinterface.hpp"
#include "structure.hpp"

class PrintDomainAtomInference: public TypedInference<LIST(const DomainAtom*)> {
public:
	PrintDomainAtomInference(): TypedInference("tostring", "Prints a domainatom") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto atom = get<0>(args);
		std::stringstream sstr;
		atom->symbol()->put(sstr);
		if(typeid(*(atom->symbol())) == typeid(Predicate)) {
			if(!atom->symbol()->sorts().empty()) {
				sstr << '(' << *(atom->args()[0]);
				for(unsigned int n = 1; n < atom->args().size(); ++n) {
					sstr << ',' << *(atom->args()[n]);
				}
				sstr << ')';
			}
		}
		else {
			if(atom->symbol()->sorts().size() > 1) {
				sstr << '(' << *(atom->args()[0]);
				for(unsigned int n = 1; n < atom->args().size()-1; ++n) {
					sstr << ',' << *(atom->args()[n]);
				}
				sstr << ')';
			}
			sstr << " = " << *(atom->args().back());
		}
		return InternalArgument(StringPointer(sstr.str()));
	}
};

#endif /* PRINTDOMAINATOM_HPP_ */
