/************************************
	GenerateAndTestGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GENERATEANDTESTGENERATOR_HPP_
#define GENERATEANDTESTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class GenerateAndTestGenerator : public InstGenerator {
	private:
		const PredTable*							_table;
		PredTable*									_full;
		std::vector<const DomElemContainer*>				_invars;
		std::vector<const DomElemContainer*>				_outvars;
		std::vector<unsigned int>						_inposs;
		std::vector<unsigned int>						_outposs;
		std::vector<unsigned int>						_firstocc;
		mutable TableIterator						_currpos;
		mutable ElementTuple						_currtuple;
	public:
		GenerateAndTestGenerator(const PredTable*,const std::vector<bool>&, const std::vector<const DomElemContainer*>&, const std::vector<unsigned int>&, const Universe& univ);
		bool first()	const;
		bool next()		const;
};

#endif /* GENERATEANDTESTGENERATOR_HPP_ */
