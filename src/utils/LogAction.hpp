#pragma once

#include <string>
#include <iostream>

void logActionAndTime(const std::string& action);
template<class Value>
void logActionAndValue(const std::string& action, const Value& value){
	std::clog <<action <<"&&" <<value <<"\n";
}
