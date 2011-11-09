#ifndef FOBDDFUNCTERM_HPP_
#define FOBDDFUNCTERM_HPP_

#include "fobdds/FoBddTerm.hpp"

class Function;

class FOBDDFuncTerm: public FOBDDArgument {
private:
	Function* _function;
	std::vector<const FOBDDArgument*> _args;

	FOBDDFuncTerm(Function* func, const std::vector<const FOBDDArgument*>& args) :
			_function(func), _args(args) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const;

	Function* func() const {
		return _function;
	}
	const FOBDDArgument* args(unsigned int n) const {
		return _args[n];
	}
	const std::vector<const FOBDDArgument*>& args() const {
		return _args;
	}
	Sort* sort() const;

	void accept(FOBDDVisitor*) const;
	const FOBDDArgument* acceptchange(FOBDDVisitor*) const;

	friend class FOBDDManager;
};

#endif /* FOBDDFUNCTERM_HPP_ */
