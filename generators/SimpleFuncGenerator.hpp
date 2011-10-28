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
public:
	// FIXME did not understand code
	SimpleFuncGenerator(const FuncTable* ft, const std::vector<bool>& pattern, const std::vector<const DomElemContainer*>& vars, const Universe& univ)
			: _function(ft) {
		_invars = vars; _invars.pop_back();
		_outvar = vars.back();
		std::vector<const DomElemContainer*> univvars;
		std::vector<SortTable*> univtabs;
		for(unsigned int n = 0; n < pattern.size() - 1; ++n) {
			if(pattern[n]) {
				_inposs.push_back(n);
			}
			else {
				_outposs.push_back(n);
				if(firstocc[n] == n) {
					univvars.push_back(vars[n]);
					univtabs.push_back(univ.tables()[n]);
				}
			}
		}
		GeneratorFactory gf;
		_univgen = gf.create(univvars,univtabs);
	}

	void reset(){
		std::vector<const DomElemContainer*> univvars;
		std::vector<SortTable*> univtabs;
		for(unsigned int n = 0; n < pattern.size() - 1; ++n) {
			if(pattern[n]) {
				_inposs.push_back(n);
			}
			else {
				_outposs.push_back(n);
				if(firstocc[n] == n) {
					univvars.push_back(vars[n]);
					univtabs.push_back(univ.tables()[n]);
				}
			}
		}
		GeneratorFactory gf;
		_univgen = gf.create(univvars,univtabs);
	}

	bool first()	const{
		if(_univgen->first()) {
			for(unsigned int n = 0; n < _inposs.size(); ++n) {
				_currinput[_inposs[n]] = _invars[_inposs[n]]->get();
			}
			for(unsigned int n = 0; n < _outposs.size(); ++n) {
				_currinput[_outposs[n]] = _invars[_outposs[n]]->get();
			}
			const DomainElement* d = _function->operator[](_currinput);
			if(d) {
				if(_check == _invars.size()) {
					*_outvar = d;
					return true;
				}
				else if(_invars[_check]->get() == d) {
					return true;
				}
				else { return next(); }
			}
			else { return next(); }
		}
		else return false;
	}
	bool next()		const{
		while(_univgen->next()) {
			for(unsigned int n = 0; n < _outposs.size(); ++n) {
				_currinput[_outposs[n]] = _invars[_outposs[n]]->get();
			}
			const DomainElement* d = _function->operator[](_currinput);
			if(d) {
				if(_check == _invars.size()) {
					*_outvar = d;
					return true;
				}
				else if(_invars[_check]->get() == d) {
					return true;
				}
			}
		}
		return false;
	}
};

#endif /* SIMPLEFUNCGENERATOR_HPP_ */
