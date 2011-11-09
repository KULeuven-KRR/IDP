/************************************
 ConstTermExtractor.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef CONSTTERMEXTRACTOR_HPP_
#define CONSTTERMEXTRACTOR_HPP_

// FIXME use?
/**
 *Classes to add terms with the same non-constant factor
 */
class ConstTermExtractor: public FOBDDVisitor {
private:
	const FOBDDDomainTerm* _result;
public:
	ConstTermExtractor(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}
	const FOBDDDomainTerm* run(const FOBDDArgument* arg) {
		const DomainElement* d = createDomElem(1);
		_result = _manager->getDomainTerm(arg->sort(), d);
		arg->accept(this);
		return _result;
	}
	void visit(const FOBDDFuncTerm* ft) {
		if (typeid(*(ft->args(0))) == typeid(FOBDDDomainTerm)) {
			assert(typeid(*(ft->args(1))) != typeid(FOBDDDomainTerm));
			_result = dynamic_cast<const FOBDDDomainTerm*>(ft->args(0));
		}
	}
};

#endif /* CONSTTERMEXTRACTOR_HPP_ */
