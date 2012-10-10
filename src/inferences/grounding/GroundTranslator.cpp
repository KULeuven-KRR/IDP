/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GroundTranslator.hpp"
#include "IncludeComponents.hpp"
//#include "grounders/LazyFormulaGrounders.hpp"
#include "grounders/DefinitionGrounders.hpp"
#include "utils/CPUtils.hpp"
#include "utils/ListUtils.hpp"

#include "inferences/grounding/GrounderFactory.hpp"
#include "errorhandling/UnsatException.hpp"

using namespace std;

int verbosity() {
	return getOption(IntType::VERBOSE_GROUNDING);
}

SymbolInfo::SymbolInfo(PFSymbol* symbol, StructureInfo structure)
		: symbol(symbol) {
	inter = structure.concrstructure->inter(symbol);
	GeneratorData data;
	std::vector<Term*> varterms;
	for (auto sort : symbol->sorts()) {
		data.containers.push_back(new const DomElemContainer());
		data.tables.push_back(structure.concrstructure->inter(sort));
		data.fovars.push_back(new Variable(sort));
		varterms.push_back(new VarTerm(data.fovars.back(), TermParseInfo()));
	}
	data.structure = structure.concrstructure;
	containers = data.containers;

	auto pf = new PredForm(SIGN::POS, symbol, varterms, FormulaParseInfo());
	data.pattern = std::vector<Pattern>(data.containers.size(), Pattern::INPUT);
	ptchecker = GrounderFactory::getChecker(pf, TruthType::POSS_TRUE, data, structure.symstructure);
	ctchecker = GrounderFactory::getChecker(pf, TruthType::CERTAIN_TRUE, data, structure.symstructure);
}

FunctionInfo::FunctionInfo(Function* symbol, StructureInfo structure)
		: symbol(symbol) {
	GeneratorData data;
	std::vector<Term*> varterms;
	for (auto sort : symbol->sorts()) {
		data.containers.push_back(new const DomElemContainer());
		data.tables.push_back(structure.concrstructure->inter(sort));
		data.fovars.push_back(new Variable(sort));
		varterms.push_back(new VarTerm(data.fovars.back(), TermParseInfo()));
	}
	data.structure = structure.concrstructure;

	auto pf = new PredForm(SIGN::POS, symbol, varterms, FormulaParseInfo());
	data.pattern = std::vector<Pattern>(data.containers.size()-1, Pattern::INPUT);
	data.pattern.push_back(Pattern::OUTPUT);
	containers = data.containers;
	truerangegenerator = GrounderFactory::getGenerator(pf, TruthType::CERTAIN_TRUE, data, structure.symstructure);
	falserangegenerator = GrounderFactory::getGenerator(pf, TruthType::CERTAIN_FALSE, data, structure.symstructure);
}

GroundTranslator::GroundTranslator(StructureInfo structure, AbstractGroundTheory* grounding)
		: _structure(structure), _grounding(grounding) {

	// Literal 0 is not allowed!
	atomtype.push_back(AtomType::LONETSEITIN);
	atom2Tuple.push_back(NULL);
	atom2TsBody.push_back((TsBody*) NULL);
}

GroundTranslator::~GroundTranslator() {
	deleteList<TsBody>(atom2TsBody);
	deleteList<CPTsBody>(var2CTsBody);
	deleteList<stpair>(atom2Tuple);
	deleteList<ftpair>(var2Tuple);
}

Lit GroundTranslator::translateReduced(PFSymbol* s, const ElementTuple& args, bool recursive) {
	auto offset = addSymbol(s);
	return translateReduced(offset, args, recursive);
}

void GroundTranslator::addKnown(VarId id){
	if(not hasVarIdMapping(id)){
		return;
	}

	auto symbol = getFunction(id);
	auto offset = addSymbol(symbol);
	auto& funcinfo = functions[offset.offset];
	auto args = getArgs(id);
	for(uint i=0; i<args.size(); ++i){
		Assert(not args[i].isVariable); // FIXME check if correct?
		*funcinfo.containers[i]=args[i]._domelement;
	}
	for(funcinfo.truerangegenerator->begin(); not funcinfo.truerangegenerator->isAtEnd(); funcinfo.truerangegenerator->operator ++()){
		Assert(funcinfo.containers.back()->get()->type()==DomainElementType::DET_INT); // NOTE: change if we can handle more then ints!
		CPBound bound(funcinfo.containers.back()->get()->value()._int);
		auto lit = reify(new CPVarTerm(id), CompType::NEQ, bound, TsType::IMPL);
		Assert(lit!=_true && lit!=_false);
		_grounding->addUnitClause(lit);
	}
	for(funcinfo.falserangegenerator->begin(); not funcinfo.falserangegenerator->isAtEnd(); funcinfo.falserangegenerator->operator ++()){
		Assert(funcinfo.containers.back()->get()->type()==DomainElementType::DET_INT); // NOTE: change if we can handle more then ints!
		CPBound bound(funcinfo.containers.back()->get()->value()._int);
		auto lit = reify(new CPVarTerm(id), CompType::NEQ, bound, TsType::RIMPL);
		Assert(lit!=_true && lit!=_false);
		_grounding->addUnitClause(-lit);
	}
}

TruthValue GroundTranslator::checkApplication(const DomainElement* domelem, SortTable* predtable, SortTable* termtable, Context funccontext, SIGN sign) {
	// Check partial functions
	if (domelem == NULL) {
		TruthValue result;
		if (funccontext == Context::POSITIVE) {
			result = TruthValue::True;
		} else if (funccontext == Context::NEGATIVE) {
			result = TruthValue::True;
		} else {
			throw IdpException("Could not find out the semantics of an ambiguous partial term. Please specify the meaning.");
		}
		if (verbosity() > 2) {
			std::clog << tabs() << "Partial function went out of bounds\n";
		}
		return result;
	}

	// Checking out-of-bounds
	if (predtable != termtable && not predtable->contains(domelem)) {
		if (verbosity() > 2) {
			std::clog << tabs() << "Term value out of predicate type" << "\n"; //TODO should be a warning
		}

		return isPos(sign) ? TruthValue::False : TruthValue::True;
	}

	return TruthValue::Unknown;
}

/*
 * TODO it might be interesting to see which is faster: first executing the checkers and if they return no answer, search/create the literal
 * 			or first search for the literal, and run the checkers if it was not yet grounded.
 */
Lit GroundTranslator::translateReduced(SymbolOffset offset, const ElementTuple& args, bool recursivecontext) { // reduction should not be allowed in recursive context or when reducedgrounding is off
	auto& symboldata = symbols[offset.offset];
	Assert(symboldata.ctchecker!=NULL);

	bool littrue = false, litfalse = false;
	for(uint i=0; i<args.size(); ++i){
		*symboldata.containers[i] = args[i];
	}
	if (symboldata.ctchecker->check()) { // Literal is irrelevant in its occurrences
		if (verbosity() > 2) {
			std::clog << tabs() << "Certainly true checker succeeded" << "\n";
		}
		littrue = true;
	}
	if (not symboldata.ptchecker->check()) { // Literal decides formula if checker succeeds
		if (verbosity() > 2) {
			std::clog << tabs() << "Possibly true checker failed" << "\n";
		}
		litfalse = true;
	}
	if (not littrue && symboldata.inter->isTrue(args)) {
		littrue = true;
	}
	if (not litfalse && symboldata.inter->isFalse(args)) {
		litfalse = true;
	}
	if (littrue && litfalse) {
		_grounding->addUnitClause(1);
		_grounding->addUnitClause(-1); // TODO Remove when unsatexception is handled everywhere
		throw UnsatException();
	}

	if (getOption(REDUCEDGROUNDING) && not recursivecontext) {
		if (littrue) {
			return _true;
		} else if (litfalse) {
			return _false;
		}
	}

	auto lit = getLiteral(offset, args);
	Assert(lit!=_true && lit!=_false);
	if (littrue) {
		_grounding->addUnitClause(lit);
	} else if (litfalse) {
		_grounding->addUnitClause(-lit);
	}
	return lit;
}

// TODO this can probably be optimized by storing _true/_false wherever applicable instead of lit
Lit GroundTranslator::getLiteral(SymbolOffset symboloffset, const ElementTuple& args) {
	if (symboloffset.functionlist) {
		std::vector<GroundTerm> terms;
		for (auto i = args.cbegin(); i < args.cend(); ++i) {
			terms.push_back(*i);
		}
		auto image = terms.back();
		Assert(image._domelement->type()==DomainElementType::DET_INT);
		// Otherwise, cannot be a cp-able function
		auto bound = image._domelement->value()._int;
		terms.pop_back();
		auto lit = reify(new CPVarTerm(translateTerm(symboloffset, terms)), CompType::EQ, CPBound(bound), TsType::EQ); // TODO TSType?
		atom2Tuple[lit]->first = functions[symboloffset.offset].symbol;
		atom2Tuple[lit]->second = args;
		atomtype[lit] = AtomType::CPGRAPHEQ;
		return lit;
	}else{
		Lit lit = 0;
		auto& symbolinfo = symbols[symboloffset.offset];
		auto jt = symbolinfo.tuple2atom.find(args);
		if (jt != symbolinfo.tuple2atom.end()) {
			lit = jt->second;
			jt->second = lit;
		} else {
			lit = nextNumber(AtomType::INPUT);
			atom2Tuple[lit]->first = symbolinfo.symbol;
			atom2Tuple[lit]->second = args;
			symbolinfo.tuple2atom.insert(jt, Tuple2Atom { args, lit });
		}

		return lit;
	}
}

Vocabulary* GroundTranslator::vocabulary() const {
	return _structure.concrstructure == NULL ? NULL : _structure.concrstructure->vocabulary();
}

// TODO expensive!
SymbolOffset GroundTranslator::getSymbol(PFSymbol* pfs) const {
	if (pfs->isFunction()) {
		auto function = dynamic_cast<Function*>(pfs);
		if (function != NULL && getOption(CPSUPPORT) && CPSupport::eligibleForCP(function, vocabulary())) {
			for (size_t n = 0; n < functions.size(); ++n) {
				if (functions[n].symbol == pfs) {
					return SymbolOffset(n, true);
				}
			}
		}
	}
	for (size_t n = 0; n < symbols.size(); ++n) {
		if (symbols[n].symbol == pfs) {
			return SymbolOffset(n, false);
		}
	}
	return SymbolOffset(-1, false);
}

SymbolOffset GroundTranslator::addSymbol(PFSymbol* pfs) {
	auto n = getSymbol(pfs);
	if (n.offset == -1) {
		if (pfs->isFunction()) {
			auto function = dynamic_cast<Function*>(pfs);
			if (function != NULL && getOption(CPSUPPORT) && CPSupport::eligibleForCP(function, vocabulary())) {
				functions.push_back(FunctionInfo(function, _structure));
				return SymbolOffset(functions.size() - 1, true);
			}
		}
		symbols.push_back(SymbolInfo(pfs, _structure));
		return SymbolOffset(symbols.size() - 1, false);

	} else {
		return n;
	}
}

Lit GroundTranslator::reify(const litlist& clause, bool conj, TsType tstype) {
	int head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	auto tsbody = new PCTsBody(tstype, clause, conj);
	atom2TsBody[head] = tsbody;
	return head;
}

bool GroundTranslator::canBeDelayedOn(PFSymbol* pfs, Context, DefId) const {
	auto symboloffset = getSymbol(pfs);
	Assert(not symboloffset.functionlist);
	auto symbolId = symboloffset.offset;
	if (symbolId == -1) { // there is no such symbol yet
		return true;
	}
	auto& grounders = symbols[symbolId].assocGrounders;
	if (grounders.empty()) {
		return true;
	}
	throw notyetimplemented("Checking allowed delays");
	/*	for (auto i = grounders.cbegin(); i < grounders.cend(); ++i) {
	 if (context == Context::BOTH) { // If unknown-delay, can only delay if in same DEFINITION
	 if (id == -1 || (*i)->getID() != id) {
	 return false;
	 }
	 } else if ((*i)->getContext() != context) { // If true(false)-delay, can delay if we do not find any false(true) or unknown delay
	 return false;
	 }
	 }
	 return true;*/
}

void GroundTranslator::notifyDelay(PFSymbol*, DelayGrounder* const) {
	//Assert(grounder!=NULL);
	//clog <<"Notified that symbol " <<toString(pfs) <<" is defined on id " <<grounder->getID() <<".\n";
	throw notyetimplemented("Notifying of delays");
	/*	auto symbolID = addSymbol(pfs);
	 Assert(not symbolID.functionlist);
	 auto& grounders = symbols[symbolID.offset].assocGrounders;
	 #ifndef NDEBUG
	 Assert(canBeDelayedOn(pfs, grounder->getContext(), grounder->getID()));
	 for (auto i = grounders.cbegin(); i < grounders.cend(); ++i) {
	 Assert(grounder != *i);
	 }
	 #endif
	 grounders.push_back(grounder);*/
}

Lit GroundTranslator::reify(LazyInstantiation* instance, TsType tstype) {
	auto tseitin = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	auto tsbody = new LazyTsBody(instance, tstype);
	atom2TsBody[tseitin] = tsbody;
	return tseitin;
}

Lit GroundTranslator::reify(double bound, CompType comp, AggFunction aggtype, SetId setnr, TsType tstype) {
	if (comp == CompType::EQ) {
		auto l = reify(bound, CompType::LEQ, aggtype, setnr, tstype);
		auto l2 = reify(bound, CompType::GEQ, aggtype, setnr, tstype);
		return reify( { l, l2 }, true, tstype);
	} else if (comp == CompType::NEQ) {
		auto l = reify(bound, CompType::GT, aggtype, setnr, tstype);
		auto l2 = reify(bound, CompType::LT, aggtype, setnr, tstype);
		return reify( { l, l2 }, false, tstype);
	} else {
		auto head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		if (comp == CompType::LT) {
			bound += 1;
			comp = CompType::LEQ;
		} else if (comp == CompType::GT) {
			bound -= 1;
			comp = CompType::GEQ;
		}
		MAssert(comp==CompType::LEQ || comp==CompType::GEQ);
		auto tsbody = new AggTsBody(tstype, bound, comp == CompType::LEQ, aggtype, setnr);
		atom2TsBody[head] = tsbody;
		return head;
	}
}

bool CompareTs::operator()(CPTsBody* left, CPTsBody* right) {
	if (left == NULL) {
		if (right == NULL) {
			return false;
		}
		return true;
	} else if (right == NULL) {
		return false;
	}
	return *left < *right;
}

Lit GroundTranslator::reify(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	auto tsbody = new CPTsBody(tstype, left, comp, right);
	// TODO => this should be generalized to sharing detection!
	auto it = cpset.find(tsbody);
	if (it != cpset.cend()) {
		delete tsbody;
		if (it->first->comp() != comp) { // NOTE: OPTIMIZATION! = and ~= map to the same tsbody etc. => look at ecnf.cpp:compEqThroughNeg
			return -it->second;
		} else {
			return it->second;
		}
	} else {
		int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		atom2TsBody[nr] = tsbody;
		cpset[tsbody] = nr;
		return nr;
	}
}

//// Adds a tseitin body only if it does not yet exist. TODO why does this seem only relevant for CP Terms?
//Lit GroundTranslator::addTseitinBody(TsBody* tsbody) {
//// TODO optimization: check whether the same comparison has already been added and reuse the tseitin.
//	auto it = _tsbodies2nr.lower_bound(tsbody);
//	if (it != _tsbodies2nr.cend() && *(it->first) == *tsbody) { // Already exists
//		delete tsbody;
//		return it->second;
//	}
//	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
//	atom2TsBody[nr] = tspair(nr, tsbody);
//	return nr;
//}

SetId GroundTranslator::translateSet(const litlist& lits, const weightlist& weights, const weightlist& trueweights, const termlist& cpvars) {
	TsSet tsset;
	tsset._setlits = lits;
	tsset._litweights = weights;
	tsset._trueweights = trueweights;
	tsset._cpvars = cpvars;
	auto setnr = _sets.size();
	_sets.push_back(tsset);
	return setnr;
}

Lit GroundTranslator::nextNumber(AtomType type) {
	Lit nr = atomtype.size();
	atom2TsBody.push_back(NULL);
	atom2Tuple.push_back(new stpair());
	atomtype.push_back(type);
	return nr;
}

VarId GroundTranslator::translateTerm(SymbolOffset offset, const vector<GroundTerm>& args) {
	Assert(offset.functionlist);
	auto& info = functions[offset.offset];
	auto it = info.term2var.lower_bound(args);
	if (it != info.term2var.cend() && it->first == args) {
		return it->second;
	} else {
		VarId varid = nextNumber();
		info.term2var.insert(it, pair<vector<GroundTerm>, VarId> { args, varid });
		auto ft = new ftpair(info.symbol, args);
		var2Tuple[varid.id] = ft;
		var2domain[varid.id] = _structure.concrstructure->inter(info.symbol->outsort());
		return varid;
	}
}

VarId GroundTranslator::translateTerm(Function* function, const vector<GroundTerm>& args) {
	Assert(CPSupport::eligibleForCP(function, vocabulary()));
	auto offset = addSymbol(function);
	return translateTerm(offset, args);
}

VarId GroundTranslator::translateTerm(CPTerm* cpterm, SortTable* domain) {
	auto varid = nextNumber();
	CPBound bound(varid);
	auto cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	var2CTsBody[varid.id] = cprelation;
	var2domain[varid.id] = domain;
	return varid;
}

VarId GroundTranslator::translateTerm(const DomainElement* element) {
	Assert(element!=NULL && element->type() == DET_INT);
	auto value = element->value()._int;

	auto it = storedTerms.find(value);
	if (it != storedTerms.cend()) {
		return it->second;
	}

	auto varid = nextNumber();
	auto cpterm = new CPVarTerm(varid);
	CPBound bound(value);
	// Add a new CP constraint
	auto cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	var2CTsBody[varid.id] = cprelation;
	// Add a new domain containing only the given domain element
	auto domain = TableUtils::createSortTable(value, value);
	var2domain[varid.id] = domain;
	storedTerms.insert( { value, varid });
	return varid;
}

VarId GroundTranslator::nextNumber() {
	VarId id;
	id.id = var2Tuple.size();
	var2Tuple.push_back(NULL);
	var2CTsBody.push_back(NULL);
	var2domain.push_back(NULL);
	return id;
}

string GroundTranslator::printTerm(const VarId& varid) const {
	if (varid.id >= var2Tuple.size()) {
		return "error";
	}
	if (not hasVarIdMapping(varid)) {
		stringstream s;
		s << "var_" << varid;
		return s.str();
	}
	auto func = getFunction(varid);
	stringstream s;
	s << toString(func);
	if (not getArgs(varid).empty()) {
		s << "(";
		for (auto gtit = getArgs(varid).cbegin(); gtit != getArgs(varid).cend(); ++gtit) {
			if ((*gtit).isVariable) {
				s << printTerm((*gtit)._varid);
			} else {
				s << toString((*gtit)._domelement);
			}
			if (gtit != getArgs(varid).cend() - 1) {
				s << ",";
			}
		}
		s << ")";
	}
	return s.str();
}

string GroundTranslator::print(Lit lit) {
	return printLit(lit);
}

string GroundTranslator::printLit(const Lit& lit) const {
	stringstream s;
	Lit nr = lit;
	if (nr == _true) {
		return "true";
	}
	if (nr == _false) {
		return "false";
	}
	if (nr < 0) {
		s << "~";
		nr = -nr;
	}
	if (not isStored(nr)) {
		return "error";
	}

	switch (atomtype[nr]) {
	case AtomType::INPUT: {
		PFSymbol* pfs = getSymbol(nr);
		s << toString(pfs);
		auto tuples = getArgs(nr);
		if (not tuples.empty()) {
			s << "(";
			bool begin = true;
			for (auto i = tuples.cbegin(); i != tuples.cend(); ++i) {
				if (not begin) {
					s << ", ";
				}
				begin = false;
				s << toString(*i);
			}
			s << ")";
		}
		break;
	}
	case AtomType::CPGRAPHEQ:
	case AtomType::TSEITINWITHSUBFORMULA:
		s << "tseitin_" << nr;
		break;
	case AtomType::LONETSEITIN:
		s << "tseitin_" << nr;
		break;
	}
	return s.str();
}
