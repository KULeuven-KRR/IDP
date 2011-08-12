/************************************
	commandinterface.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMMANDINTERFACE_HPP_
#define COMMANDINTERFACE_HPP_

#include <vector>
#include <string>

#include <internalargument.hpp>
#include <monitors/interactiveprintmonitor.hpp>
#include <monitors/tracemonitor.hpp>

//TODO refactor the monitors as extra arguments

class Inference {
private:
	std::string				_name;		//!< the name of the procedure
	std::vector<ArgType>	_argtypes;	//!< types of the input arguments
	bool needprintmonitor_, needtracemonitor_;
	InteractivePrintMonitor* printmonitor_;
	TraceMonitor* 			tracemonitor_;

protected:
	void add(ArgType type) { _argtypes.push_back(type); }
	InteractivePrintMonitor* printmonitor() const { return printmonitor_; }
	TraceMonitor* tracemonitor() const { return tracemonitor_; }

public:
	Inference(const std::string& name, bool needprintmonitor = false, bool needtracemonitor = false):
			_name(name),
			needprintmonitor_(needprintmonitor),
			needtracemonitor_(needtracemonitor),
			printmonitor_(NULL),
			tracemonitor_(NULL){
	}
	virtual ~Inference(){}

	const std::vector<ArgType>& getArgumentTypes() const { return _argtypes; }

	const std::string& getName() const { return _name;	}

	virtual InternalArgument execute(const std::vector<InternalArgument>&) const = 0;

	bool needPrintMonitor() const { return needprintmonitor_; }
	bool needTraceMonitor() const { return needtracemonitor_; }
	void addPrintMonitor(InteractivePrintMonitor* monitor) { printmonitor_ = monitor; }
	void addTraceMonitor(TraceMonitor* monitor) { tracemonitor_ = monitor; }

	void clean(){
		if(printmonitor_!=NULL){
			delete(printmonitor_);
			printmonitor_=NULL;
		}
		if(tracemonitor_!=NULL){
			delete(tracemonitor_);
			tracemonitor_=NULL;
		}
	}
};

#endif /* COMMANDINTERFACE_HPP_ */
