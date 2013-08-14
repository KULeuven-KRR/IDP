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

#include "fobdds/FoBddUtils.hpp"

#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/bddvisitors/FirstNonConstMultTerm.hpp"

#include "IncludeComponents.hpp"

using namespace std;

const DomainElement* Addition::getNeutralElement() {
	return createDomElem(0);
}

bool Addition::operator()(const FOBDDTerm* arg1, const FOBDDTerm* arg2) {
	FirstNonConstMultTerm extractor(manager);
	auto arg1first = extractor.run(arg1);
	auto arg2first = extractor.run(arg2);

	if (arg1first == arg2first) {
		return arg1->before(arg2);
	} else if (isBddDomainTerm(arg1first)) {
		if (isBddDomainTerm(arg2first)) {
			return arg1->before(arg2);
		} else {
			return true;
		}
	} else if (isBddDomainTerm(arg2first)) {
		return false;
	} else {
		return arg1first->before(arg2first);
	}
}

const DomainElement* Multiplication::getNeutralElement() {
	return createDomElem(1);
}

// Ordering method: true if ordered before
bool Multiplication::operator()(const FOBDDTerm* arg1, const FOBDDTerm* arg2) {
	if (isBddDomainTerm(arg1)) {
		if (isBddDomainTerm(arg2)) {
			return arg1->before(arg2);
		} else {
			return true;
		}
	} else if (isBddDomainTerm(arg2)) {
		return false;
	} else {
		return arg1->before(arg2);
	}
}

#include "fobdds/bddvisitors/CollectSameOperationTerms.hpp"

bool TermOrder::before(const FOBDDTerm* arg1, const FOBDDTerm* arg2, std::shared_ptr<FOBDDManager> manager) {
	CollectSameOperationTerms<Multiplication> fa(manager);
	auto flat1 = fa.getTerms(arg1);
	auto flat2 = fa.getTerms(arg2);
	if (flat1.size() < flat2.size()) {
		return true;
	} else if (flat1.size() > flat2.size()) {
		return false;
	} else {
		for (size_t n = 1; n < flat1.size(); ++n) {
			if (Multiplication::before(flat1[n], flat2[n], manager)) {
				return true;
			} else if (Multiplication::before(flat2[n], flat1[n], manager)) {
				return false;
			}
		}
		return false;
	}
}

varset getFOVariables(const fobddvarset& vars){
	varset list;
	for(auto var: vars){
		list.insert(var->variable());
	}
	return list;
}
