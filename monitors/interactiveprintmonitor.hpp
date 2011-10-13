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
#include <sstream>

class InteractivePrintMonitor{
public:
	virtual void print(const std::string& str)=0;
	virtual void flush() = 0;
	virtual void printerror(const std::string& str) = 0;

	template<class T>
	InteractivePrintMonitor& operator<<(const T& object){
		std::stringstream ss;
		ss <<object;
		print(ss.str());
		return *this;
	}
};

#endif /* INTERACTIVEPRINTMONITOR_HPP_ */
