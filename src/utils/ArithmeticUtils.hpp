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

#ifndef ARITHMETICUTILS_HPP_
#define ARITHMETICUTILS_HPP_

#include "safeint3.hpp"

/*
 * Wrapper classes for the SafeInt3 Microsoft code.
 *
 * Returned bool indicates whether the operation was
 * executed safely (e.g. no overflow/underflow)
 */

template<typename T, typename U>
void addition(T t, U u, T& result, bool& safe) {
	safe = SafeAdd<T,U>(t,u,result);
}

template<typename T, typename U>
void subtraction(T t, U u, T& result, bool& safe) {
	safe = SafeSubtract<T,U>(t,u,result);
}

template<typename T, typename U>
void multiplication(T t, U u, T& result, bool& safe) {
	safe = SafeMultiply<T,U>(t,u,result);
}

template<typename T, typename U>
void division(T t, U u, T& result, bool& safe) {
	safe = SafeDivide<T,U>(t,u,result);
}

template<typename T, typename U>
void modulus(T t, U u, T& result, bool& safe) {
	safe = SafeModulus<T,U>(t,u,result);
}

#endif /* ARITHMETICUTILS_HPP_ */
