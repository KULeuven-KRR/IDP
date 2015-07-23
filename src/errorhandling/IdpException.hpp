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

#include <sstream>
#include <string>
#include "utils/PrintStacktrace.hpp"

class Exception {
public:
	Exception(){
	}
	virtual ~Exception() {
	}
	virtual std::string getMessage() const = 0;
};

class NoSuchProcedureException: public Exception {
	std::string getMessage() const;
};

class AssertionException: public Exception {
private:
	std::string message;
public:
	AssertionException(std::string message);
	std::string getMessage() const;
};

class IdpException: public Exception {
private:
	std::string message;
public:
	IdpException(std::string message);
	std::string getMessage() const;
};


// Exception which indicates a bug in the code (so maybe should not be shown to the user, ...)
class InternalIdpException: public Exception {
private:
	std::string message;
public:
	InternalIdpException(std::string message);
	std::string getMessage() const;
};

class TimeoutException: public Exception {
public:
	TimeoutException();
	std::string getMessage() const;
};
