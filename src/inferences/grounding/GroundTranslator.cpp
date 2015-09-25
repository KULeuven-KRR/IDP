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

#include "GroundTranslator.hpp"
#include "IncludeComponents.hpp"
#include "grounders/DefinitionGrounders.hpp"
#include "LazyGroundingManager.hpp"
#include "utils/CPUtils.hpp"
#include "utils/ListUtils.hpp"

#include "inferences/grounding/GrounderFactory.hpp"
#include "errorhandling/UnsatException.hpp"
#include "generators/UnionGenerator.hpp"
#include "generators/GeneratorFactory.hpp"

using namespace std;

int verbosity() {
	return getOption(IntType::VERBOSE_GROUNDING);
}

CheckerInfo::CheckerInfo(PFSymbol* symbol, StructureInfo structure) {
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
	data.funccontext = Context::POSITIVE;
	ptchecker = GrounderFactory::getChecker(pf, TruthType::POSS_TRUE, data, structure.symstructure, structure.concrstructure, { }, true);
	ctchecker = GrounderFactory::getChecker(pf, TruthType::CERTAIN_TRUE, data, structure.symstructure, structure.concrstructure, { }, true);
}

CheckerInfo::~CheckerInfo() {
	delete (ptchecker);
	delete (ctchecker);
}

SymbolInfo::SymbolInfo(PFSymbol* symbol, StructureInfo structure)
		: 	symbol(symbol),
			checkers(new CheckerInfo(symbol, structure)) {
}

SymbolInfo::~SymbolInfo(){
	delete(checkers);
}

FunctionInfo::FunctionInfo(Function* symbol, StructureInfo structure)
		: 	symbol(symbol),
			checkers(new CheckerInfo(symbol, structure)) {
	GeneratorData data;
	std::vector<Term*> varterms;
	for (uint i = 0; i < symbol->sorts().size(); ++i) {
		auto sort = symbol->sorts()[i];
		data.tables.push_back(structure.concrstructure->inter(sort));
		auto var = new Variable(sort);
		varterms.push_back(new VarTerm(var, TermParseInfo()));
		data.fovars.push_back(var);
	}
	data.containers = checkers->containers;
	data.structure = structure.concrstructure;
	data.quantfovars.push_back(data.fovars.back());
	data.funccontext = Context::POSITIVE;
	auto pf = new PredForm(SIGN::POS, symbol, varterms, FormulaParseInfo());
	auto pattern = std::vector<Pattern>(data.containers.size() - 1, Pattern::INPUT);
	pattern.push_back(Pattern::OUTPUT);
	auto allinput = std::vector<Pattern>(data.containers.size(), Pattern::INPUT);
	auto symbolictruegenerator = GrounderFactory::getGenerator(pf, TruthType::CERTAIN_TRUE, data, pattern, structure.symstructure, structure.concrstructure, { }, true);
	auto symbolictruechecker = GrounderFactory::getChecker(pf, TruthType::CERTAIN_TRUE, data, structure.symstructure, structure.concrstructure, { }, true);
	auto symbolicfalsegenerator = GrounderFactory::getGenerator(pf, TruthType::CERTAIN_FALSE, data, pattern, structure.symstructure, structure.concrstructure, { }, true);
	auto symbolicfalsechecker = GrounderFactory::getChecker(pf, TruthType::CERTAIN_FALSE, data, structure.symstructure, structure.concrstructure, { }, true);
	auto concretetruegenerator = GeneratorFactory::create(checkers->inter->ct(), pattern, checkers->containers, Universe(data.tables));
	auto concretetruechecker = GeneratorFactory::create(checkers->inter->ct(), allinput, checkers->containers, Universe(data.tables));
	auto concretefalsegenerator = GeneratorFactory::create(checkers->inter->cf(), pattern, checkers->containers, Universe(data.tables));
	auto concretefalsechecker = GeneratorFactory::create(checkers->inter->cf(), allinput, checkers->containers, Universe(data.tables));
	truerangegenerator = new UnionGenerator( { symbolictruegenerator, concretetruegenerator }, { symbolictruechecker, concretetruechecker });
	falserangegenerator = new UnionGenerator( { symbolicfalsegenerator, concretefalsegenerator }, { symbolicfalsechecker, concretefalsechecker });
	delete (pf);
}
FunctionInfo::~FunctionInfo() {
	delete (truerangegenerator);
	delete (falserangegenerator);
}

GroundTranslator::GroundTranslator(StructureInfo structure, AbstractGroundTheory* grounding)
		: 	_structure(structure),
			_grounding(grounding),
			_groundingmanager(NULL),
			_trueLit(0), // IMPORTANT: _trueLit set on initialize!
			maxquantsetid(1) {

	// Literal 0 is not allowed!
	atomtype.push_back(AtomType::LONETSEITIN);
	atom2Tuple.push_back(NULL);
	atom2TsBody.push_back((TsBody*) NULL);
}

Lit GroundTranslator::getNonDenoting(const VarId& varid) const{
	return var2partial.at(varid.id);
}

void GroundTranslator::initialize(){
	_trueLit = createNewUninterpretedNumber();
	_grounding->addUnitClause(_trueLit);
}

GroundTranslator::~GroundTranslator() {
	deleteList<TsBody>(atom2TsBody);
	deleteList<CPTsBody>(var2CTsBody);
	deleteList<stpair>(atom2Tuple);
	deleteList<ftpair>(var2Tuple);
	deleteList<FunctionInfo>(functions);
	deleteList<SymbolInfo>(symbols);
}

void GroundTranslator::removeTsBody(Lit atom){
	auto body = atom2TsBody[atom];
	if(not isa<CPTsBody>(*body)){
		delete(body);
		atom2TsBody[atom] = NULL;
	}
}

Lit GroundTranslator::translateNonReduced(PFSymbol* symbol, const ElementTuple& args){
	auto offset = addSymbol(symbol);
	return translate(offset, args, false);
}

Lit GroundTranslator::translateReduced(PFSymbol* s, const ElementTuple& args, bool recursive) {
	auto offset = addSymbol(s);
	return translate(offset, args, getOption(REDUCEDGROUNDING) && not recursive);
}

Lit GroundTranslator::translateReduced(const SymbolOffset& s, const ElementTuple& args, bool recursive) {
	return translate(s, args, getOption(REDUCEDGROUNDING) && not recursive);
}

void GroundTranslator::addKnown(VarId id) {
	if (not hasVarIdMapping(id)) {
		return;
	}

	auto symbol = getFunction(id);
	auto offset = addSymbol(symbol);
	auto& funcinfo = *functions[offset.offset];
	auto args = getArgs(id);
	auto& containers = funcinfo.checkers->containers;
	for (uint i = 0; i < args.size(); ++i) {
		Assert(not args[i].isVariable);
		// TODO verify that this can never happen (shouldnt this be domelem instead of groundterms then?)
		*containers[i] = args[i]._domelement;
	}
	for (funcinfo.truerangegenerator->begin(); not funcinfo.truerangegenerator->isAtEnd(); funcinfo.truerangegenerator->operator ++()) {
		Assert(containers.back()->get()->type()==DomainElementType::DET_INT);
		// NOTE: change if we can handle more then ints!
		CPBound bound(containers.back()->get()->value()._int);
		auto lit = reify(new CPVarTerm(id), CompType::EQ, bound, TsType::IMPL);
		Assert(lit!=trueLit() && lit!=falseLit());
		_grounding->addUnitClause(lit);
	}
	for (funcinfo.falserangegenerator->begin(); not funcinfo.falserangegenerator->isAtEnd(); funcinfo.falserangegenerator->operator ++()) {
		Assert(containers.back()->get()->type()==DomainElementType::DET_INT);
		// NOTE: change if we can handle more then ints!
		CPBound bound(containers.back()->get()->value()._int);
		auto lit = reify(new CPVarTerm(id), CompType::EQ, bound, TsType::RIMPL);
		Assert(lit!=trueLit() && lit!=falseLit());
		_grounding->addUnitClause(-lit);
	}
}

TruthValue GroundTranslator::checkApplication(const DomainElement* domelem, SortTable* predtable, SortTable* termtable, Context funccontext, SIGN sign, const Formula* origFormula) {
	// Check partial functions
	if (domelem == NULL) {
		TruthValue result;
		switch (funccontext) {
		case Context::POSITIVE:
			result = TruthValue::False;
			break;
		case Context::NEGATIVE:
			result = TruthValue::False;
			break;
		case Context::BOTH:
			if (origFormula != NULL) {
				Error::error("Could not find out the semantics of an ambiguous partial term. Please specify the meaning.", origFormula->pi());
			} else {
				Error::error("Could not find out the semantics of an ambiguous partial term. Please specify the meaning.");
			}
			throw IdpException("Critical error encountered. Aborting.");
		}
		if (verbosity() > 2) {
			std::clog << tabs() << "Partial function went out of bounds\n";
		}
		return isPos(sign) ? result : ~result;
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
Lit GroundTranslator::translate(const SymbolOffset& offset, const ElementTuple& args, bool reduced) { // reduction should not be allowed in recursive context or when reducedgrounding is off
	auto& known = knownlits[reduced][offset.functionlist][offset.offset];
	auto findknown = known.find(args);
	if(findknown!=known.cend()){
		return findknown->second;
	}
	CheckerInfo* checkers = NULL;
	if (offset.functionlist) {
		checkers = functions[offset.offset]->checkers;
	} else {
		checkers = symbols[offset.offset]->checkers;
	}
	Assert(checkers->ctchecker!=NULL);

	bool littrue = false, litfalse = false;
	for (uint i = 0; i < args.size(); ++i) {
		*checkers->containers[i] = args[i];
	}
	if (checkers->ctchecker->check()) { // Literal is irrelevant in its occurrences
		if (verbosity() > 2) {
			std::clog << tabs() << "Certainly true checker succeeded" << "\n";
		}
		littrue = true;
	}
	if (not checkers->ptchecker->check()) { // Literal decides formula if checker succeeds
		if (verbosity() > 2) {
			std::clog << tabs() << "Possibly true checker failed" << "\n";
		}
		litfalse = true;
	}
	if (not littrue && checkers->inter->isTrue(args)) {
		littrue = true;
	}
	if (not litfalse && checkers->inter->isFalse(args)) {
		litfalse = true;
	}
	if (littrue && litfalse) {
		_grounding->addEmptyClause();
		throw UnsatException();
	}

	if (reduced) {
		if (littrue) {
			known[args] = trueLit();
			return trueLit();
		} else if (litfalse) {
			known[args] = falseLit();
			return falseLit();
		}
	}

	auto lit = getLiteral(offset, args);
	Assert(lit!=trueLit() && lit!=falseLit());
	if (littrue) {
		_grounding->addUnitClause(lit);
	} else if (litfalse) {
		_grounding->addUnitClause(-lit);
	}
	known[args] = lit;
	return lit;
}

// ERROR if a domain element is not an int, as no var can be constructed for it!!!
void GroundTranslator::createImplications(Lit newhead, const std::map<std::vector<GroundTerm>, Lit >& term2lits, const std::vector<GroundTerm>& terms, bool ){
	for(auto terms2lit: term2lits){
		litlist equalities;
		bool equalitycertainlyfalse = false;
		for(uint i=0; i< terms.size(); ++i){
			auto termleft = terms[i];
			auto termright = terms2lit.first[i];

			// This also handles the case where the domain element are not integers, in which case no var can be created at the moment
			if(not termleft.isVariable and not termright.isVariable and termleft._domelement!=termright._domelement){
				equalitycertainlyfalse = true;
				break;
			}

			auto varleft = termleft._varid;
			if(not termleft.isVariable){
				varleft = translateTerm(termleft._domelement);
			}

			auto varright = termright._varid;
			if(not termright.isVariable){
				varright = translateTerm(termright._domelement);
			}
			equalities.push_back(reify(new CPVarTerm(varleft), CompType::EQ, CPBound(varright), TsType::EQ));
		}
		if(equalitycertainlyfalse){
			continue;
		}
		_grounding->add(newhead, TsType::RIMPL, equalities, true, -1);
		equalities[equalities.size()-1]=newhead;
		_grounding->add(terms2lit.second, TsType::RIMPL, equalities, true, -1);
	}
}

std::map<std::vector<GroundTerm>, Lit >& GroundTranslator::getTerm2Lits(PFSymbol* symbol){
	auto symboloffset = getSymbol(symbol);
	if(symboloffset.functionlist){
		return functions[symboloffset.offset]->lazyatoms2lit;
	}else{
		return symbols[symboloffset.offset]->lazyatoms2lit;
	}
}

/**
 * Takes care of writing out the same tseitin for the same set of terms.
 * TODO: eventually, this should go to the solver, which will allow it to also reason on equality of P(c) and P(d)
 * 		without having to add implications between their tseitins here (T_Pc & c=d => T_Pd)
 */
Lit GroundTranslator::addLazyElement(PFSymbol* symbol, const std::vector<GroundTerm>& terms, bool recursive){
	if(recursive){
		throw notyetimplemented("Recursive lazy element");
	}

	Lit result;
	auto& lazyatoms2lit = getTerm2Lits(symbol);
	if(contains(lazyatoms2lit,terms)){
		result = lazyatoms2lit.at(terms);
	} else {
		result = createNewUninterpretedNumber();
		if(getOption(ADDEQUALITYTHEORY)){
			createImplications(result, lazyatoms2lit, terms, recursive);
		}
		lazyatoms2lit[terms] = result;
		_grounding->addLazyElement(result, symbol, terms, recursive);
	}
	return result;
}

// TODO this can probably be optimized by storing trueLit()/falseLit() wherever applicable instead of lit
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
		Assert(lit>=0);
		atom2Tuple[lit]->first = functions[symboloffset.offset]->symbol;
		atom2Tuple[lit]->second = args;
		atomtype[lit] = AtomType::CPGRAPHEQ;

		// TODO handle lazy grounding here

		return lit;
	} else { // Atom had not been introduced yet, so do this here
		Lit lit = 0;
		auto& symbolinfo = *symbols[symboloffset.offset];
		auto jt = symbolinfo.tuple2atom.find(args);
		if (jt != symbolinfo.tuple2atom.end()) {
			lit = jt->second;
			jt->second = lit;
		} else {
			lit = nextNumber(AtomType::INPUT);
			Assert(lit>=0);
			atom2Tuple[lit]->first = symbolinfo.symbol;
			atom2Tuple[lit]->second = args;
			symbolinfo.tuple2atom.insert(jt, Tuple2Atom { args, lit });
		}

		if(_groundingmanager!=NULL){
			_groundingmanager->notifyNewLiteral(symbolinfo.symbol, args, lit);
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
				if (functions[n]->symbol == pfs) {
					return SymbolOffset(n, true);
				}
			}
		}
	}
	for (size_t n = 0; n < symbols.size(); ++n) {
		if (symbols[n]->symbol == pfs) {
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
				functions.push_back(new FunctionInfo(function, _structure));
				return SymbolOffset(functions.size() - 1, true);
			}
		}
		symbols.push_back(new SymbolInfo(pfs, _structure));
		return SymbolOffset(symbols.size() - 1, false);
	} else {
		return n;
	}
}
Lit GroundTranslator::conjunction(const Lit l, const Lit r, TsType tstype) {
	if (l == trueLit()) {
		return r;
	}
	if (l == falseLit()) {
		return l;
	}
	if (r == trueLit()) {
		return l;
	}
	if (r == falseLit()) {
		return r;
	}
	return reify( { l, r }, true, tstype);
}

Lit GroundTranslator::reify(const litlist& clause, bool conj, TsType tstype) {
	int head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	auto tsbody = new PCTsBody(tstype, clause, conj);
	atom2TsBody[head] = tsbody;
	return head;
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

//#warning need much more sharing in groundtranslator cp reify (negations!)
//#warning optimize in solver by passing translator and adding a cheaper method to do the below for var comp int?
Lit GroundTranslator::reify(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	auto leftvar = dynamic_cast<CPVarTerm*>(left);
	auto newright = right;
	if(leftvar!=NULL && newright._isvarid && leftvar->varid().id>newright._varid.id && (comp==CompType::EQ || comp==CompType::NEQ)){
		auto rightvar = newright._varid;
		newright = CPBound(leftvar->varid());
		left = new CPVarTerm(rightvar);
	}
	auto tsbody = new CPTsBody(tstype, left, comp == CompType::NEQ ? CompType::EQ : comp, newright);

	auto it = cpset.find(tsbody);
	if (it != cpset.cend()) {
		delete tsbody;
		auto storedcomparison = it->first->comp(); // NOTE: OPTIMIZATION! = and ~= map to the same tsbody etc. => look at ecnf.cpp:compEqThroughNeg
		return storedcomparison==comp?it->second:-it->second;
	}

	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	atom2TsBody[nr] = tsbody;
	cpset[tsbody] = nr;
	if (comp == CompType::NEQ) {
		nr = -nr;
	}
	return nr;
}

// Note: set IDs start from 1
SetId GroundTranslator::translateSet(int id, const ElementTuple& freevar_inst, const litlist& lits, const weightlist& weights, const weightlist& trueweights, const termlist& cpvars) {
	TsSet tsset;
	tsset._setlits = lits;
	tsset._litweights = weights;
	tsset._trueweights = trueweights;
	tsset._cpvars = cpvars;
	auto setnr = _sets.size() + 1;
	_sets.push_back(tsset);
	_freevar2set[id][freevar_inst] = setnr;
/*	cerr <<"Set " <<setnr <<":{";
	for(auto i=0; i<lits.size(); ++i){
		cerr <<"(" <<printLit(lits[i]) <<", " <<printTerm(cpvars[i]) <<")\n"; // TODO weights
	}
	for(auto i=0; i<trueweights.size(); ++i){
		cerr <<"(" <<printLit(trueLit()) <<", " <<trueweights[i] <<")\n";
	}
	cerr <<"}\n";*/
	return setnr;
}

bool GroundTranslator::isSet(SetId setID) const {
	return setID.id!=0 && _sets.size() > (size_t) setID.id - 1;
}
const TsSet GroundTranslator::groundset(SetId setID) const {
	Assert(isSet(setID));
	return _sets[setID.id - 1];
}
SetId GroundTranslator::getPossibleSet(int id, const ElementTuple& freevar_inst) const{
	if(not contains(_freevar2set, id)){
		return SetId(0);
	}
	const auto& atid = _freevar2set.at(id);
	auto setid = atid.find(freevar_inst);
	if(setid!=atid.cend()){
		return setid->second;
	}else{
		return SetId(0);
	}
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
	auto& info = *functions[offset.offset];
	auto it = info.term2var.lower_bound(args);
	if (it != info.term2var.cend() && it->first == args) {
		return it->second;
	} else {
		auto varid = nextVarNumber();
		info.term2var.insert(it, pair<vector<GroundTerm>, VarId> { args, varid });
		auto ft = new ftpair(info.symbol, args);
		var2Tuple[varid.id] = ft;
		var2domain[varid.id] = _structure.concrstructure->storableInter(info.symbol->outsort());
		if(info.symbol->partial()){
			auto atom = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
			var2partial[varid.id] = atom;
			atom2TsBody[atom] = new DenotingTsBody(TsType::EQ, varid); // TODO Rule?
		}
		return varid;
	}
}

VarId GroundTranslator::translateTerm(Function* function, const vector<GroundTerm>& args) {
	Assert(CPSupport::eligibleForCP(function, vocabulary()));
	auto offset = addSymbol(function);
	VarId trans = translateTerm(offset, args);
	if(_groundingmanager!=NULL){
		_groundingmanager->notifyNewVarId(function,args,trans);
	}
	return trans;
}

VarId GroundTranslator::translateTerm(CPTerm* cpterm, SortTable* domain) {
	auto termit = cp2id.find(cpterm);
	if (termit != cp2id.cend()) {
		auto& sort2id = termit->second;
		auto domainit = sort2id.find(domain);
		if(domainit != sort2id.cend()){
			var2domain[domainit->second.id] = domain;
			return domainit->second;
		}
	}

	auto varid = nextVarNumber(domain);

	CPBound bound(varid);
	auto cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	var2CTsBody[varid.id] = cprelation;
	cp2id[cpterm][domain] = varid;
	return varid;
}

VarId GroundTranslator::translateTerm(const DomainElement* element) {
	Assert(element!=NULL && element->type() == DET_INT);
	auto value = element->value()._int;

	auto it = storedTerms.find(value);
	if (it != storedTerms.cend()) {
		return it->second;
	}

	auto varid = nextVarNumber();
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

VarId GroundTranslator::nextVarNumber(SortTable* domain) {
	VarId id;
	id.id = var2Tuple.size();
	var2Tuple.push_back(NULL);
	var2CTsBody.push_back(NULL);
	var2domain.push_back(domain);
	var2partial.push_back(falseLit());
	return id;
}

string GroundTranslator::printTerm(const GroundTerm& gt) const {
	if(not gt.isVariable){
		return toString(gt._domelement);
	}

	auto varid = gt._varid;
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
	s << print(func);
	if (not getArgs(varid).empty()) {
		s << "(";
		bool begin = true;
		for(auto arg : getArgs(varid)){
			if(not begin){
				s <<",";
			}
			begin = false;
			if (arg.isVariable) {
				s << printTerm(arg._varid);
			} else {
				s << print(arg._domelement);
			}
		}
		s << ")";
	}
	return s.str();
}

string GroundTranslator::printLit(const Lit& lit) const {
	stringstream s;
	Lit nr = lit;
	if (nr == trueLit()) {
		return "true";
	}
	if (nr == falseLit()) {
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
	case AtomType::CPGRAPHEQ:
	case AtomType::INPUT: {
		auto pfs = getSymbol(nr);
		s << print(pfs);
		auto tuples = getArgs(nr);
		if (not tuples.empty()) {
			s << "(";
			printList(s, tuples, ", ");
			s << ")";
		}
		break;
	}
	case AtomType::TSEITINWITHSUBFORMULA:
	case AtomType::LONETSEITIN:
		s << "tseitin_" << nr;
		break;
	}
	return s.str();
}
