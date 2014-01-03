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
	IDP2Printer(Stream& stream) :
			StreamPrinter<Stream>(stream) {
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

	void visit(const Structure* structure) {
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
				if (pi->ct()->size() == 0 && pi->cf()->size() == 0) {
					continue;
				}
				if (not pi->approxTwoValued()) {
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
					auto pi = fi->graphInter();
					auto ct = pi->ct();
					auto cf = pi->cf();
					if (ct->approxEmpty() && cf->approxEmpty()) {
						continue;
					}
					output() << "Partial: " << '\n'; //TEMPORARY GO TO PARTIAL BLOCK
					printTab();
					auto name = f->nameNoArity();
					name = capitalize(name);
					output() << name << " = ";
					printAsFunc(ct);
					printAsFunc(cf);
					output() << '\n';
					output() << "Data: "; //RETURN TO DATA BLOCK
				}
				output() << '\n';
			}
		}
		unindent();
		output() << '\n';
	}

	template<typename Visitor, typename List>
	void visitList(Visitor v, const List& list) {
		for (auto i = list.cbegin(); i < list.cend(); ++i) {
			CHECKTERMINATION;
			(*i)->accept(v);
		}
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

	std::string capitalize(std::string str) {
		std::string::iterator it(str.begin());
		if (it != str.end())
		str[0] = toupper((unsigned char)str[0]);
		return str;
	}

public:
	void visit(const AbstractGroundTheory*){
		throw notyetimplemented("Printing theories in IDP2 format");
	}
	void visit(const GroundDefinition*){
		throw notyetimplemented("Printing ground definitions in IDP2 format");
	}
	void visit(const Query*) {
		throw notyetimplemented("Printing queries in IDP2 format");
	}
	void visit(const FOBDD*) {
		throw notyetimplemented("Printing fobdds in IDP2 format");
	}
	void visit(const Compound*) {
		throw notyetimplemented("Printing compounds in IDP2 format");
	}
	void visit(const Vocabulary*) {
		throw notyetimplemented("Printing vocabularies in IDP2 format");
	}
	void visit(const Namespace*) {
		throw notyetimplemented("Printing namespaces in IDP2 format");
	}
	void visit(const Theory*) {
		throw notyetimplemented("Printing theories in IDP2 format");
	}
	void visit(const GroundTheory<GroundPolicy>*) {
		throw notyetimplemented("Printing (ground)theories in IDP2 format");
	}
	void visit(const PredForm*) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}
	void visit(const EqChainForm*) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}
	void visit(const EquivForm*) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}
	void visit(const BoolForm*) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}
	void visit(const QuantForm*) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}
	void visit(const AggForm*) {
		throw notyetimplemented("Printing formulas in IDP2 format");
	}
	void visit(const Rule*) {
		throw notyetimplemented("Printing rules in IDP2 format");
	}
	void visit(const Definition*) {
		throw notyetimplemented("Printing definitions in IDP2 format");
	}
	void visit(const FixpDef*) {
		throw notyetimplemented("Printing fixpdefinitions in IDP2 format");
	}
	void visit(const VarTerm*) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}
	void visit(const FuncTerm*) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}
	void visit(const DomainTerm*) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}
	void visit(const AggTerm*) {
		throw notyetimplemented("Printing terms in IDP2 format");
	}
	void visit(const EnumSetExpr*) {
		throw notyetimplemented("Printing set expressions in IDP2 format");
	}
	void visit(const QuantSetExpr*) {
		throw notyetimplemented("Printing set expressions in IDP2 format");
	}
	void visit(const CPSetTerm*) {
		throw notyetimplemented("Printing CP thingies in IDP2 format");
	}
	void visit(const CPVarTerm*) {
		throw notyetimplemented("Printing CP thingies in IDP2 format");
	}
	void visit(const Sort*) {
		throw notyetimplemented("Printing sorts in IDP2 format");
	}
	void visit(const Predicate*) {
		throw notyetimplemented("Printing predicates in IDP2 format");
	}
	void visit(const Function*) {
		throw notyetimplemented("Printing functions in IDP2 format");
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
	virtual void visit(const UserProcedure*) {
		throw notyetimplemented("Printing procedures in IDP2 format");
	}
};
