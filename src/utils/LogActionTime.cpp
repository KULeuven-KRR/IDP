/*
 * LogActionTime.cpp
 *
 *  Created on: Dec 3, 2012
 *      Author: broes
 */

#include "LogActionTime.hpp"
#include <cstdio>
#include <iostream>
using namespace std;

void logActionAndTime(const std::string& action){
	std::clog <<action << (long long)((double)clock()*1000/(CLOCKS_PER_SEC)) <<"ms\n";
}
