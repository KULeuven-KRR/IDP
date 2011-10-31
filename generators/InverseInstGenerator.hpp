/************************************
 InverseInstGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef INVERSEINSTGENERATOR_HPP_
#define INVERSEINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include "generators/LookupGenerator.hpp"

/**
 * Given a predicate table, generate tuples which, given the input, are not in the predicate table
 */
// TODO optimize generators if we can guarantee that sorts are always traversed from smallest to largest. This leads to problems as it might be easier to iterate
// over N as 0 1 -1 2 -2 to avoid having to start at -infinity
// TODO Code below is correct, but can be optimized in case there are output variables that occur more than once in 'vars'
// TODO can probably be optimized a lot if with the order in which we run over it.
class InverseInstGenerator: public InstGenerator {
private:
	PredTable* _table;
	InstGenerator *_universegen;
	InstChecker *_predchecker;
	std::vector<const DomElemContainer*> _outvars, _copiedoutvars;

public:
	InverseInstGenerator(PredTable* t, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars)
			: _table(t){

		auto tempvars = vars;
		for(auto i=0; i<pattern.size(); ++i){
			if(pattern[i]==Pattern::OUTPUT){
				tempvars[i] = new DomElemContainer();
				_outvars.push_back(vars[i]);
				_copiedoutvars.push_back(tempvars[i]);
			}
		}
		PredTable temp(new FullInternalPredTable(), t->universe());
		_universegen = GeneratorFactory::create(&temp, pattern, tempvars, t->universe()); // Use tempvars so we can safely iterate over then and later only have to set the output
		_predchecker = new LookupGenerator(t, tempvars, t->universe());
	}

	bool check() const{
		assert(_outvars.size()==0);
		return not _predchecker->check();
	}

	void reset(){
		_universegen->begin();
		while(not _universegen->isAtEnd()){
			if(not _predchecker->check()){ // It is NOT a tuple in the table
				break;
			}
		}
		if(_universegen->isAtEnd()){
			notifyAtEnd();
		}
	}

	void next(){
		for(auto i=0; i<_copiedoutvars.size(); ++i){
			_outvars[i]->operator =(_copiedoutvars[i]);
		}
		while(not _universegen->isAtEnd()){
			if(not _predchecker->check()){ // It is NOT a tuple in the table
				break;
			}
		}
		if(_universegen->isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* INVERSEINSTGENERATOR_HPP_ */
