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

#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBddVisitor.hpp"

bool FOBDD::containsDeBruijnIndex(unsigned int index) const {
	if (_kernel->containsDeBruijnIndex(index)) {
		return true;
	} else if (_falsebranch != NULL && _falsebranch->containsDeBruijnIndex(index)) {
		return true;
	} else if (_truebranch != NULL && _truebranch->containsDeBruijnIndex(index)) {
		return true;
	} else {
		return false;
	}
}

void FOBDD::accept(FOBDDVisitor* visitor) const {
	visitor->visit(this);
}

bool FOBDD::operator<(const FOBDD& rhs) const {
	Assert((falsebranch()==NULL) == (truebranch()==NULL));

	if(truebranch()==NULL){
		return true;
	}
	if(rhs.truebranch()==NULL){
		return false;
	}

	if(*kernel()<*rhs.kernel()){
		return true;
	}else if(*rhs.kernel()<*kernel()){
		return false;
	}else{
		if(*falsebranch()<*rhs.falsebranch()){
			return true;
		}else if(*rhs.falsebranch()<*falsebranch()){
			return false;
		}else{
			return *truebranch()<*rhs.truebranch();
		}
	}
}

std::ostream& FOBDD::put(std::ostream& output) const {
	output << print(_kernel);
	pushtab();
	output << "" <<nt();
	output << "FALSE BRANCH:";
	pushtab();
	output << "" <<nt();
	output << print(_falsebranch);
	poptab();
	output << "" <<nt();
	output << "TRUE BRANCH:";
	pushtab();
	output << "" <<nt();
	output << print(_truebranch);
	poptab();
	poptab();
	return output;
}

std::ostream& TrueFOBDD::put(std::ostream& output) const {
	output << "true";
	return output;
}

std::ostream& FalseFOBDD::put(std::ostream& output) const {
	output << "false";
	return output;
}

