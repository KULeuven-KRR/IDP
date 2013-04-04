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

#pragma once

#include "printers/print.hpp"
#include "IncludeComponents.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/GroundPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

template<typename Stream>
class ASPPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	const GroundTranslator* _translator;

	using StreamPrinter<Stream>::output;
	using StreamPrinter<Stream>::printTab;
	using StreamPrinter<Stream>::unindent;
	using StreamPrinter<Stream>::indent;
	using StreamPrinter<Stream>::isDefClosed;
	using StreamPrinter<Stream>::isDefOpen;
	using StreamPrinter<Stream>::closeDef;
	using StreamPrinter<Stream>::openDef;
	using StreamPrinter<Stream>::isTheoryOpen;
	using StreamPrinter<Stream>::closeTheory;
	using StreamPrinter<Stream>::openTheory;

	std::string _currentSymbol; //!< Variable to hold the string representing the symbol currently being printed.

	// Indicates whether the warning concerning three-valued structures (where only false atoms are printed) has already been issued.
	static bool threevalWarningIssued;

public:
	ASPPrinter(Stream& stream)
		: StreamPrinter<Stream>(stream), _translator(NULL) {
	}

	virtual void setTranslator(GroundTranslator* t) {
		_translator = t;
	}
	virtual void startTheory() {
		openTheory();
	}
	virtual void endTheory() {
		closeTheory();
	}

	void visit(const Vocabulary*) {
		throw notyetimplemented("Printing vocabularies in ASP format");
	}
	void visit(const Namespace*) {
		throw notyetimplemented("Printing namespaces in ASP format");
	}
	void visit(const Theory*) {
		throw notyetimplemented("Printing theories in ASP format");
	}
	void visit(const Query*) {
		throw notyetimplemented("Printing queries in ASP format");
	}

	void visit(const AbstractStructure* structure) {
		Assert(isTheoryOpen());
		if (not structure->isConsistent()) {
			output() << "INCONSISTENT STRUCTURE";
			return;
		}
		Vocabulary* voc = structure->vocabulary();
		for (auto it = voc->firstSort(); it != voc->lastSort(); ++it) {
			CHECKTERMINATION
			auto sort = it->second;
			if (not sort->builtin()) {
				_currentSymbol = toString(sort);
				auto sorttable = structure->inter(sort);
				visit(sorttable);
			}
		}
		for (auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
			CHECKTERMINATION
			auto udpreds = it->second->nonbuiltins();
			for (auto jt = udpreds.cbegin(); jt != udpreds.cend(); ++jt) {
				auto pred = *jt;
				if (pred->arity() != 1 || pred->sorts()[0]->pred() != pred) {
					auto predinter = structure->inter(pred);
					if(not predinter->approxTwoValued() && not threevalWarningIssued){
						Warning::warning("Printing only true facts of three-valued structures.");
						threevalWarningIssued = true;
					}
					_currentSymbol = toString(pred);
					visit(predinter->ct());
				}
			}
		}
		for (auto it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
			CHECKTERMINATION
			auto udfuncs = it->second->nonbuiltins();
			for (auto jt = udfuncs.cbegin(); jt != udfuncs.cend(); ++jt) {
				auto func = *jt;
				auto funcinter = structure->inter(func);

				if(not funcinter->approxTwoValued() && not threevalWarningIssued){
					Warning::warning("Printing only true facts of three-valued structures.");
					threevalWarningIssued = true;
				}
				auto funcgraph = funcinter->graphInter();
				_currentSymbol = toString(func);
				visit(funcgraph->ct());
			}
		}
	}

	void visit(const SortTable* table) {
		Assert(isTheoryOpen());
		for (auto it = table->sortBegin(); not it.isAtEnd(); ++it) {
			output() << _currentSymbol << "(" << print(*it) << ").";
		}
		output() << "\n";
	}

	void visit(const PredTable* table) {
		Assert(isTheoryOpen());
		if (not table->approxFinite()) {
			output() << "possibly infinite table"; //FIXME throw warning?
		}
		auto kt = table->begin();
		if (table->arity() > 0) {
			if (not kt.isAtEnd()) {
				for (; not kt.isAtEnd(); ++kt) {
					CHECKTERMINATION
					output() << _currentSymbol << "(";
					auto tuple = *kt;
					bool begintuple = true;
					for (auto lt = tuple.cbegin(); lt != tuple.cend(); ++lt) {
						if (not begintuple) {
							output() << ',';
						}
						begintuple = false;
						output() << print(*lt);
					}
					output() << ").";
				}
			}
		} else if (not kt.isAtEnd()) {
			output() << _currentSymbol << ".";
		} else {
			//Note: atom is false.
		}
		output() << "\n";
	}


	virtual void visit(const GroundClause&) {
		throw notyetimplemented("Printing ground clauses in ASP format");
	}
	virtual void visit(const GroundFixpDef*) {
		throw notyetimplemented("Printing groud fixpoint definitions in ASP format");
	}
	virtual void visit(const GroundSet*) {
		throw notyetimplemented("Printing ground sets in ASP format");
	}
	virtual void visit(const PCGroundRule*) {
		throw notyetimplemented("Printing ground rules in ASP format");
	}
	virtual void visit(const AggGroundRule*) {
		throw notyetimplemented("Printing ground rules in ASP format");
	}
	virtual void visit(const GroundAggregate*) {
		throw notyetimplemented("Printing ground aggregates in ASP format");
	}
	virtual void visit(const CPReification*) {
		throw notyetimplemented("Printing ground constraints in ASP format");
	}
};
