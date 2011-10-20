/************************************
	EmptyGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EMPTYGENERATOR_HPP_
#define EMPTYGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class EmptyGenerator : public InstGenerator {
	public:
		bool first()	const { return false;	}
		bool next()		const { return false;	}
};

#endif /* EMPTYGENERATOR_HPP_ */
