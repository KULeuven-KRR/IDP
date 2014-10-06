#pragma once

#include "IncludeComponents.hpp"
#include "utils/ContainerUtils.hpp"
#include "inferences/SolverConnection.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

using namespace SolverConnection;

template<class Executor>
class IDP2ECNF {
private:
	std::set<VarId> _addedvarids;
	Executor execute;

	GroundTranslator* translator;

public:
	IDP2ECNF(): translator(NULL){}
	IDP2ECNF(Executor exec, GroundTranslator* translator)
			: 	execute(exec),
				translator(translator) {
	}

	void setExec(Executor exec){
		execute = exec;
	}
	void setTranslator(GroundTranslator* tl){
		translator = tl;
	}

	Executor& getExec() {
		return execute;
	}

	void addFDVariable(const VarId& varid) {
		if (contains(_addedvarids, varid)) {
			return;
		}
		_addedvarids.insert(varid);
		auto domain = translator->domain(varid);
		Assert(domain != NULL);

		if (domain->isRange()) {
			if (not domain->approxFinite()) {
				Warning::warning(
						"Approximating int as all integers in -2^32..2^32, as the solver does not support true infinity at the moment. Models might be lost.");
			}
			// the domain is a complete range from minvalue to maxvalue.
			auto nonden = translator->getNonDenoting(varid);
			if (nonden == translator->falseLit()) {
				execute(MinisatID::IntVarRange(convert(varid), domain->first()->value()._int, domain->last()->value()._int));
			} else {
				execute(MinisatID::IntVarRange(convert(varid), domain->first()->value()._int, domain->last()->value()._int, createLiteral(nonden)));
			}
		} else {
			// the domain is not a complete range.
			std::vector<MinisatID::Weight> w;
			for (auto it = domain->sortBegin(); not it.isAtEnd(); ++it) {
				CHECKTERMINATION;
				w.push_back((MinisatID::Weight) (*it)->value()._int);
			}
			auto nonden = translator->getNonDenoting(varid);
			if (nonden == translator->falseLit()) {
				execute(MinisatID::IntVarEnum(convert(varid), w));
			} else {
				execute(MinisatID::IntVarEnum(convert(varid), w, createLiteral(nonden)));
			}
		}
	}
	void addFDVariables(const varidlist& varids) {
		for (auto varid : varids) {
			addFDVariable(varid);
		}
	}

	void add(const GroundClause& cl) {
		execute(MinisatID::Disjunction(createList(cl)));
	}

	void addLazyAddition(const litlist& glist, int constraintID) {
		execute(MinisatID::LazyAddition(createList(glist), constraintID));
	}

	void add(DefId defnr, const PCGroundRule& rule) {
		MinisatID::litlist list;
		for (auto lit : rule.body()) {
			list.push_back(createLiteral(lit));
		}
		execute(MinisatID::Rule(createAtom(rule.head()), list, rule.type() == RuleType::CONJ, defnr.id, useUFSAndOnlyIfSem()));
	}

	void add(DefId defID, Lit head, double bound, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem) {
		auto sign = lowerbound ? MinisatID::AggSign::LB : MinisatID::AggSign::UB;
		auto msem = MinisatID::AggSem::COMP;
		switch (sem) {
		case TsType::EQ:
			break;
		case TsType::IMPL: // not head or aggregate
			msem = MinisatID::AggSem::OR;
			head = -head;
			break;
		case TsType::RIMPL: // head or not aggregate
			msem = MinisatID::AggSem::OR;
			if (sign == MinisatID::AggSign::LB) {
				sign = MinisatID::AggSign::UB;
				bound -= 1;
			} else {
				sign = MinisatID::AggSign::LB;
				bound += 1;
			}
			break;
		case TsType::RULE:
			msem = MinisatID::AggSem::DEF;
			break;
		}
		execute(
				MinisatID::Aggregate(createLiteral(head), setnr.id, createWeight(bound), convert(aggtype), sign, msem, defID.id,
						useUFSAndOnlyIfSem()));
	}

	void addOptimization(AggFunction function, SetId setid) {
		execute(MinisatID::MinimizeAgg(1, setid.id, convert(function)));
	}
	void addOptimization(VarId varid) {
		addFDVariable(varid);
		execute(MinisatID::OptimizeVar(1, convert(varid), true));
	}

	void add(Lit, VarId varid){
		addFDVariable(varid);
	}

	void add(Lit tseitin, TsType type, const GroundClause rhs, bool conjunctive) {
		auto newtseitin = tseitin;
		MinisatID::ImplicationType impltype = MinisatID::ImplicationType::IMPLIES;
		auto newconj = conjunctive;
		auto newrhs = rhs;
		switch (type) {
		case TsType::RULE:
			throw IdpException("Invalid code path");
			break;
		case TsType::IMPL:
			impltype = MinisatID::ImplicationType::IMPLIES;
			break;
		case TsType::RIMPL:
			for (uint i = 0; i < rhs.size(); ++i) {
				newrhs[i] = -rhs[i];
			}
			newtseitin = -newtseitin;
			newconj = not newconj;
			impltype = MinisatID::ImplicationType::IMPLIES;
			break;
		case TsType::EQ:
			impltype = MinisatID::ImplicationType::EQUIVALENT;
			break;
		}
		execute(MinisatID::Implication(createLiteral(newtseitin), impltype, createList(newrhs), newconj));
	}

	void add(SetId setnr, const litlist& lits, bool weighted, const weightlist& weights) {
		MinisatID::WLSet set(setnr.id);
		if (not weighted) {
			for (size_t n = 0; n < lits.size(); ++n) {
				set.wl.push_back(MinisatID::WLtuple { createLiteral(lits[n]), MinisatID::Weight(1) });
			}
		} else {
			for (size_t n = 0; n < lits.size(); ++n) {
				set.wl.push_back(MinisatID::WLtuple { createLiteral(lits[n]), createWeight(weights[n]) });
			}
		}
		execute(set);
	}

	void add(Lit tseitin, CPTsBody* body) {
		MinisatID::EqType comp = MinisatID::EqType::EQ;
		switch (body->comp()) {
		case CompType::EQ:
			comp = MinisatID::EqType::EQ;
			break;
		case CompType::NEQ:
			comp = MinisatID::EqType::NEQ;
			break;
		case CompType::LEQ:
			comp = MinisatID::EqType::LEQ;
			break;
		case CompType::GEQ:
			comp = MinisatID::EqType::GEQ;
			break;
		case CompType::LT:
			comp = MinisatID::EqType::L;
			break;
		case CompType::GT:
			comp = MinisatID::EqType::G;
			break;
		}
		CPTerm* left = body->left();
		CPBound right = body->right();
		if (isa<CPVarTerm>(*left)) {
			auto term = dynamic_cast<CPVarTerm*>(left);
			addFDVariable(term->varid());
			if (right._isvarid) {
				addFDVariable(right._varid);
				execute(MinisatID::CPBinaryRelVar(createLiteral(tseitin), convert(term->varid()), comp, convert(right._varid)));
			} else {
				execute(MinisatID::CPBinaryRel(createLiteral(tseitin), convert(term->varid()), comp, right._bound));
			}
		} else if (isa<CPSetTerm>(*left)) {
			auto term = dynamic_cast<CPSetTerm*>(left);

			switch (term->type()) {
			case AggFunction::SUM: {
				addFDVariables(term->varids());

				auto varids = term->varids();
				auto weights = term->weights();
				auto conditions = term->conditions();
				auto bound = 0;
				if (right._isvarid) {
					bound = 0;
					varids.push_back(right._varid);
					weights.push_back(-1);
					conditions.push_back(translator->trueLit());
				} else {
					bound = right._bound;
				}
				polAddWeightedSum(createAtom(tseitin), conditions, varids, weights, bound, comp);
				break;
			}
			case AggFunction::PROD: {
				VarId rhsvarid;
				if (right._isvarid) {
					rhsvarid = right._varid;
				} else {
					rhsvarid = translator->translateTerm(createDomElem(right._bound));
				}

				polAddWeightedProd(createAtom(tseitin), term->conditions(), term->varids(), term->weights().back(), rhsvarid, comp);
				break;
			}
			case AggFunction::MIN: {
				VarId rhsvarid;
				if (right._isvarid) {
					rhsvarid = right._varid;
				} else {
					rhsvarid = translator->translateTerm(createDomElem(right._bound));
				}

				addFDVariable(rhsvarid);
				addFDVariables(term->varids());

#define condlist(tseitin, comp,conj) add(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), comp, rhsvarid, conj), conj);

				/**
				 * ==
				 * 		forall setelem: elem >= rightvar
				 * 		exists setelem: elem =< rightvar
				 * ~=
				 * 		exists setelem: elem > rightvar  |
				 * 				forall setelem: elem < rightvar
				 * <
				 * 		forall setelem: elem < rightvar
				 * >
				 * 		exists setelem: elem > rightvar
				 * =<
				 * 		forall setelem: elem =< rightvar
				 * >=
				 * 		exists setelem: elem >= rightvar
				 */
				switch (body->comp()) {
				case CompType::EQ: {
					auto ts1 = translator->createNewUninterpretedNumber(), ts2 = translator->createNewUninterpretedNumber();
					condlist(ts1, MinisatID::EqType::GEQ, true);
					condlist(ts2, MinisatID::EqType::LEQ, false);
					add(tseitin, TsType::EQ, { ts1, ts2 }, true);
					break;
				}
				case CompType::NEQ: {
					auto ts1 = translator->createNewUninterpretedNumber(), ts2 = translator->createNewUninterpretedNumber();
					condlist(ts1, MinisatID::EqType::G, false);
					condlist(ts2, MinisatID::EqType::L, true);
					add(tseitin, TsType::EQ, { ts1, ts2 }, false);
					break;
				}
				case CompType::LEQ:
					condlist(tseitin, MinisatID::EqType::LEQ, false);
					break;
				case CompType::GEQ:
					condlist(tseitin, MinisatID::EqType::GEQ, true);
					break;
				case CompType::LT:
					condlist(tseitin, MinisatID::EqType::L, false);
					break;
				case CompType::GT:
					condlist(tseitin, MinisatID::EqType::G, true);
					break;
				}
				break;
			}
			case AggFunction::MAX: {
				VarId rhsvarid;
				if (right._isvarid) {
					rhsvarid = right._varid;
				} else {
					rhsvarid = translator->translateTerm(createDomElem(right._bound));
				}

				addFDVariable(rhsvarid);
				addFDVariables(term->varids());

				switch (body->comp()) {
				case CompType::EQ: {
					auto ts1 = translator->createNewUninterpretedNumber(), ts2 = translator->createNewUninterpretedNumber();
					condlist(ts1, MinisatID::EqType::LEQ, true);
					condlist(ts2, MinisatID::EqType::GEQ, false);
					add(tseitin, TsType::EQ, { ts1, ts2 }, true);
					break;
				}
				case CompType::NEQ: {
					auto ts1 = translator->createNewUninterpretedNumber(), ts2 = translator->createNewUninterpretedNumber();
					condlist(ts1, MinisatID::EqType::L, false);
					condlist(ts2, MinisatID::EqType::G, true);
					add(tseitin, TsType::EQ, { ts1, ts2 }, false);
					break;
				}
				case CompType::LEQ:
					condlist(tseitin, MinisatID::EqType::LEQ, true);
					break;
				case CompType::GEQ:
					condlist(tseitin, MinisatID::EqType::GEQ, false);
					break;
				case CompType::LT:
					condlist(tseitin, MinisatID::EqType::L, true);
					break;
				case CompType::GT:
					condlist(tseitin, MinisatID::EqType::G, false);
					break;
				}
				break;
			}
			default:
				throw IdpException("Invalid code path");
			}
		}
	}

	void polAddWeightedSum(const MinisatID::Atom& head, const litlist& conditions, const varidlist& varids, const intweightlist& weights, const int& bound,
			MinisatID::EqType rel) {
		addFDVariables(varids);

		std::vector<MinisatID::Weight> w;
		for (auto i : weights) {
			w.push_back(MinisatID::Weight(i));
		}
		std::vector<MinisatID::VarID> vars;
		for (auto var : varids) {
			vars.push_back(convert(var));
		}
		execute(MinisatID::CPSumWeighted(MinisatID::mkPosLit(head), createList(conditions), vars, w, rel, bound));
	}

	void polAddWeightedProd(const MinisatID::Atom& head, const litlist& conditions, const varidlist& varids, const int& weight, VarId bound,
			MinisatID::EqType rel) {
		addFDVariables(varids);
		addFDVariable(bound);

		MinisatID::Weight w = weight;
		std::vector<MinisatID::VarID> vars;
		for (auto var : varids) {
			vars.push_back(convert(var));
		}
		execute(MinisatID::CPProdWeighted(MinisatID::mkPosLit(head), createList(conditions), vars, w, rel, convert(bound)));
	}

	/**
	 * Generates a list of literals which enforce some comparison whenever the associated condition is true
	 */
	litlist getConditionalComparisonList(const litlist& conditions, const varidlist& varids, MinisatID::EqType comp, VarId rhsvarid, bool forall) {
		litlist tseitins;
		for (uint i = 0; i < varids.size(); ++i) {
			auto impltseitin = translator->createNewUninterpretedNumber();
			auto comptseitin = translator->createNewUninterpretedNumber();
			tseitins.push_back(impltseitin);
			execute(MinisatID::CPBinaryRelVar(createLiteral(comptseitin), convert(varids[i]), comp, convert(rhsvarid)));
			auto cond = conditions[i];
			if(forall){
				add(impltseitin, TsType::EQ, { -cond, comptseitin }, false);
			}else{
				add(impltseitin, TsType::EQ, { cond, comptseitin }, true);
			}
		}
		return tseitins;
	}
};
