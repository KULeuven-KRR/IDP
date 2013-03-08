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
#include "theory/Query.hpp"
#include "groundtheories/GroundPolicy.hpp"

#include "utils/StringUtils.hpp"

//TODO is not guaranteed to generate correct idp files!
// FIXME do we want this? Because printing cp constraints etc. should be done correctly then!
//TODO usage of stored parameters might be incorrect in some cases.

template<typename Stream>
class IDP2Printer: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
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

public:
	IDP2Printer(Stream& stream)
			: 	StreamPrinter<Stream>(stream) {
	}


	virtual void startTheory() {
		openTheory();
	}
	virtual void endTheory() {
		closeTheory();
	}

	template<typename T>
	std::string printFullyQualified(T o) const {
		std::stringstream ss;
		ss << o->nameNoArity();
		if (o->sorts().size() > 0) {
			ss << "[";
			printList(ss, o->sorts(), ",", not o->isFunction());
			if (o->isFunction()) {
				ss << ":";
				ss << print(*o->sorts().back());
			}
			ss << "]";
		}
		return ss.str();
	}



	void visit(const AbstractStructure* structure) {
		Assert(isTheoryOpen());

		printTab();
		output() << "Data: " << '\n';
		indent();

		auto voc = structure->vocabulary();
		for (auto it = voc->firstSort(); it != voc->lastSort(); ++it) {
			auto s = it->second;
			if (not s->builtin()) {
				printTab();
				auto name = s->name();
				name = capitalize(name);
				output() << name << " = ";
				auto st = structure->inter(s);
				visit(st);
				output() << '\n';
			}
		}
		for (auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
			auto sp = it->second->nonbuiltins();
			for (auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
				auto p = *jt;
				if (p->arity() == 1 && p->sorts()[0]->pred() == p) { // If it is in fact a sort, ignore it
					continue;
				}
				auto pi = structure->inter(p);
				if(pi->ct()->size() == 0 && pi->cf()->size() == 0){
					continue;
				}
				if(not pi->approxTwoValued()){
					output() << "Partial: " << '\n'; //TEMPORARY GO TO PARTIAL BLOCK
				}
				printTab();
				auto name = p->nameNoArity();
				name = capitalize(name);
				output() << name << " = ";
				visit(pi->ct());
				if (not pi->approxTwoValued()) {
					visit(pi->cf());
					output() << '\n';
					output() << "Data: "; //RETURN TO DATA BLOCK
				}
				output() << '\n';
			}
		}
		for (auto it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
			auto sf = it->second->nonbuiltins();
			for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
				auto f = *jt;
				auto fi = structure->inter(f);
				if (fi->approxTwoValued()) {
					printTab();
					auto name = f->nameNoArity();
					name = capitalize(name);
					output() << name << " = ";
					auto ft = fi->funcTable();
					visit(ft);
				} else {
					output() << "Partial: " << '\n';//TEMPORARY GO TO PARTIAL BLOCK
					printTab();
					auto pi = fi->graphInter();
					auto ct = pi->ct();
					printAsFunc(ct);
					auto cf = pi->cf();
					printAsFunc(cf);
					output() << '\n';
					output() << "Data: ";//RETURN TO DATA BLOCK
				}
				output() << '\n';
			}
		}
		unindent();
		output() << '\n';
	}

	void visit(const Query* q) {
		throw notyetimplemented("Printing queries in IDP2 format");
	}

	void visit(const Vocabulary* v) {
		throw notyetimplemented("Printing vocabularies in IDP2 format");
	}

	void visit(const Namespace* s) {
		throw notyetimplemented("Printing namespaces in IDP2 format");
	}

	void visit(const Theory* t) {
		throw notyetimplemented("Printing theories in IDP2 format");
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto i = list.cbegin(); i < list.cend(); ++i) {
			CHECKTERMINATION;
			(*i)->accept(v);
		}

	}

	void visit(const GroundTheory<GroundPolicy>* g) {
		throw notyetimplemented("Printing (ground)theories in IDP2 format");
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}

	void visit(const EqChainForm* f) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}

	void visit(const EquivForm* f) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}

	void visit(const BoolForm* f) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}

	void visit(const QuantForm* f) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}

	void visit(const AggForm* f) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}

	/** Definitions **/

	void visit(const Rule* r) {
		throw notyetimplemented("Printing rules in IDP2 format");
	}

	void visit(const Definition* d) {
		throw notyetimplemented("Printing definitions in IDP2 format");
	}

	void visit(const FixpDef* d) {
		throw notyetimplemented("Printing fixpdefinitions in IDP2 format");
	}


	void visit(const VarTerm* t) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}

	void visit(const FuncTerm* t) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}

	void visit(const DomainTerm* t) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}

	void visit(const AggTerm* t) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}

	/** Set expressions **/

	void visit(const EnumSetExpr* s) {
		throw notyetimplemented("Printing set expressions in IDP2 format");
	}

	void visit(const QuantSetExpr* s) {
		throw notyetimplemented("Printing set expressions in IDP2 format");
	}



	void visit(const CPWSumTerm* cpt) {
		throw notyetimplemented("Printing CP thingies in IDP2 format");
	}

	void visit(const CPWProdTerm* cpt) {
		throw notyetimplemented("Printing CP thingies in IDP2 format");
	}

	void visit(const CPVarTerm* cpt) {
		throw notyetimplemented("Printing CP thingies in IDP2 format");
	}

	void visit(const PredTable* table) {
		Assert(isTheoryOpen());
		if (not table->finite()) {
			std::clog << "Requested to print infinite table, did not do this.\n";
		}
		TableIterator kt = table->begin();
		if (table->arity() > 0) {
			output() << "{ ";
			if (not kt.isAtEnd()) {
				bool beginlist = true;
				for (; not kt.isAtEnd(); ++kt) {
					CHECKTERMINATION;
					if (not beginlist) {
						output() << "; ";
					}
					beginlist = false;
					ElementTuple tuple = *kt;
					bool begintuple = true;
					for (auto lt = tuple.cbegin(); lt != tuple.cend(); ++lt) {
						if (not begintuple) {
							output() << ',';
						}
						begintuple = false;
						output() << print(*lt);
					}
				}
			}
			output() << " }";
		} else if (not kt.isAtEnd()) {
			output() << "{()}";
		} else {
			output() << "{}";
		}
	}

	void visit(FuncTable* table) {
		Assert(isTheoryOpen());
		std::vector<SortTable*> vst = table->universe().tables();
		vst.pop_back();
		Universe univ(vst);
		if (univ.approxFinite()) {
			TableIterator kt = table->begin();
			if (table->arity() != 0) {
				output() << "{ ";
				if (not kt.isAtEnd()) {
					ElementTuple tuple = *kt;
					output() << print(tuple[0]);
					for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
						output() << ',' << print(tuple[n]);
					}
					output() << "->" << print(tuple.back());
					++kt;
					for (; not kt.isAtEnd(); ++kt) {
						CHECKTERMINATION;
						output() << "; ";
						tuple = *kt;
						output() << print(tuple[0]);
						for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
							output() << ',' << print(tuple[n]);
						}
						output() << "->" << print(tuple.back());
					}
				}
				output() << " }";
			} else if (not kt.isAtEnd()) {
				output() << print((*kt)[0]);
			} else {
				output() << "{ }";
			}
		} else {
			output() << "possibly infinite table";
		}
	}

	void visit(const Sort* s) {
		throw notyetimplemented("Printing sorts in IDP2 format");
	}

	void visit(const Predicate* p) {
		throw notyetimplemented("Printing predicates in IDP2 format");
	}

	void visit(const Function* f) {
		throw notyetimplemented("Printing functions in IDP2 format");
	}

	void visit(const SortTable* table) {
		Assert(isTheoryOpen());
		output() << "{ ";
		if (table->isRange()) {
			output() << print(table->first()) << ".." << print(table->last());
		} else {
			auto it = table->sortBegin();
			if (not it.isAtEnd()) {
				output() << print((*it));
				++it;
				for (; not it.isAtEnd(); ++it) {
					CHECKTERMINATION;
					output() << "; " << print((*it));
				}
			}
		}
		output() << " }";
	}

	virtual void visit(const GroundClause&) {
		throw notyetimplemented("Printing ground clauses in IDP2 format");
	}
	virtual void visit(const GroundFixpDef*) {
		throw notyetimplemented("Printing groud fixpoint definitions in IDP2 format");
	}
	virtual void visit(const GroundSet*) {
		throw notyetimplemented("Printing ground sets in IDP2 format");
	}
	virtual void visit(const PCGroundRule*) {
		throw notyetimplemented("Printing ground rules in IDP2 format");
	}
	virtual void visit(const AggGroundRule*) {
		throw notyetimplemented("Printing ground rules in IDP2 format");
	}
	virtual void visit(const GroundAggregate*) {
		throw notyetimplemented("Printing ground aggregates in IDP2 format");
	}
	virtual void visit(const CPReification*) {
		throw notyetimplemented("Printing ground constraints in IDP2 format");
	}

private:


	void printAsFunc(const PredTable* table) {
		if (not table->finite()) {
			std::clog << "Requested to print infinite predtable, did not print it.\n";
			return;
		}
		TableIterator kt = table->begin();
		output() << "{ ";
		if (not kt.isAtEnd()) {
			ElementTuple tuple = *kt;
			if (tuple.size() > 1) {
				output() << print(tuple[0]);
			}
			for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
				output() << ',' << print(tuple[n]);
			}
			output() << "->" << print(tuple.back());
			++kt;
			for (; not kt.isAtEnd(); ++kt) {
				CHECKTERMINATION;
				output() << "; ";
				tuple = *kt;
				if (tuple.size() > 1) {
					output() << print(tuple[0]);
				}
				for (unsigned int n = 1; n < tuple.size() - 1; ++n) {
					output() << ',' << print(tuple[n]);
				}
				output() << "->" << print(tuple.back());
			}
		}
		output() << " }";
	}



std::string capitalize(std::string str){
    std::string::iterator it(str.begin());
    if (it != str.end())
        str[0] = toupper((unsigned char)str[0]);
    while(++it != str.end()){
        *it = tolower((unsigned char)*it);
    }
    return str;
}
};
