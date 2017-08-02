/*
 * LogActionTime.cpp
 *
 *  Created on: Dec 3, 2012
 *      Author: broes
 */

#include "LogAction.hpp"
#include <cstdio>
#include <ctime>
#include <iostream>
using namespace std;

void logActionAndTime(const std::string& action){
	std::clog <<action << (long long)((double)clock()*1000/(CLOCKS_PER_SEC)) <<" ms\n";
}

void logActionAndTimeSince(const std::string& action, clock_t startTime){
	std::clog <<action << (long long)((double)(clock() - startTime)*1000/(CLOCKS_PER_SEC)) <<" ms\n";
}
