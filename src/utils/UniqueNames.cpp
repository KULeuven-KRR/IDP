#include "UniqueNames.hpp"

std::string createName(){
	std::stringstream ss;
	ss << getGlobal()->getNewID();
	return ss.str();
}
