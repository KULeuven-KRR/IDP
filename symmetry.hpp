/************************************
	symmetry.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <set>
#include <map>
#include <vector>
#include <string>

class DomainElement;
class AbstractStructure;
class AbstractTheory;
class Sort;

typedef std::set<const DomainElement*> IVSet;

std::map<const Sort*,std::vector<IVSet> > findIVSets(const AbstractTheory*, const AbstractStructure*);

std::string printIVSets(std::map<const Sort*,std::vector<IVSet> >&);
