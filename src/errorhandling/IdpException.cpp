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
	ss << message;
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
	ss << message;
	return ss.str();
}

InternalIdpException::InternalIdpException(std::string message)
		: message(message) {
#ifdef DEBUG
	printStacktrace();
#endif
}

std::string InternalIdpException::getMessage() const {
	std::stringstream ss;
	ss << "InternalIdpException: " << message;
	return ss.str();
}

TimeoutException::TimeoutException() {
}

std::string TimeoutException::getMessage() const {
	std::stringstream ss;
	ss << "Inference timed-out";
	return ss.str();
}
