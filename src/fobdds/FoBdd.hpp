/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDD_HPP_
#define FOBDD_HPP_

#include <iostream>

class FOBDDKernel;
class FOBDDManager;
class FOBDDVisitor;

class FOBDD {
private:
	friend class FOBDDManager;
	friend class TrueFOBDD;
	friend class FalseFOBDD;

	const FOBDDKernel* _kernel;
	const FOBDD* _truebranch;
	const FOBDD* _falsebranch;

	void replacefalse(const FOBDD* f) {
		_falsebranch = f;
	}
	void replacetrue(const FOBDD* t) {
		_truebranch = t;
	}
	void replacekernel(const FOBDDKernel* k) {
		_kernel = k;
	}

	FOBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) :
			_kernel(kernel), _truebranch(truebranch), _falsebranch(falsebranch) {
	}

public:
	virtual ~FOBDD(){

	}
	bool containsFreeDeBruijnIndex() const {
		return containsDeBruijnIndex(0);
	}
	bool containsDeBruijnIndex(unsigned int index) const;

	const FOBDDKernel* kernel() const {
		return _kernel;
	}
	const FOBDD* falsebranch() const {
		return _falsebranch;
	}
	const FOBDD* truebranch() const {
		return _truebranch;
	}

	void accept(FOBDDVisitor* visitor) const;

	virtual std::ostream& put(std::ostream& output) const;

};

class TrueFOBDD: public FOBDD {
private:
	friend class FOBDDManager;
	TrueFOBDD(const FOBDDKernel* kernel) :
			FOBDD(kernel, 0, 0) {
	}
public:
	virtual std::ostream& put(std::ostream& output) const;

};

class FalseFOBDD: public FOBDD {
private:
	friend class FOBDDManager;
	FalseFOBDD(const FOBDDKernel* kernel) :
			FOBDD(kernel, 0, 0) {
	}
public:
	virtual std::ostream& put(std::ostream& output) const;


};

#endif /* FOBDD_HPP_ */
