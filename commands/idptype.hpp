/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef IDPTYPE_HPP_
#define IDPTYPE_HPP_

#include <vector>
#include "commandinterface.hpp"

class IdpTypeInference: public Inference {
public:
	IdpTypeInference(): Inference("idpType") {
		add(AT_INT);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		ArgType tp = (ArgType)args[0]._value._int;
		return InternalArgument(StringPointer(toCString(tp)));
	}
};

#endif /* IDPTYPE_HPP_ */
