/************************************
	rungidl.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef RUNGIDL_HPP_
#define RUNGIDL_HPP_

#include <string>
#include <vector>
#include <ostream>

enum class Status {SUCCESS, FAIL};

std::ostream& operator<<(std::ostream& stream, Status status);

int run(int argc, char* argv[]);

Status test(const std::vector<std::string>& inputfileurls);

#endif /* RUNGIDL_HPP_ */
