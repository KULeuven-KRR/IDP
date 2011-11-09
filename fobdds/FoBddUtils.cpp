
#include "fobdds/FoBddUtils.hpp"

#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/bddvisitors/ExtractFirstNonFuncTerm.hpp"

using namespace std;

const DomainElement* Addition::getNeutralElement() {
	return createDomElem(0);
}

bool Addition::operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2) {
	ExtractFirstNonFuncTerm extractor;
	auto arg1first = extractor.run(arg1);
	auto arg2first = extractor.run(arg2);

	if (arg1first == arg2first) {
		return arg1 < arg2;
	} else if (sametypeid<FOBDDDomainTerm>(*arg1first)) {
		if (sametypeid<FOBDDDomainTerm>(*arg2first)) {
			return arg1 < arg2;
		} else {
			return true;
		}
	} else if (sametypeid<FOBDDDomainTerm>(*arg2first)) {
		return false;
	} else {
		return arg1first < arg2first;
	}
}

const DomainElement* Multiplication::getNeutralElement() {
	return createDomElem(1);
}

// Ordering method: true if ordered before
// TODO comment and check what they do!
bool Multiplication::operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2) {
	if (sametypeid<FOBDDDomainTerm>(*arg1)) {
		if (sametypeid<FOBDDDomainTerm>(*arg2)) {
			return arg1 < arg2;
		} else {
			return true;
		}
	} else if (sametypeid<FOBDDDomainTerm>(*arg2)) {
		return false;
	} else {
		return arg1 < arg2;
	}
}
