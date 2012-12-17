/*
 * LogActionTime.cpp
 *
 *  Created on: Dec 3, 2012
 *      Author: broes
 */

#include "LogAction.hpp"
#include <cstdio>
#include <iostream>
using namespace std;

void logActionAndTime(const std::string& action){
	std::clog <<action <<"&&" <<((double)clock()/CLOCKS_PER_SEC) <<"\n";
}
void logActionAndValue(const std::string& action, double value){
	std::clog <<action <<"&&" <<value <<"\n";
}
