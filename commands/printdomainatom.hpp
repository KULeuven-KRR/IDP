/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTDOMAINATOM_HPP_
#define PRINTDOMAINATOM_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"
#include "options.hpp"

class PrintDomainAtomInference: public Inference {
public:
	PrintDomainAtomInference(): Inference("tostring") {
		add(AT_DOMAINATOM);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		const DomainAtom* atom = args[0]._value._domainatom;
		Options* opts = args[1].options();
		std::stringstream sstr;
		atom->symbol()->put(sstr,opts->longnames());
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
