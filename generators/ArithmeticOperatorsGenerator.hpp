/************************************
	PlusGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PLUSGENERATOR_HPP_
#define PLUSGENERATOR_HPP_

#include <cassert>
#include "generators/InstGenerator.hpp"

enum class ARITHRESULT { VALID, INVALID};

/**
 * Instance generator for the formula x op y = z
 * where x and y are input and numeric and z is the output.
 */
class ArithOpGenerator : public InstGenerator {
private:
	const DomElemContainer*	_in1;
	const DomElemContainer*	_in2;
	const DomElemContainer*	_out;
	SortTable*				_outdom;
	bool 					alreadyrun;
protected:
	virtual ARITHRESULT doCalculation(double left, double right, double& result) const = 0;
public:
	ArithOpGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, SortTable* dom) :
		_in1(in1), _in2(in2), _out(out), _outdom(dom), alreadyrun(false) {
	}

	void reset(){
		alreadyrun = false;
	}

	void next() {
		if(alreadyrun){
			notifyAtEnd();
			return;
		}
		const DomainElement* left = _in1->get();
		const DomainElement* right = _in2->get();
		assert(left->type()==right->type());

		DomainElementType type = left->type();
		assert(type==DET_INT || type==DET_DOUBLE);

		double result;
		ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
		if(status!=ARITHRESULT::VALID){
			notifyAtEnd();
			return;
		}
		*_out = DomainElementFactory::instance()->create(type==DET_INT?int(result):result);
		if(not _outdom->contains(_out->get())){
			notifyAtEnd();
		}
		alreadyrun = true;
	}

	bool check() const {
		double result;
		ARITHRESULT status = doCalculation(getValue(_in1), getValue(_in2), result);
		return status==ARITHRESULT::VALID && result==getValue(_out);
	}

private:
	double getValue(const DomElemContainer* cont) const{
		auto domelem = cont->get();
		assert(domelem->type()==DET_DOUBLE || domelem->type()==DET_INT);
		if(domelem->type==DET_DOUBLE){
			return domelem->value()._double;
		}else{
			return domelem->value()._int;
		}
	}

};

// FIXME handle overflows
class DivGenerator : public ArithOpGenerator {
protected:
	double doCalculation(double left, double right, double& result) const {
		if(right == 0){ // cannot divide by zero
			return ARITHRESULT::INVALID;
		}
		result = left/right;
		return ARITHRESULT::VALID;
	};
public:
	DivGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, SortTable* dom) :
		ArithOpGenerator(in1, in2, out, dom) {
	}
};

class TimesGenerator : public ArithOpGenerator {
protected:
	double doCalculation(double left, double right, double& result) const {
		result = left * right;
		return ARITHRESULT::VALID;
	};
public:
	TimesGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, SortTable* dom) :
		ArithOpGenerator(in1, in2, out, dom) {
	}
};

class MinusGenerator : public ArithOpGenerator {
protected:
	double doCalculation(double left, double right, double& result) const {
		result = left - right;
		return ARITHRESULT::VALID;
	};
public:
	MinusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, SortTable* dom) :
		ArithOpGenerator(in1, in2, out, dom) {
	}
};

class PlusGenerator : public ArithOpGenerator {
protected:
	double doCalculation(double left, double right, double& result) const {
		result = left + right;
		return ARITHRESULT::VALID;
	};
public:
	PlusGenerator(const DomElemContainer* in1, const DomElemContainer* in2, const DomElemContainer* out, SortTable* dom) :
		ArithOpGenerator(in1, in2, out, dom) {
	}
};

#endif /* PLUSGENERATOR_HPP_ */
