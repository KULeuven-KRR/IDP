#ifndef ARGCHECKER_HPP_
#define ARGCHECKER_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddTerm.hpp"

class TermOccursNested: public FOBDDVisitor {
private:
	bool _result;
	const FOBDDArgument* _arg;

	void visit(const FOBDDVariable* var) {
		if (var == _arg) {
			_result = true;
		}
	}
	void visit(const FOBDDDeBruijnIndex* index) {
		if (index == _arg) {
			_result = true;
		}
	}
	void visit(const FOBDDDomainTerm* dt) {
		if (dt == _arg) {
			_result = true;
		}
	}
	void visit(const FOBDDFuncTerm* ft) {
		if (ft == _arg) {
			_result = true;
		} else {
			for (auto it = ft->args().cbegin(); it != ft->args().cend(); ++it) {
				(*it)->accept(this);
				if (_result) {
					break;
				}
			}
		}
	}
public:
	TermOccursNested(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}
	bool termHasSubterm(const FOBDDArgument* super, const FOBDDArgument* arg) {
		_result = false;
		_arg = arg;
		super->accept(this);
		return _result;
	}
};

#endif /* ARGCHECKER_HPP_ */
