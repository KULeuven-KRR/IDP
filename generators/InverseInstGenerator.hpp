/************************************
	InverseInstGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INVERSEINSTGENERATOR_HPP_
#define INVERSEINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class InverseInstGenerator : public InstGenerator {
	private:
		std::vector<const DomElemContainer*>	_univvars;
		std::vector<const DomElemContainer*>	_outtabvars;
		InstGenerator*					_univgen;
		InstGenerator*					_outtablegen;
		mutable bool					_outend;
		bool outIsSmaller() const;
		bool univIsSmaller() const;
	public:
		InverseInstGenerator(PredTable* t, const std::vector<bool>&, const std::vector<const DomElemContainer*>&);
		bool first() const;
		bool next() const;
};


#endif /* INVERSEINSTGENERATOR_HPP_ */
