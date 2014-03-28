/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

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
	SubstituteTerms(std::shared_ptr<FOBDDManager> manager, const std::map<const From*, const To*> map)
			: FOBDDVisitor(manager), _from2to(map) {
	}

	const FOBDDTerm* change(const From* v) {
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
	SubstituteIndex(std::shared_ptr<FOBDDManager> manager, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable)
			: FOBDDVisitor(manager), _index(index), _variable(variable) {
	}
	const FOBDDTerm* change(const FOBDDDeBruijnIndex* i) {
		if (i == _index) {
			return _variable;
		} else if(i->index()>_index->index()){
			return _manager->getDeBruijnIndex(i->sort(), i->index()-1);
		}else{
			return i;
		}
	}
	const FOBDDKernel* change(const FOBDDQuantKernel* k) {
		_index = _manager->getDeBruijnIndex(_index->sort(), _index->index() + 1);
		const FOBDD* nbdd = FOBDDVisitor::change(k->bdd());
		_index = _manager->getDeBruijnIndex(_index->sort(), _index->index() - 1);
		return _manager->getQuantKernel(k->sort(), nbdd);
	}

	const FOBDDQuantSetExpr* change (const FOBDDQuantSetExpr* qse){
		_index = _manager->getDeBruijnIndex(_index->sort(), _index->index() + qse->quantvarsorts().size());
		auto newset = FOBDDVisitor::change(qse);
		_index = _manager->getDeBruijnIndex(_index->sort(), _index->index() - qse->quantvarsorts().size());
		return newset;
	}
};

#endif /* SUBSTITUTE_HPP_ */
