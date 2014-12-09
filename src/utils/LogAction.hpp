#pragma once

#include <string>
#include <iostream>
#include <ctime>

void logActionAndTime(const std::string& action);
template<class Value>
void logActionAndValue(const std::string& action, const Value& value){
	std::clog <<action <<"&&" <<value <<"\n";
}
void logActionAndTimeSince(const std::string& action, clock_t startTime);
