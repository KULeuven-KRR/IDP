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

#include "Symmetry.hpp"
#include "IncludeComponents.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/modelexpansion/DefinitionPostProcessing.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

/**********
 * Miscellaneous methods
 **********/

/**
 *	returns a tuple of domain elements symmetrical under a binary permutation of domain elements to a given tuple, for the given arguments
 */
ElementTuple symmetricalTuple(const ElementTuple& original, const DomainElement* first, const DomainElement* second, const set<unsigned int>& argumentPlaces) {
	ElementTuple symmetrical = original;
	for (auto argumentPlaces_it = argumentPlaces.cbegin(); argumentPlaces_it != argumentPlaces.cend(); ++argumentPlaces_it) {
		if (symmetrical[*argumentPlaces_it] == first) {
			symmetrical[*argumentPlaces_it] = second;
		} else if (symmetrical[*argumentPlaces_it] == second) {
			symmetrical[*argumentPlaces_it] = first;
		}
	}
	return symmetrical;
}

// TODO: method with variable argument number would be nice: #include <stdarg.h>
// TODO: also, method belongs in AbstractGroundTheory

void addClause(AbstractGroundTheory* gt, const int first, const int second) {
	int arr[] = {first, second};
	vector<int> clause(arr, arr + sizeof (arr) / sizeof (arr[0]));
	gt->add(clause);
}

void addClause(AbstractGroundTheory* gt, const int first, const int second, const int third) {
	int arr[] = {first, second, third};
	vector<int> clause(arr, arr + sizeof (arr) / sizeof (arr[0]));
	gt->add(clause);
}

void addClause(AbstractGroundTheory* gt, const int first, const int second, const int third, const int fourth) {
	int arr[] = {first, second, third, fourth};
	vector<int> clause(arr, arr + sizeof (arr) / sizeof (arr[0]));
	gt->add(clause);
}

/**
 * 	given a symmetry in the form of two lists of literals which represent a bijection, this method adds CNF-clauses to the theory which break the symmetry.
 */

void addSymBreakingClausesToGroundTheory(AbstractGroundTheory* gt, const std::vector<int>& literals, const std::vector<int>& symLiterals) {
	std::vector<int>::const_iterator literals_it = literals.cbegin();
	std::vector<int>::const_iterator symLiterals_it = symLiterals.cbegin();
	// this is the current literal and its symmetric:
	int lit = *literals_it;
	int symLit = *symLiterals_it;
	// these are the tseitin vars needed to shorten the formula (initialization happens only when needed):
	int tseitin = 0;
	int prevTseitin = 0;

	if (literals.size() > 0) {
		// (~l1 | s(l1))
		addClause(gt, -lit, symLit);
	}
	if (literals.size() > 1) {
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// (~t1 | l1 | ~s(l1))
		addClause(gt, -tseitin, lit, -symLit);
		// (t1 | ~l1)
		addClause(gt, tseitin, -lit);
		// (t1 | s(l1))
		addClause(gt, tseitin, symLit);
		// (~t1 | ~l2 | s(l2))
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		addClause(gt, -tseitin, -lit, symLit);
	}
	std::vector<int>::const_iterator oneButLast_it = literals.cend();
	--oneButLast_it;
	while (literals_it != oneButLast_it) {
		prevTseitin = tseitin;
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// ( ~tn | tn )
		addClause(gt, -tseitin, prevTseitin);
		// ( ~tn | ln | ~s(ln) )
		addClause(gt, -tseitin, lit, -symLit);
		// ( tn | ~tn-1 | ~ln)
		addClause(gt, tseitin, -prevTseitin, -lit);
		// ( tn | ~tn-1 | s(ln))
		addClause(gt, tseitin, -prevTseitin, symLit);
		// ( ~tn | ~ln+1 | s(ln+1) )
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		addClause(gt, -tseitin, -lit, symLit);
	}
}

/**
 * 	given a symmetry in the form of two lists of domain elements which represent a bijection, this method adds CNF-clauses to the theory which break the symmetry.
 *
 * 	This variation induces extra solutions by relaxing the constraints on the tseitin variables. The advantage is less and smaller clauses.
 */
void addSymBreakingClausesToGroundTheoryShortest(AbstractGroundTheory* gt, const std::vector<int>& literals, const std::vector<int>& symLiterals) {
	std::vector<int>::const_iterator literals_it = literals.cbegin();
	std::vector<int>::const_iterator symLiterals_it = symLiterals.cbegin();
	// this is the current literal and its symmetric:
	int lit = *literals_it;
	int symLit = *symLiterals_it;
	// these are the tseitin vars needed to shorten the formula (initialization happens only when needed):
	int tseitin = 0;
	int prevTseitin = 0;

	if (literals.size() > 0) {
		// (~l1 | s(l1))
		addClause(gt, -lit, symLit);
	}
	if (literals.size() > 1) {
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// (t1 | ~l1)
		addClause(gt, tseitin, -lit);
		// (t1 | s(l1))
		addClause(gt, tseitin, symLit);
		// (~t1 | ~l2 | s(l2))
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		addClause(gt, -tseitin, -lit, symLit);
	}
	std::vector<int>::const_iterator oneButLast_it = literals.cend();
	--oneButLast_it;
	while (literals_it != oneButLast_it) {
		prevTseitin = tseitin;
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// ( tn | ~tn-1 | ~ln)
		addClause(gt, tseitin, -prevTseitin, -lit);
		// ( tn | ~tn-1 | s(ln))
		addClause(gt, tseitin, -prevTseitin, symLit);
		// ( ~tn | ~ln+1 | s(ln+1) )
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		addClause(gt, -tseitin, -lit, symLit);
	}
}

/**
 *	Method used to generate ordered tuples of domain elements which represent (partial) ground elements, useful for generating SAT variables.
 *
 *	This method extends a collection of ground elements by replacing each ground element g with for each domain element d in domainTable but not in excludedElements, the ground elements resulting from extending g with d at argument rank.
 *	So if (a . .) is a ground element, and domainTable is {a, b, c}, and excludedElements is {b}, and the rank is 1, (a . .) will be replaced by (a a .) and (a c .).
 */
vector<vector<const DomainElement*> > fillGroundElementsOneRank(vector<vector<const DomainElement*> >& groundElements, const SortTable* domainTable,
		const int rank, const set<const DomainElement*>& excludedElements) {
	set<const DomainElement*> domain; //set to order the elements
	for (SortIterator domain_it = domainTable->sortBegin(); not domain_it.isAtEnd(); ++domain_it) {
		if (!excludedElements.count(*domain_it)) {
			domain.insert(*domain_it);
		}
	}
	vector<vector<const DomainElement*> > newGroundElements(groundElements.size() * domain.size());
	for (unsigned int ge = 0; ge < groundElements.size(); ++ge) {
		int index = 0;
		for (auto domain_it = domain.cbegin(); domain_it != domain.cend(); ++domain_it) {
			newGroundElements[ge * domain.size() + index] = groundElements[ge];
			newGroundElements[ge * domain.size() + index][rank] = (*domain_it);
			++index;
		}
	}
	return newGroundElements;
}

UFSymbolArg::UFSymbolArg() : forbiddenNode(new ForbiddenNode()) {
}

UFSymbolArg::~UFSymbolArg() {
	for (auto pair : SAnodes) {
		for (auto node : *pair.second) {
			delete node;
		}
		delete pair.second;
	}
	for (auto pair : VARnodes) {
		delete pair.second;
	}
	for (auto node : twoValuedNodes) {
		delete node;
	}
	delete forbiddenNode;
}

UFNode* UFSymbolArg::get(PFSymbol* sym, unsigned int arg, bool twoValued) {
	if (twoValued) { // create a new, independent node, so that partitions are not merged
		SymbolArgumentNode* result = new SymbolArgumentNode(sym, arg);
		twoValuedNodes.push_back(result);
		return result;
	}

	if (SAnodes.count(sym) == 0) {
		std::vector<SymbolArgumentNode*>* argnodelist = new std::vector<SymbolArgumentNode*>();
		for (unsigned int i = 0; i < sym->nrSorts(); ++i) {
			argnodelist->push_back(new SymbolArgumentNode(sym, i));
		}
		SAnodes.insert({sym, argnodelist});
	}
	return SAnodes.at(sym)->at(arg);
}

UFNode* UFSymbolArg::get(const Variable* var) {
	if (VARnodes.count(var) == 0) {
		VARnodes.insert({var, new VariableNode(var)});
	}
	return VARnodes[var];
}

UFNode* UFSymbolArg::get(const Sort* s, const DomainElement* de) {
	DomainElementNode* result = new DomainElementNode(s, de);
	twoValuedNodes.push_back(result);
	return result;
}

UFNode* UFSymbolArg::getForbiddenNode() {
	return forbiddenNode;
}

UFNode* UFSymbolArg::find(UFNode* in) {
	UFNode* rt = in->parent;
	while (rt != rt->parent) { // find root
		rt = rt->parent;
	}
	while (in != rt) { // adjust root all the way up
		UFNode* oldparent = in->parent;
		in->parent = rt;
		in = oldparent;
	}
	return rt;
}

void UFSymbolArg::merge(UFNode* first, UFNode* second) {
	UFNode* x = find(first);
	UFNode* y = find(second);
	if (x == y) return;
	// make smaller root point to larger one
	if (x->depth < y->depth) {
		x->parent = y;
	} else if (x->depth > y->depth) {
		y->parent = x;
	} else {
		y->parent = x;
		x->depth = x->depth + 1;
	}
}

void UFSymbolArg::getPartition(std::unordered_multimap<UFNode*, UFNode*>& out) {
	for (auto pair : SAnodes) {
		for (auto node : *pair.second) {
			out.insert({find(node), node});
		}
	}
	for (auto pair : VARnodes) {
		out.insert({find(pair.second), pair.second});
	}
	for (auto node : twoValuedNodes) {
		out.insert({find(node), node});
	}
	out.insert({find(forbiddenNode), forbiddenNode});
}

void UFSymbolArg::printPartition(std::ostream& ostr) {
	std::unordered_multimap<UFNode*, UFNode*> partition;
	getPartition(partition);
	UFNode* currentrep = nullptr;
	for (auto pair : partition) {
		if (pair.first != currentrep) {
			if (currentrep != nullptr) {
				ostr << std::endl;
			}
			currentrep = pair.first;
			currentrep->put(ostr);
			ostr << " <- ";
		}
		pair.second->put(ostr);
		ostr << ", ";
	}
	ostr << std::endl;
}

InterchangeabilityAnalyzer::InterchangeabilityAnalyzer(const Structure* s)
: _structure(s), subNode(nullptr), disjointSet(UFSymbolArg()) {
}

void InterchangeabilityAnalyzer::analyze(const AbstractTheory* t) {
	t->accept(this);
}

void InterchangeabilityAnalyzer::analyzeForOptimization(const Term* t) {
	t->accept(this);
	disjointSet.merge(disjointSet.getForbiddenNode(), subNode);
}

void InterchangeabilityAnalyzer::visit(const PredForm* f) {
	if (f->symbol()->builtin()) {
		if (is(f->symbol(), STDPRED::EQ)) {
			// due to lack of multiple dispatch we're simply storing the fact that the upper symbol is an equality
			UFNode* equalityRep = nullptr;
			for (size_t n = 0; n < f->subterms().size(); ++n) {
				f->subterms()[n]->accept(this);
				if (equalityRep == nullptr) {
					equalityRep = subNode;
				} else {
					disjointSet.merge(equalityRep, subNode);
				}
			}
		} else { // (is(f->symbol(), STDPRED::GT) || is(f->symbol(), STDPRED::LT))
			// all occurrences in GT or LT are asymmetric:
			for (size_t n = 0; n < f->subterms().size(); ++n) {
				f->subterms()[n]->accept(this);
				disjointSet.merge(disjointSet.getForbiddenNode(), subNode);
			}
		}
	} else {
		for (size_t n = 0; n < f->subterms().size(); ++n) {
			f->subterms()[n]->accept(this);
			disjointSet.merge(disjointSet.get(f->symbol(), n, _structure->inter(f->symbol())->approxTwoValued()), subNode);
		}
	}
}

void InterchangeabilityAnalyzer::visit(const EqChainForm*) {
	Assert(false); // Assuming all eqchainforms are rewritten
}

void InterchangeabilityAnalyzer::visit(const EquivForm*) {
	Assert(false); // Assuming all equivforms are rewritten
}

void InterchangeabilityAnalyzer::visit(const VarTerm* vt) {
	subNode = disjointSet.get(vt->var());
}

void InterchangeabilityAnalyzer::visit(const FuncTerm* t) {
	if (t->function()->builtin()) {
		if (is(t->function(), STDFUNC::MINELEM)) { // treat this like a DomainTerm
			Sort* srt = t->function()->outsort();
			auto st = _structure->inter(srt);
			Assert(not st->empty());
			subNode = disjointSet.get(srt, st->first());
		} else if (is(t->function(), STDFUNC::MAXELEM)) { // treat this like a DomainTerm
			Sort* srt = t->function()->outsort();
			auto st = _structure->inter(srt);
			Assert(not st->empty());
			subNode = disjointSet.get(srt, st->last());
		} else { // all other builtin functions are asymmetrical
			size_t n;
			for (n = 0; n < t->subterms().size(); ++n) {
				t->subterms()[n]->accept(this);
				disjointSet.merge(disjointSet.getForbiddenNode(), subNode);
			}
			subNode = disjointSet.getForbiddenNode();
		}
	} else {
		size_t n;
		for (n = 0; n < t->subterms().size(); ++n) {
			t->subterms()[n]->accept(this);
			disjointSet.merge(disjointSet.get(t->function(), n, _structure->inter(t->function())->approxTwoValued()), subNode);
		}
		Assert(n == t->function()->nrSorts() - 1);
		subNode = disjointSet.get(t->function(), n, _structure->inter(t->function())->approxTwoValued());
	}
}

void InterchangeabilityAnalyzer::visit(const DomainTerm* dt) {
	subNode = disjointSet.get(dt->_sort, dt->_value);
}

void InterchangeabilityAnalyzer::visit(const AggTerm* at) {
	traverse(at);
	subNode = disjointSet.getForbiddenNode(); // aggregate terms themselves break symmetry
}

void InterchangeabilityAnalyzer::visit(const QuantSetExpr* s) {
	s->getTerm()->accept(this);
	disjointSet.merge(disjointSet.getForbiddenNode(), subNode); // functions used in aggregate terms are asymmetric
	s->getCondition()->accept(this);
}

void detectInterchangeability(std::vector<InterchangeabilityGroup*>& out_groups, const AbstractTheory* t, const Structure* s, const Term* obj) {
	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "*** DETECTING SYMMETRY ***" << std::endl;
	}

	AbstractTheory* theo = t->clone();
	
	if(obj!=nullptr){
		Term* obj_clone = obj->clone();
		DomainTerm* dummyTerm = new DomainTerm(obj->sort(),s->inter(obj->sort())->last(),TermParseInfo());
		theo->add(new PredForm(SIGN::POS,get(STDPRED::LT,obj_clone->sort()),{obj_clone,dummyTerm},FormulaParseInfo()));
	}
	
	getIntchGroups(theo,s,out_groups);
	
	for (auto icg : out_groups) {
		if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
			icg->print(clog);
		}
	}
	
	//delete theo; // TODO: is this sufficient, given all the theory manipulation that has been done?
	theo->recursiveDelete();
}

void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<std::pair<PFSymbol*, unsigned int> >& forcedSymbArgs){
	// TODO: fix forcedSymbArgs usage. If provided, should _only_ look for those arguments!
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		clog << "pushing quantifiers completely..." << std::endl;
	}
	theo = FormulaUtils::pushQuantifiersCompletely(theo);
	if (getOption(IntType::VERBOSE_SYMMETRY) > 2) {
		theo->put(clog);
	}
	
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		clog << "partitioning connected arguments in theory..." << std::endl;
	}
	InterchangeabilityAnalyzer ia = InterchangeabilityAnalyzer(s);
	ia.analyze(theo);
	// add to partition all forced symbol arguments
	// these may not appear in the theory, but the caller of the function is interested in their status nonetheless
	for(auto sa: forcedSymbArgs){
		ia.disjointSet.get(sa.first,sa.second);
	}
	
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		ia.disjointSet.printPartition(clog);
	}
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		clog << "detecting interchangeable domains..." << std::endl;
	}
	
	// First, extract sets of related arguments not occurring in asymmetric symbols.
	// A symbol is asymmetric if it has more than 1 argument, and the symbol has at least two of its three interpretation tables non-empty.
	// In other words, the symbol is not completely true, not completely false, or not completely unknown.
	std::unordered_multimap<UFNode*, UFNode*> partition;
	ia.disjointSet.getPartition(partition);

	std::vector<InterchangeabilitySet*> intersets;
	InterchangeabilitySet* currentset = nullptr;
	UFNode* currentrep = nullptr;

	for (auto pair : partition) {
		if (pair.first != currentrep) { // we're entering a new partition, so reinitialize
			currentrep = pair.first;
			if (currentset != nullptr) {
				intersets.push_back(currentset);
			}
			currentset = new InterchangeabilitySet(s);
		}
		if (currentset != nullptr) { // add the UFNode information in the current partition
			bool stilSymmetric = pair.second->addTo(currentset);
			if (!stilSymmetric) { // no longer symmetric, erase current interchangeability set
				delete currentset;
				currentset = nullptr;
			}
		}
	}
	if (currentset != nullptr) { // last set is not yet added
		intersets.push_back(currentset);
	}

	for (auto ichset : intersets) {
		// TODO: this is a rather ugly hack. ichset should not have been derived before...
		bool hasCorrectSymbArgs = (forcedSymbArgs.size()==0); // so if no forcedsymbargs are provided, look for symmetry in all symbargs in the theory. Otherwise, look for those connected to forcedsymbargs
		for(auto sa:forcedSymbArgs){
			if(ichset->symbolargs.count(sa.first)>0 && ichset->symbolargs[sa.first]->count(sa.second)>0){
				hasCorrectSymbArgs=true;
			}
		}
		if(!hasCorrectSymbArgs){
			continue;
		}
		
		ichset->calculateInterchangeableSets();
		if (getOption(IntType::VERBOSE_SYMMETRY) > 2) {
			ichset->print(clog);
		}
		ichset->getIntchGroups(out_groups);
	}

	for (size_t i = 0; i < intersets.size(); ++i) {
		delete intersets[i];
	}
}

void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups){
	std::vector<std::pair<PFSymbol*, unsigned int> > tmp;
	getIntchGroups(theo, s, out_groups, tmp);
}

template<class T>
void deleteAndClear(std::vector<T*>& vec){
	for(auto x: vec){
		delete x;
	}
	vec.clear();
}

enum PredVal {T, F, U};

PredVal getImage(PredInter* pi, ElementTuple& tup){
	if(pi->isTrue(tup,true)){
		return PredVal::T;
	}else if(pi->isFalse(tup,true)){
		return PredVal::F;
	}else{
		return PredVal::U;
	}
}

void setImage(PredInter* pi, ElementTuple& tup, PredVal& img){
	if(img==PredVal::T){
		pi->makeTrueExactly(tup,true);
	}else if(img==PredVal::F){
		pi->makeFalseExactly(tup,true);
	}else{
		pi->makeUnknownExactly(tup,true);
	}
}

InterchangeabilityGroup::InterchangeabilityGroup(std::vector<const DomainElement*>& domels, std::vector<PFSymbol*> symbs3val, 
		std::unordered_map<PFSymbol*, std::unordered_set<unsigned int>* >& symbargs){
	for (auto de : domels) {
		elements.insert(de);
	}
	for(auto symb: symbs3val){
		std::unordered_set<unsigned int>* args = new std::unordered_set<unsigned int>(*(symbargs[symb]));
		symbolargs.insert({symb,args});
	}
}

InterchangeabilityGroup::~InterchangeabilityGroup() {
	for(auto paar: symbolargs){
		delete paar.second;
	}
}

void InterchangeabilityGroup::print(std::ostream& ostr) {
	for (auto paar : symbolargs) {
		paar.first->put(ostr);
		ostr << "/";
		for(auto arg: *(paar.second)){
			ostr << arg << " ";
		}
	}
	ostr << "<- ";
	for (auto de : elements) {
		de->put(ostr);
		ostr << " ";
	}
	ostr << std::endl;
}

unsigned int InterchangeabilityGroup::getNrSwaps() {
	return (elements.size()*(elements.size() - 1)) / 2;
}

bool InterchangeabilityGroup::hasSymbArg(PFSymbol* symb, unsigned int arg){
	return symbolargs.count(symb)>0 && symbolargs[symb]->count(arg)>0;
}

void InterchangeabilityGroup::breakSymmetry(AbstractGroundTheory* gt, Structure* struc, bool nbModelsEquivalent) const {
  set<const DomainElement*> ordered_els;
  ordered_els.insert(elements.cbegin(),elements.cend());
  set<const DomainElement*>::const_iterator smaller = ordered_els.cbegin();
  set<const DomainElement*>::const_iterator bigger = ordered_els.cbegin();
  ++bigger;
  for (; bigger != ordered_els.cend(); ++bigger, ++smaller) {
    std::vector<int> lits;
    std::vector<int> sym_lits;
    getSymmetricLiterals(gt,struc, *smaller,*bigger,lits,sym_lits);
    
    if (nbModelsEquivalent) {
      addSymBreakingClausesToGroundTheory(gt, lits, sym_lits);
    } else {
      addSymBreakingClausesToGroundTheoryShortest(gt, lits, sym_lits);
    }
  }
}

/**
 *	Given a binary symmetry S represented by two domain elements, this method generates two disjunct lists of SAT variables which represent S.
 *	The first list is ordered, and for the ith variable v in either of the lists, S(v) is the ith variable in the other list.
 *	This method is useful in creating short symmetry breaking formulae.
 *
 *	Order is based on the pointers of the domain elements, not on the order given by for instance a SortIterator!
 */
void InterchangeabilityGroup::getSymmetricLiterals(AbstractGroundTheory* gt, Structure* struc, const DomainElement* smaller, const DomainElement* bigger, std::vector<int>& originals, std::vector<int>& symmetricals) const{
	set<const DomainElement*> excludedSet;
	excludedSet.insert(smaller);
	excludedSet.insert(bigger);
	const set<const DomainElement*> emptySet;

    for(auto symbarg: symbolargs){
      PFSymbol* symb = symbarg.first;
      std::set<unsigned int> argumentPlaces;
      argumentPlaces.insert(symbarg.second->cbegin(),symbarg.second->cend());
      if(struc->inter(symb)->approxTwoValued()){
        continue; // no need to construct sym breaking constraints for this symbol
      }
      
      for (auto arg : argumentPlaces){
          vector<vector<const DomainElement*> > groundElements(1);
          groundElements[0] = vector<const DomainElement*>(symb->nrSorts());
          for (unsigned int argument = 0; argument < arg; ++argument) {
              if (symbarg.second->count(argument)) {
                  groundElements = fillGroundElementsOneRank(groundElements, struc->inter(symb->sort(argument)), argument, excludedSet);
              } else {
                  groundElements = fillGroundElementsOneRank(groundElements, struc->inter(symb->sort(argument)), argument, emptySet);
              }
          }
          for (unsigned int it = 0; it < groundElements.size(); it++) {
              groundElements[it][arg] = smaller;
          }
          for (unsigned int argument = arg + 1; argument < symb->nrSorts(); ++argument) {
              Sort* currSort = symb->sort(argument);
              groundElements = fillGroundElementsOneRank(groundElements, struc->inter(currSort), argument, emptySet);
          }
          for (auto ge_it = groundElements.cbegin(); ge_it != groundElements.cend(); ++ge_it) {
              ElementTuple original = *ge_it;
              ElementTuple symmetrical = symmetricalTuple(original, smaller, bigger, argumentPlaces);
              originals.push_back(gt->translator()->translateReduced(symb, original, false));
              symmetricals.push_back(gt->translator()->translateReduced(symb, symmetrical, false));
          }
      }
    }
}

bool InterchangeabilitySet::add(PFSymbol* p, unsigned int arg) {
	if (symbolargs.count(p) == 0) {
		symbolargs.insert({p, new std::unordered_set<unsigned int>()});
	}
	symbolargs[p]->insert(arg);
	sorts.insert(p->sort(arg));
	return true;
}

bool InterchangeabilitySet::add(const Sort* s) {
	sorts.insert(s);
	return true;
}

bool InterchangeabilitySet::add(const DomainElement* de) {
	occursAsConstant.insert(de);
	return true;
}

void InterchangeabilitySet::calculateInterchangeableSets() {
	std::unordered_set<const DomainElement*> allElements;
	// run over all sorts to find relevant domain elements
	for (auto s : sorts) {
        if(!_struct->inter(s)->finite()){
          allElements.clear();
          break; // not going to detect symmetry over an infinite domain ;)
        }
		auto sortiter = _struct->inter(s)->sortBegin();
		while (!sortiter.isAtEnd()) {
			if (occursAsConstant.count(*sortiter) == 0) {
				allElements.insert(*sortiter);
			}
			++sortiter;
		}
	}
	// partition domain elements based on isEqual method of ElementOccurrence
	for (auto de : allElements) {
		std::shared_ptr<ElementOccurrence> eloc(new ElementOccurrence(this, de));
		auto part_iter = partition.insert({eloc, new ElementTuple()});
		part_iter.first->second->push_back(de);
	}
}

void InterchangeabilitySet::getIntchGroups(std::vector<InterchangeabilityGroup*>& out) {
	for (auto it : partition) {
		// Add the partition if sufficient number of elements:
		if (it.second->size() < 2) {
			continue;
		}
		// Find the non-two-valued symbols:
		std::vector<PFSymbol*> symbs3val;
		for (auto sa : symbolargs) {
			PFSymbol* symb = sa.first;
			if (!_struct->inter(symb)->approxTwoValued()) {
				symbs3val.push_back(symb);
			}
		}
		if(symbs3val.size()==0){
			continue;
		}
		out.push_back(new InterchangeabilityGroup(*it.second,symbs3val,symbolargs));
	}
}

void InterchangeabilitySet::print(std::ostream& ostr) {
	for (auto sa : symbolargs) {
		sa.first->put(ostr);
		for (auto arg : *sa.second) {
			ostr << "/" << arg;
		}
		ostr << ", ";
	}
	ostr << ": ";

	for (auto paar : partition) {
		//ostr << paar.first->hash << ", ";
		for (auto de : *paar.second) {
			de->put(ostr);
			ostr << " ";
		}
		ostr << "| ";
	}
	ostr << std::endl;
}

ElementOccurrence::ElementOccurrence(InterchangeabilitySet* intset, const DomainElement* de) : ics(intset), domel(de) {
	hash = (size_t) ics;
	for (auto sa : ics->symbolargs) {
		PFSymbol* symb = sa.first;
		if (symb->nrSorts() == 1) { // use unary symbols to create hash function
			PredInter* pi = ics->_struct->inter(symb);
			ElementTuple tmp;
			tmp.push_back(de);
			if (pi->isFalse(tmp)) {
				hash = (hash << 5)^(hash * 3);
			} else if (pi->isTrue(tmp)) {
				hash = (hash << 7)^(hash * 5);
			} else {
				hash = (hash << 3)^(hash * 7);
			}
		}
	}
}

bool checkTableForSwapConsistency(PredTable* table, const DomainElement* first, const DomainElement* second, std::unordered_set<unsigned int>* args) {
	if (table->arity() == 1) {
		return table->contains({first}) == table->contains({second});
	}
	
	// TODO: optimize for functions with arity 1, and symmetric domain

	// else: table has greater arity, iterate over table to check whether it is symmetrical under swap
	TableIterator table_it = table->begin();
	while (!table_it.isAtEnd()) {
		// create symmetrical tuple
		const ElementTuple& origTuple = *table_it;
		ElementTuple swapTuple(origTuple.size());
		bool swapIsDifferent = false;
		for (unsigned int i = 0; i < origTuple.size(); ++i) {
			if (args->count(i) > 0) {
				if (origTuple[i] == first) {
					swapTuple[i] = second;
					swapIsDifferent = true;
				} else if (origTuple[i] == second) {
					swapTuple[i] = first;
					swapIsDifferent = true;
				} else {
					swapTuple[i] = origTuple[i];
				}
			} else {
				swapTuple[i] = origTuple[i];
			}
		}
		if (swapIsDifferent && !table->contains(swapTuple)) {
			return false;
		}
		++table_it;
	}
	return true;
}

bool ElementOccurrence::isEqualTo(const ElementOccurrence& other) const {
	if (other.ics != ics) {
		return false;
	}
	for (auto sa : ics->symbolargs) {
		PFSymbol* symb = sa.first;
		PredInter* pi = ics->_struct->inter(symb);
		// run over true and false table to check whether symmetrical tuples are also in there:
		if (!checkTableForSwapConsistency(pi->ct(), domel, other.domel, sa.second)) {
			return false;
		}
		if (!pi->approxTwoValued() && !checkTableForSwapConsistency(pi->cf(), domel, other.domel, sa.second)) {
			return false;
		}
	}
	return true;
}

void SymbolArgumentNode::put(std::ostream & outstr) {
	symbol->put(outstr);
	outstr << "/" << arg;
}

bool SymbolArgumentNode::addTo(InterchangeabilitySet * ichset) {
	return ichset->add(symbol, arg);
}

void VariableNode::put(std::ostream & outstr) {
	var->put(outstr);
}

bool VariableNode::addTo(InterchangeabilitySet * ichset) {
	return ichset->add(var->sort());
}

void DomainElementNode::put(std::ostream & outstr) {
	s->put(outstr);
	outstr << "::";
	de->put(outstr);
}

bool DomainElementNode::addTo(InterchangeabilitySet * ichset) {
	return ichset->add(de);
}

void ForbiddenNode::put(std::ostream & outstr) {
	outstr << "ASYMMETRIC";
}

bool ForbiddenNode::addTo(InterchangeabilitySet * ichset) {
	return false;
}