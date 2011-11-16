/************************************
	SubstituteTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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

	std::map<const From*, const To*>& getFrom2To() {
		return _from2to;
	}
public:
	SubstituteTerms(FOBDDManager* manager, const std::map<const From*, const To*> map) :
			FOBDDVisitor(manager), _from2to(map) {
	}

	const FOBDDArgument* change(const From* v) {
		auto it = _from2to.find(v);
		if (it != _from2to.cend()) {
			return it->second;
		} else {
			return v;
		}
	}
};

class SubstituteIndex: public FOBDDVisitor {
private:
	const FOBDDDeBruijnIndex* _index;
	const FOBDDVariable* _variable;
public:
	SubstituteIndex(FOBDDManager* manager, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable) :
			FOBDDVisitor(manager), _index(index), _variable(variable) {
	}
	const FOBDDArgument* change(const FOBDDDeBruijnIndex* i) {
		if (i == _index) {
			return _variable;
		} else {
			return i;
		}
	}
	const FOBDDKernel* change(const FOBDDQuantKernel* k) {
		_index = _manager->getDeBruijnIndex(_index->sort(), _index->index() + 1);
		const FOBDD* nbdd = FOBDDVisitor::change(k->bdd());
		_index = _manager->getDeBruijnIndex(_index->sort(), _index->index() - 1);
		return _manager->getQuantKernel(k->sort(), nbdd);
	}
};

#endif /* SUBSTITUTE_HPP_ */
