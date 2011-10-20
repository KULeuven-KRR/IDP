/************************************
	SimpleFuncGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SIMPLEFUNCGENERATOR_HPP_
#define SIMPLEFUNCGENERATOR_HPP_

#include <vector>
#include "generators/InstGenerator.hpp"

class FuncTable;
class DomElemContainer;
class DomainElement;
class Universe;

class SimpleFuncGenerator : public InstGenerator {
	private:
		const FuncTable*						_function;
		InstGenerator*							_univgen;
		std::vector<unsigned int>				_inposs;
		std::vector<unsigned int>				_outposs;
		std::vector<const DomElemContainer*>	_invars;
		const DomElemContainer*					_outvar;
		mutable std::vector<const DomainElement*>	_currinput;
		unsigned int							_check;
	public:
		SimpleFuncGenerator(const FuncTable*, const std::vector<bool>&, const std::vector<const DomElemContainer*>&, const Universe&, const std::vector<unsigned int>&);
		bool first()	const;
		bool next()		const;
};

#endif /* SIMPLEFUNCGENERATOR_HPP_ */
