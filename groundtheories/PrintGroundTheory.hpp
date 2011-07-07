/************************************
	PrintGroundTheory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTGROUNDTHEORY_HPP_
#define PRINTGROUNDTHEORY_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <ostream>

#include "theory.hpp"
#include "ecnf.hpp"
#include "commontypes.hpp"

class InteractivePrintMonitor;
class EcnfPrinter;

/**
 * A ground theory which does not store the grounding, but directly writes it to its monitors.
 */
//TODO monitor policy
class PrintGroundTheory : public AbstractGroundTheory {
private:
	InteractivePrintMonitor* monitor_;
	EcnfPrinter*			printer_;

public:
	PrintGroundTheory(InteractivePrintMonitor* monitor, AbstractStructure* str);
	void recursiveDelete() { delete(this);	}

	EcnfPrinter&	printer() { return *printer_; }

	// Mutators
	virtual void	addClause(GroundClause& cl, bool skipfirst = false);
	virtual void	addDefinition(GroundDefinition*);
	virtual void	addFixpDef(GroundFixpDef*);
	virtual void	addSet(int setnr, int defnr, bool weighted);
	virtual void	addAggregate(int tseitin, AggTsBody* body);
	virtual void 	addCPReification(int tseitin, CPTsBody* body);

	virtual void	addPCRule(int defnr, int tseitin, PCTsBody* body);
	virtual void	addAggRule(int defnr, int tseitin, AggTsBody* body);

	// Visitor
	virtual void			accept(TheoryVisitor* v) const		{ /*NOOP*/	}
	virtual AbstractTheory*	accept(TheoryMutatingVisitor* v)	{ return NULL; /*TODO*/	}

	// Debugging
	virtual std::ostream&	put(std::ostream& output, unsigned int)	const { assert(false); return output;	}
	virtual std::string		to_string()								const { assert(false); return "";		}
};

#endif /* PRINTGROUNDTHEORY_HPP_ */
