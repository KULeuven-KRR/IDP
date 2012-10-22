/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IdpException.hpp"

std::string NoSuchProcedureException::getMessage() const {
	std::stringstream ss;
	ss << "No such lua procedure";
	return ss.str();
}

AssertionException::AssertionException(std::string message)
		: message(message) {
#ifdef DEBUG
	printStacktrace();
#endif
}
std::string AssertionException::getMessage() const {
	std::stringstream ss;
	ss << "AssertionException: " << message;
	return ss.str();
}

IdpException::IdpException(std::string message)
		: message(message) {
#ifdef DEBUG
	printStacktrace();
#endif
}

std::string IdpException::getMessage() const {
	std::stringstream ss;
	ss << "IdpException: " << message;
	return ss.str();
}
