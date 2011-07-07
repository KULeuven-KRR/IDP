/************************************
	interactiveprintmonitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INTERACTIVEPRINTMONITOR_HPP_
#define INTERACTIVEPRINTMONITOR_HPP_

/**
 * Class intended for use in inferences in which printing during the inference is necessary.
 */

#include <string>

class InteractivePrintMonitor{
public:
	virtual void print(const std::string&) = 0;
	virtual void printerror(const std::string&) = 0;
};

#endif /* INTERACTIVEPRINTMONITOR_HPP_ */
