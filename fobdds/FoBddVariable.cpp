#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "vocabulary.hpp"

using namespace std;

Sort* FOBDDVariable::sort() const {
	return _variable->sort();
}

Sort* FOBDDFuncTerm::sort() const {
	return _function->outsort();
}
