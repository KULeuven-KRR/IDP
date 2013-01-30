/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef INTERACTIVEPRINTMONITOR_HPP_
#define INTERACTIVEPRINTMONITOR_HPP_

/**
 * Class intended for use in inferences in which printing during the inference is necessary.
 */

#include <string>
#include <sstream>

class InteractivePrintMonitor {
public:
	virtual ~InteractivePrintMonitor(){}
	virtual void print(const std::string& str)=0;
	virtual void flush() = 0;
	virtual void printerror(const std::string& str) = 0;

	template<class T>
	InteractivePrintMonitor& operator<<(const T& object) {
		std::stringstream ss;
		ss << object;
		print(ss.str());
		return *this;
	}
};

#endif /* INTERACTIVEPRINTMONITOR_HPP_ */
