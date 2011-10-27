/************************************
	EnumLookupGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ENUMLOOKUPGENERATOR_HPP_
#define ENUMLOOKUPGENERATOR_HPP_

#include <map>
#include "generators/InstGenerator.hpp"

typedef std::map<ElementTuple,std::vector<ElementTuple>,Compare<ElementTuple> > LookupTable;

/**
 * Given a map from tuples to a list of tuples, with given input variables and output variables, go over the list of tuples of the corresponding input tuple.
 */
class EnumLookupGenerator : public InstGenerator {
private:
	const LookupTable&						_table;
	std::vector<const DomElemContainer*>	_invars;
	std::vector<const DomElemContainer*>	_outvars;
	LookupTable::const_iterator				_currpos;
	std::vector<std::vector<const DomainElement*> >::const_iterator	_iter;
public:
	EnumLookupGenerator(const LookupTable& t, const std::vector<const DomElemContainer*>& in, const std::vector<const DomElemContainer*>& out)
			: _table(t), _invars(in), _outvars(out) {
	}

	void reset(){
		std::vector<const DomainElement*> _currargs;
		for(unsigned int n = 0; n < _invars.size(); ++n) {
			_currargs[n] = _invars[n]->get();
		}
		_currpos = _table.find(_currargs);
		if(_currpos == _table.cend() || _currpos->second.size()==0){
			notifyAtEnd();
		}
		_iter = _currpos->second.cbegin();
	}

	void next(){
		_iter = _currpos->second.cbegin();
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
		}
		++_iter;
		if(_iter == _currpos->second.cend()){
			notifyAtEnd();
		}
	}
};


#endif /* ENUMLOOKUPGENERATOR_HPP_ */
