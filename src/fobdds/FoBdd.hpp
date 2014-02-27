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

#pragma once

#include <iostream>
#include <memory>
#include "parseinfo.hpp"

class FOBDDKernel;
class FOBDDManager;
class FOBDDVisitor;
class ParseInfo;

class FOBDD {
private:
	friend class FOBDDManager;
	friend class TrueFOBDD;
	friend class FalseFOBDD;

	const FOBDDKernel* _kernel;
	const FOBDD* _truebranch;
	const FOBDD* _falsebranch;
	std::weak_ptr<FOBDDManager> _manager;
	ParseInfo _pi; //!< The place where the fobdd was parsed.

	void replacefalse(const FOBDD* f) {
		_falsebranch = f;
	}
	void replacetrue(const FOBDD* t) {
		_truebranch = t;
	}
	void replacekernel(const FOBDDKernel* k) {
		_kernel = k;
	}

	FOBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch, std::shared_ptr<FOBDDManager> manager, const ParseInfo& pi = *new ParseInfo()) :
			_kernel(kernel), _truebranch(truebranch), _falsebranch(falsebranch), _manager(manager), _pi(pi) {
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
	const ParseInfo& pi() const {
		return _pi;
	}

	void accept(FOBDDVisitor* visitor) const;

	std::shared_ptr<FOBDDManager>manager() const{
		if(_manager.expired()){
			throw IdpException("Invalid code path");
		}
		return _manager.lock();
	}

	bool operator<(const FOBDD& rhs) const;

	virtual std::ostream& put(std::ostream& output) const;
};

class TrueFOBDD: public FOBDD {
private:
	friend class FOBDDManager;
	TrueFOBDD(const FOBDDKernel* kernel, std::shared_ptr<FOBDDManager> manager) :
			FOBDD(kernel, 0, 0,manager) {
	}
public:
	virtual std::ostream& put(std::ostream& output) const;
};

class FalseFOBDD: public FOBDD {
private:
	friend class FOBDDManager;
	FalseFOBDD(const FOBDDKernel* kernel, std::shared_ptr<FOBDDManager> manager) :
			FOBDD(kernel, 0, 0, manager) {
	}
public:
	virtual std::ostream& put(std::ostream& output) const;
};
