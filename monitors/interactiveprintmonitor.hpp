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
	virtual std::string str() = 0;
	virtual void printerror(const std::string& str) = 0;
};

class SavingPrintMonitor : public InteractivePrintMonitor{
private:
	std::stringstream ss;
public:
	virtual void print(const std::string& str){
		ss <<str;
	}
	std::string str() { return ss.str(); }
	virtual void printerror(const std::string& str){
		ss <<str;
	}
};

template<class T> InteractivePrintMonitor& operator<<(InteractivePrintMonitor& monitor, const T& object) {
	std::stringstream ss;
	ss <<object;
	monitor.print(ss.str());
	return monitor;
}

#endif /* INTERACTIVEPRINTMONITOR_HPP_ */
