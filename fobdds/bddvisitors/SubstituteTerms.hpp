#ifndef SUBSTITUTE_HPP_
#define SUBSTITUTE_HPP_

#include <map>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddIndex.hpp"

template<typename From, typename To>
class SubstituteTerms: public FOBDDVisitor {
protected:
	std::map<const From*, const To*> _from2to;

	std::map<const From*, const To*>& getFrom2To(){
		return _from2to;
	}
public:
	SubstituteTerms(FOBDDManager* manager, const std::map<const From*, const To*> map) :
			FOBDDVisitor(manager), _from2to(map) {
	}

	const FOBDDVariable* change(const From* v) {
		auto it = _from2to.find(v);
		if (it != _from2to.cend()) {
			return it->second;
		} else {
			return v;
		}
	}
};

template<typename To>
class SubstituteIndices: public SubstituteTerms<FOBDDDeBruijnIndex, To> {
	typedef SubstituteTerms<FOBDDDeBruijnIndex, To> Parent;
public:
	SubstituteIndices(FOBDDManager* manager, const std::map<const FOBDDDeBruijnIndex*, const To*> map) :
			Parent(manager, map) {
	}

	const FOBDDKernel* change(const FOBDDQuantKernel* k) {
		auto kernel = k;
		for(auto i=Parent::getFrom2To().begin(); i<Parent::getFrom2To().end(); ++i){
			(*i).first = Parent::_manager->getDeBruijnIndex((*i).first->sort(), (*i).first->index() + 1);
			const FOBDD* nbdd = FOBDDVisitor::change(kernel->bdd());
			(*i).first = Parent::_manager->getDeBruijnIndex((*i).first->sort(), (*i).first->index() - 1);
			kernel = Parent::_manager->getQuantKernel(kernel->sort(), nbdd);
		}
		return kernel;
	}
};

#endif /* SUBSTITUTE_HPP_ */
