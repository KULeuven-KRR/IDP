/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef IDP_IDPEXCEPTION_HPP_
#define IDP_IDPEXCEPTION_HPP_

#include <sstream>
#include <string>

class Exception {
public:
	~Exception() {
	}
	virtual std::string getMessage() const = 0;
};

class NoSuchProcedureException: public Exception {
	std::string getMessage() const {
		std::stringstream ss;
		ss << "No such lua procedure";
		return ss.str();
	}
};

class AssertionException: public Exception {
private:
	std::string message;
public:
	AssertionException(std::string message)
			: message(message) {

	}
	std::string getMessage() const {
		std::stringstream ss;
		ss << "AssertionException: " << message;
		return ss.str();
	}
};

class IdpException: public Exception {
private:
	std::string message;
public:
	IdpException(std::string message)
			: message(message) {

	}
	std::string getMessage() const {
		std::stringstream ss;
		ss << "IdpException: " << message;
		return ss.str();
	}
};

#endif /* IDP_IDPEXCEPTION_HPP_ */
