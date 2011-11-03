/************************************
 InverseInstGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef INVERSEINSTGENERATOR_HPP_
#define INVERSEINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include "generators/LookupGenerator.hpp"
#include "generators/GeneratorFactory.hpp"

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
	std::vector<const DomElemContainer*> _outvars, _copiedoutvars, fulltempvars;
	bool _reset;

public:
	InverseInstGenerator(PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars)
			: _table(table), _reset(true){
		// NOTE: Use tempvars so we can safely iterate over then and later only have to set the output
		std::vector<const DomElemContainer*> tempvars;
		std::vector<SortTable*> temptables;
		for(auto i=0; i<pattern.size(); ++i){
			if(pattern[i]==Pattern::OUTPUT){
				tempvars.push_back(new DomElemContainer());
				fulltempvars.push_back(tempvars.back());
				temptables.push_back(table->universe().tables()[i]);
				_outvars.push_back(vars[i]);
				_copiedoutvars.push_back(tempvars.back());
			}else{
				fulltempvars.push_back(vars[i]);
			}
		}
		Universe universe(temptables);
		PredTable temp(new FullInternalPredTable(), universe);
		_universegen = GeneratorFactory::create(&temp, pattern, tempvars, universe);
		_predchecker = new LookupGenerator(table, fulltempvars, table->universe());
	}

	void reset(){
		_reset = true;
	}

	void next(){
		if(_reset){
			_reset = false;
			_universegen->begin();
		}else{
			_universegen->operator ++();
		}

		for(; not _universegen->isAtEnd(); _universegen->operator ++()){

			if(not _predchecker->check()){ // It is NOT a tuple in the table
				for(auto i=0; i<_copiedoutvars.size(); ++i){
					_outvars[i]->operator =(_copiedoutvars[i]);
				}
			}
		}
		if(_universegen->isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* INVERSEINSTGENERATOR_HPP_ */
