/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef COMMANDINTERFACE_HPP_
#define COMMANDINTERFACE_HPP_

#include <vector>
#include <string>

#include <internalargument.hpp>
#include <monitors/interactiveprintmonitor.hpp>

//TODO refactor the monitors as extra arguments

class Inference {
private:
	std::string				_name;		//!< the name of the procedure
	std::vector<ArgType>	_argtypes;	//!< types of the input arguments
	bool needprintmonitor_, needtracemonitor_;
	InteractivePrintMonitor* printmonitor_;

protected:
	void add(ArgType type) { _argtypes.push_back(type); }
	InteractivePrintMonitor* printmonitor() const { return printmonitor_; }

public:
	Inference(const std::string& name, bool needprintmonitor = false):
			_name(name),
			needprintmonitor_(needprintmonitor),
			printmonitor_(NULL){
	}
	virtual ~Inference(){}

	const std::vector<ArgType>& getArgumentTypes() const { return _argtypes; }

	const std::string& getName() const { return _name;	}

	virtual InternalArgument execute(const std::vector<InternalArgument>&) const = 0;

	bool needPrintMonitor() const { return needprintmonitor_; }
	void addPrintMonitor(InteractivePrintMonitor* monitor) { printmonitor_ = monitor; }

	void clean(){
		if(printmonitor_!=NULL){
			delete(printmonitor_);
			printmonitor_=NULL;
		}
	}
};

#endif /* COMMANDINTERFACE_HPP_ */
