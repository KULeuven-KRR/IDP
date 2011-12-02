#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <common.hpp>
#include "GroundingContext.hpp"

class Variable;
class DomElemContainer;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;
class TsBody;

typedef std::vector<Variable*> varlist;
typedef std::map<Variable*, const DomElemContainer*> var2dommap;

typedef unsigned int VarId;

typedef std::pair<ElementTuple, Lit> Tuple2Atom;
typedef std::pair<Lit, TsBody*> tspair;

#endif /* UTILS_HPP_ */
