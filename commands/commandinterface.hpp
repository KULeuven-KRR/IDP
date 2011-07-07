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

class Inference {
private:
	std::string				_name;		//!< the name of the procedure
	std::vector<ArgType>	_argtypes;	//!< types of the input arguments

protected:
	void add(ArgType type) { _argtypes.push_back(type); }

public:
	Inference(const std::string& name): _name(name){ }
	virtual ~Inference(){}

	const std::vector<ArgType>& getArgumentTypes() const { return _argtypes; }

	const std::string& getName() const { return _name;	}

	virtual InternalArgument execute(const std::vector<InternalArgument>&) const = 0;
};

#endif /* COMMANDINTERFACE_HPP_ */
