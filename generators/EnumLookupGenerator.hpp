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
	LookupTable								_table;
	LookupTable::const_iterator				_currpos;
	std::vector<std::vector<const DomainElement*> >::const_iterator	_iter;
	std::vector<const DomElemContainer*>	_invars, _outvars;
	bool _reset;
public:
	EnumLookupGenerator(const LookupTable& t, const std::vector<const DomElemContainer*>& in, const std::vector<const DomElemContainer*>& out)
			: _table(t), _invars(in), _outvars(out), _reset(true) {
#ifdef DEBUG
		for(auto i=_table.cbegin(); i!=_table.cend(); ++i){
			for(auto j=(*i).second.cbegin(); j<(*i).second.cend(); ++j){
				Assert((*j).size()==out.size());
			}
		}
		for(auto i=in.cbegin(); i<in.cend(); ++i){
			Assert(*i != NULL);
		}
		for(auto i=out.cbegin(); i<out.cend(); ++i){
			Assert(*i != NULL);
		}
#endif
	}

	EnumLookupGenerator* clone() const{
		return new EnumLookupGenerator(*this);
	}

	void reset(){
		_reset = true;
	}

	// Increment is done AFTER returning a tuple!
	void next(){
		if(_reset){
			_reset = false;
			std::vector<const DomainElement*> _currargs;
			for(auto i=_invars.cbegin(); i<_invars.cend(); ++i) {
				_currargs.push_back((*i)->get());
			}
			_currpos = _table.find(_currargs);
			if(_currpos == _table.cend() || _currpos->second.size()==0){
				notifyAtEnd();
				return;
			}
			_iter = _currpos->second.cbegin();
		}else{
			if(_iter == _currpos->second.cend()){
				notifyAtEnd();
				return;
			}
		}
		Assert(_iter!=_currpos->second.cend());
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
		}
		++_iter;
	}
};


#endif /* ENUMLOOKUPGENERATOR_HPP_ */
