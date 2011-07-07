/************************************
	symmetry.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <set>
#include <map>
#include <vector>

class DomainElement;
class AbstractStructure;
class Sort;

typedef std::set<const DomainElement*> IVSet;

void findDontCares(AbstractStructure*, std::map<Sort*, IVSet>&, std::map<Sort*, IVSet>&, std::map<Sort*, std::vector<IVSet> >&);
