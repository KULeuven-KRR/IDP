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

#ifndef LAZYINST_HPP_
#define LAZYINST_HPP_

#include "inferences/grounding/GroundUtils.hpp"

class LazyDisjunctiveGrounder;
class InstGenerator;
class DomElemContainer;
class DomainElement;

typedef std::pair<const DomElemContainer*, const DomainElement*> dominst;
typedef std::vector<dominst> dominstlist;

/**
 * Represents a variable instantiation and current generator / grounder state from which grounding should continue.
 */
struct LazyInstantiation {
private:
	const LazyDisjunctiveGrounder* grounder;
public:
	Lit residual;
	InstGenerator* generator;
	size_t index;
	dominstlist freevarinst;

	void notifyTheoryOccurrence(TsType type);
	void notifyGroundingRequested(int ID, bool groundall, bool& stilldelayed);

	void setGrounder(const LazyDisjunctiveGrounder* g) { grounder = g;}
	const LazyDisjunctiveGrounder* getGrounder() const { return grounder; }

	/*	bool operator==(const LazyInstantiation& rhs) const {
	 return rhs.residual == residual && freevarinst == rhs.freevarinst;
	 }*/
};

#endif /* LAZYINST_HPP_ */
