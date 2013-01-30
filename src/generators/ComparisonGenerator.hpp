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

#ifndef GTGenerator_HPP_
#define GTGenerator_HPP_

#include "commontypes.hpp"
#include "InstGenerator.hpp"
#include "structure/MainStructureComponents.hpp"

class SortTable;
class DomElemContainer;

enum class Input {
	BOTH, NONE, LEFT, RIGHT
};
enum class CompResult {
	VALID, INVALID
};

/**
 * Formula: x op y, with op one of {=<, <, =>, >, =, ~=}, possibly different domains
 */
class ComparisonGenerator: public InstGenerator {
private:
	SortTable *_leftsort, *_rightsort;
	const DomainElement *_latestleft, *_latestright;
	const DomElemContainer *_leftvar, *_rightvar;
	CompType _comparison;
	Input _input; // NOTE: is never RIGHT after initialization
	SortIterator _left, _right;

	bool _reset, increaseouter;

	/**
	 * The first argument is the finite one of such is available.
	 * NOTE: optimized for EQ comparison
	 */
	void findNext(SortIterator* finiteside, SortIterator* undefinedside, SortTable* undefinedSort, const DomElemContainer* finiteContainer,
			const DomElemContainer* undefinedContainer, bool finiteGTorGEQundef);

public:
	ComparisonGenerator(SortTable* leftsort, SortTable* rightsort, const DomElemContainer* leftvalue, const DomElemContainer* rightvalue, Input input,
			CompType type);

	virtual void put(std::ostream& stream) const;
	ComparisonGenerator* clone() const;
	void internalSetVarsAgain();
	void reset();

	void next();

private:
	bool leftIsInput() const;
	bool rightIsInput() const;

	CompResult checkAndSet();
	bool correct();
};

#endif /* GTGenerator_HPP_ */
