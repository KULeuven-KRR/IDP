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

#ifndef PROPAGATESCHED_HPP
#define PROPAGATESCHED_HPP

#include <queue>
#include "PropagationCommon.hpp"

class FOPropagation;
class Formula;



/**
 * 	Class for scheduling propagation steps.	
 */
class FOPropScheduler {
private:
	std::queue<FOPropagation*> _queue; //!< The queue of scheduled propagations
public:
	void add(FOPropagation*); //!< Push a propagation on the queue
	FOPropagation* next(); //!< Pop the first propagation from the queue and return it

	bool hasNext() const; //!< True iff the queue is non-empty
};

/**
 * 	Class representing a single propagation step.
 */
class FOPropagation {
private:
	const Formula* _parent; //!< The parent formula
	FOPropDirection _direction; //!< Direction of propagation (from parent to child or vice versa)
	bool _ct; //!< Indicates which domain is propagated
			  //!< If _direction == DOWN and _ct == true,
			  //!< then the ct-domain of the parent is propagated to the child, etc.
	const Formula* _child; //!< The subformula // NOTE child can be NULL if child == NULL, propagate to all childs.
public:
	FOPropagation(const Formula* p, FOPropDirection dir, bool ct, const Formula* c)
			: _parent(p), _direction(dir), _ct(ct), _child(c) {
	}

	const Formula* getChild() const {
		return _child;
	}
	FOPropDirection getDirection() const {
		return _direction;
	}
	const Formula* getParent() const {
		return _parent;
	}
	bool isCT() const {
		return _ct;
	}
};




#endif //PROPAGATESCHED_HPP
