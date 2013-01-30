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

#include <string>
#include <sstream>
#include <vector>
#include <functional>

bool startsWith(const std::string& string, const std::string& substring);

// trim from start
std::string &ltrim(std::string &s);

// trim from end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

std::string trim(const std::string& s);

std::string replaceAllIn(const std::string& text, const std::string& find, const std::string& replacement);

std::string replaceAllAndTrimEachLine(const std::string& text, const std::string& find, const std::string& replacement);

std::vector<std::string> split(const std::string &s, const std::string& delim);

/**
 * Return a string representing a list of elements (not including last if printlast is false), delimited by delim and which are printed themselves by calling the provided function).
 */
template<template<class, typename ...> class List, class Element, typename... Args, class FunctionFromElementToString>
void printList(std::ostream& stream, const List<Element, Args...>& list, const std::string& delim, const FunctionFromElementToString& func, bool printlast = true) {
	auto begin = true;
	typename List<Element, Args...>::size_type counter = 0;
	for (auto s = list.cbegin(); s!=list.cend(); ++s) {
		if(not printlast && counter==list.size()-1) {
			break;
		}
		counter++;
		if (not begin) {
			stream << delim;
		}
		begin = false;
		func(stream, *s);
	}
}
