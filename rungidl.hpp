/************************************
	rungidl.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef RUNGIDL_HPP_
#define RUNGIDL_HPP_

#include <string>

enum class Status {SUCCESS, FAIL};

Status run(const std::string& inputfileurl);

int run(int argc, char* argv[]);

#endif /* RUNGIDL_HPP_ */
