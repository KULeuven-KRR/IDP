/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "LazyInst.hpp"

class DelayGrounder {
private:
	DefId _id;
	Context _context;

	bool _isGrounding;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	AbstractGroundTheory* _grounding;

	std::vector<std::pair<int, int> > sameargs; // a list of indices into the head terms which are the same variables

public:
	// @precondition: two IDs HAVE to be different if referring to an instance of a symbol in a DIFFERENT definition (if it is a head)
	//		it HAS to be -1 if it is not a head occurrence
	// 		in all other cases, they should preferably be equal
	DelayGrounder(PFSymbol* symbol, const std::vector<Term*>& terms, Context context, DefId id, AbstractGroundTheory* gt);
	virtual ~DelayGrounder() {
	}

	void ground(const Lit& boundlit, const ElementTuple& args);
	void notify(const Lit& boundlit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders);

	DefId getID() const {
		return _id;
	}
	Context getContext() const {
		return _context;
	}

protected:
	AbstractGroundTheory* getGrounding() const {
		return _grounding;
	}

	const std::vector<std::pair<int, int> >& getSameargs() const {
		return sameargs;
	}

	void doGrounding();
	virtual void doGround(const Lit& boundlit, const ElementTuple& args) = 0;
};

class LazyUnknUnivGrounder: public FormulaGrounder, public DelayGrounder {
private:
	bool _isGrounding;
	std::vector<const DomElemContainer*> _varcontainers;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	FormulaGrounder* _subgrounder;

public:
	LazyUnknUnivGrounder(const PredForm* pf, Context context, const var2dommap& varmapping, AbstractGroundTheory* groundtheory, FormulaGrounder* sub,
			const GroundingContext& ct);

	virtual void run(ConjOrDisj& formula) const;

	tablesize getGroundedSize() const {
		return tablesize(TableSizeType::TST_UNKNOWN, 0); // TODO
	}

protected:
	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}

	dominstlist createInst(const ElementTuple& args);
	void doGround(const Lit& boundlit, const ElementTuple& args);
};

class LazyTwinDelayUnivGrounder: public FormulaGrounder, public DelayGrounder {
private:
	std::vector<ElementTuple> _seen;

	bool _isGrounding;
	std::vector<const DomElemContainer*> _varcontainers;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	FormulaGrounder* _subgrounder;

public:
	LazyTwinDelayUnivGrounder(PFSymbol* symbol, const std::vector<Term*>& terms, Context context, const var2dommap& varmapping,
			AbstractGroundTheory* groundtheory, FormulaGrounder* sub, const GroundingContext& ct);

	tablesize getGroundedSize() const {
		return tablesize(TableSizeType::TST_UNKNOWN, 0); // TODO
	}

	virtual void run(ConjOrDisj& formula) const;

protected:
	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}

	dominstlist createInst(const ElementTuple& args);
	void doGround(const Lit& boundlit, const ElementTuple& args);
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
