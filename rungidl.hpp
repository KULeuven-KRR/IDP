/************************************
	rungidl.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef RUNGIDL_HPP_
#define RUNGIDL_HPP_

#include <string>
#include <vector>

enum class Status {SUCCESS, FAIL};

Status getTestStatus();
void setTestStatus(Status status);

void run(const std::string& inputfileurl);
void run(const std::vector<std::string>& inputfileurls);
int run(int argc, char* argv[]);

#endif /* RUNGIDL_HPP_ */
