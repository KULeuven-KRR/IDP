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

#include "theory/TheoryUtils.hpp"

#include "SaucyGraph.hpp"

using namespace std;

/**
 * 	Given a symmetry in the form of two lists of literals which represent a bijection, this method adds CNF-clauses to the theory which break the symmetry.
 *  These clausal formulas are the same as implemented by BreakID (bitbucket.org/krr/breakid).
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
        gt->add({-lit, symLit});
	}
	if (literals.size() > 1) {
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// (~t1 | l1 | ~s(l1))
		gt->add({-tseitin, lit, -symLit});
		// (t1 | ~l1)
		gt->add({tseitin, -lit});
		// (t1 | s(l1))
		gt->add({tseitin, symLit});
		// (~t1 | ~l2 | s(l2))
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		gt->add({-tseitin, -lit, symLit});
	}
	std::vector<int>::const_iterator oneButLast_it = literals.cend();
	--oneButLast_it;
	while (literals_it != oneButLast_it) {
		prevTseitin = tseitin;
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// ( ~tn | tn )
		gt->add({-tseitin, prevTseitin});
		// ( ~tn | ln | ~s(ln) )
		gt->add({-tseitin, lit, -symLit});
		// ( tn | ~tn-1 | ~ln)
		gt->add({tseitin, -prevTseitin, -lit});
		// ( tn | ~tn-1 | s(ln))
		gt->add({tseitin, -prevTseitin, symLit});
		// ( ~tn | ~ln+1 | s(ln+1) )
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		gt->add({-tseitin, -lit, symLit});
	}
}

/**
 * 	Given a symmetry in the form of two lists of domain elements which represent a bijection, this method adds CNF-clauses to the theory which break the symmetry.
 *
 * 	This variation induces extra solutions by relaxing the constraints on the tseitin variables. The advantage is less and smaller clauses.
 * 
 *  These clausal formulas are the same as implemented by BreakID (bitbucket.org/krr/breakid).
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
		gt->add({-lit, symLit});
	}
	if (literals.size() > 1) {
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// (t1 | ~l1)
		gt->add({tseitin, -lit});
		// (t1 | s(l1))
		gt->add({tseitin, symLit});
		// (~t1 | ~l2 | s(l2))
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		gt->add({-tseitin, -lit, symLit});
	}
	std::vector<int>::const_iterator oneButLast_it = literals.cend();
	--oneButLast_it;
	while (literals_it != oneButLast_it) {
		prevTseitin = tseitin;
		tseitin = gt->translator()->createNewUninterpretedNumber();
		// ( tn | ~tn-1 | ~ln)
		gt->add({tseitin, -prevTseitin, -lit});
		// ( tn | ~tn-1 | s(ln))
		gt->add({tseitin, -prevTseitin, symLit});
		// ( ~tn | ~ln+1 | s(ln+1) )
		++literals_it;
		lit = *literals_it;
		++symLiterals_it;
		symLit = *symLiterals_it;
		gt->add({-tseitin, -lit, symLit});
	}
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

UFNode* UFSymbolArg::find(UFNode* in) const {
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

void detectInterchangeability(std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms, const AbstractTheory* t, const Structure* s, const Term* obj) {
	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "*** DETECTING SYMMETRY ***" << std::endl;
	}

	AbstractTheory* theo = t->clone();
	
	if (obj!=nullptr) {
		Term* obj_clone = obj->clone();
		DomainTerm* dummyTerm = new DomainTerm(obj->sort(),s->inter(obj->sort())->last(),TermParseInfo());
		theo->add(new PredForm(SIGN::POS,get(STDPRED::LT,obj_clone->sort()),{obj_clone,dummyTerm},FormulaParseInfo()));
	}
	
	getIntchGroups(theo,s,out_groups,out_syms);
	
    if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
      for (auto icg : out_groups) {
        icg->print(clog);
      }
      for(auto sym: out_syms){
        sym->print(clog);
      }
    }
	
	theo->recursiveDelete();
}

void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms, const std::vector<std::pair<PFSymbol*, unsigned int> >& forcedSymbArgs) {
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
	for(auto sa: forcedSymbArgs) {
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

    // Calculate both interchangeable domains and local domain permutations
	for (auto ichset : intersets) {
		// TODO: this is a rather ugly hack. ichset should not have been derived before...
		bool hasCorrectSymbArgs = (forcedSymbArgs.size()==0); // so if no forcedsymbargs are provided, look for symmetry in all symbargs in the theory. Otherwise, look for those connected to forcedsymbargs
		for(auto sa:forcedSymbArgs) {
			if (ichset->symbolargs.hasArgPos(sa.first,sa.second)) {
				hasCorrectSymbArgs=true;
			}
		}
		if (!hasCorrectSymbArgs) {
			continue;
		}
        
        bool allTwoValued = true;
        for(auto symb: ichset->symbolargs.symbols){
          if(!s->inter(symb)->approxTwoValued()){
            allTwoValued=false;
            break;
          }
        }
        if(allTwoValued){
          continue; // no symmetry breaking possible
        }
		
		ichset->calculateInterchangeableSets();
		if (getOption(IntType::VERBOSE_SYMMETRY) > 2) {
			ichset->print(clog);
		}
		ichset->getIntchGroups(out_groups);
        
        
        bool hasNonTrivialSymbol = false;
        for(auto argpos: ichset->symbolargs.argPositions){
          if(argpos.second.size()>1){ // TODO: also require the table to be non-trivial (totally true, false or unknown)
            hasNonTrivialSymbol=true;
            break;
          }
        }
        if(!hasNonTrivialSymbol){
          continue; // all symmetry is broken by interchangeability
        }        
        saucy_::Graph sg = saucy_::Graph(ichset);
        sg.addInterpretations(s);  
        sg.runSaucy(out_syms);
	}

	for (size_t i = 0; i < intersets.size(); ++i) {
		delete intersets[i];
	}
}

void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms) {
	std::vector<std::pair<PFSymbol*, unsigned int> > tmp;
	getIntchGroups(theo, s, out_groups, out_syms, tmp);
}

void ArgPosSet::addArgPos(PFSymbol* symb, unsigned int arg){
  if(argPositions.count(symb)==0){
    std::set<unsigned int> newSet;
    argPositions[symb]=newSet;
  }
  argPositions.at(symb).insert(arg);
  symbols.insert(symb);
}
bool ArgPosSet::hasArgPos(PFSymbol* symb, unsigned int arg){
  if(argPositions.count(symb)==0){
    return false;
  }
  return argPositions.at(symb).count(arg);
}

void ArgPosSet::print(std::ostream& ostr){
  for(auto paar: argPositions){
    paar.first->put(ostr);
    ostr << "/";
    for(auto arg: paar.second) {
        ostr << arg << " ";
    }
  }
}

InterchangeabilityGroup::InterchangeabilityGroup(std::vector<const DomainElement*>& domels, std::vector<PFSymbol*>& symbs3val, 
		ArgPosSet& symbargs) {
	for (auto de : domels) {
		elements.insert(de);
	}
	for(auto symb: symbs3val) {
        Assert(symbargs.argPositions.count(symb)>0);
        for(auto arg: symbargs.argPositions.at(symb)){
          symbolargs.addArgPos(symb,arg);
        }
	}
}

void InterchangeabilityGroup::print(std::ostream& ostr) {
	symbolargs.print(ostr);
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

bool InterchangeabilityGroup::hasSymbArg(PFSymbol* symb, unsigned int arg) {
	return symbolargs.hasArgPos(symb,arg);
}

void InterchangeabilityGroup::addBreakingClauses(AbstractGroundTheory* gt, Structure* struc, bool nbModelsEquivalent) const {
  std::vector<Symmetry*> generators;
  getGoodGenerators(generators);
  for(auto gen: generators){
    gen->addBreakingClauses(gt,struc,nbModelsEquivalent);
    delete gen;
  }
}

void InterchangeabilityGroup::getGoodGenerators(std::vector<Symmetry*>& out) const{
  Assert(elements.size()>1);
  std::map<DomainElement,const DomainElement*> sortedEls; // sort the els according to the internal sort (crucial for getting the right generators for complete breaking)
  for(auto el: elements){
    sortedEls[*el]=el;
  }

  auto sortedIt = sortedEls.cbegin();
  auto first = sortedIt->second;
  ++sortedIt;
  
  while(sortedIt!=sortedEls.cend()){
    Symmetry* newSym = new Symmetry(symbolargs);
    auto second = sortedIt->second;
    newSym->addImage(first,second);
    newSym->addImage(second,first);
    out.push_back(newSym);
    first=second;
    ++sortedIt;
  }
}

bool InterchangeabilitySet::add(PFSymbol* p, unsigned int arg) {
    symbolargs.addArgPos(p,arg);
    Sort* srt = p->sort(arg);
	sorts.insert(srt);
    symbolargs.addArgPos(srt->pred(),0); // don't forget to add the sort predicates as well (type hierarchies...)!
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

void InterchangeabilitySet::getDomain(std::unordered_set<const DomainElement*>& allElements, bool includeConstants){
  	// run over all sorts to find relevant domain elements
	for (auto s : sorts) {
        if (!_struct->inter(s)->finite()) {
          allElements.clear();
          return; // not going to detect symmetry over an infinite domain ;)
        }
		auto sortiter = _struct->inter(s)->sortBegin();
		while (!sortiter.isAtEnd()) {
			if (includeConstants || occursAsConstant.count(*sortiter) == 0) { // ignore domain elements occurring as constants in theory
				allElements.insert(*sortiter);
			}
			++sortiter;
		}
	}
}

void InterchangeabilitySet::calculateInterchangeableSets() {
	std::unordered_set<const DomainElement*> allElements;
    getDomain(allElements);
    
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
		for (auto symb : symbolargs.symbols) {
			if (!_struct->inter(symb)->approxTwoValued()) {
				symbs3val.push_back(symb);
			}
		}
		if (symbs3val.size()==0) {
			continue;
		}
		out.push_back(new InterchangeabilityGroup(*it.second,symbs3val,symbolargs));
	}
}

void InterchangeabilitySet::print(std::ostream& ostr) {
    symbolargs.print(ostr);
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
	for (auto sa : ics->symbolargs.argPositions) {
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

bool checkTableForSwapConsistency(PredTable* table, const DomainElement* first, const DomainElement* second, std::set<unsigned int>& args) {
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
			if (args.count(i) > 0) {
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
	for (auto sa : ics->symbolargs.argPositions) {
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

Symmetry::Symmetry(const ArgPosSet& ap):argpositions(ap){}

void Symmetry::addImage(const DomainElement* first, const DomainElement* second){
  if(first==second){
    return;
  }
  image[first]=second;
}

void Symmetry::print(std::ostream& ostr){
  argpositions.print(ostr);
  clog << ": ";
  for(auto paar: image){
    ostr << "(";
    paar.first->put(ostr); 
    clog << " "; 
    paar.second->put(ostr); 
    ostr << ")";
  }
  ostr << std::endl;
}

bool Symmetry::transformToSymmetric(std::vector<const DomainElement*>& tuple, std::set<unsigned int>& positions){
  bool hasChanged = false;
  for(unsigned int i: positions){
    if(image.count(tuple[i])){
      tuple[i]=image[tuple[i]];
      hasChanged = true;
    }
  }
  return hasChanged;
}

void Symmetry::getGroundSymmetry(AbstractGroundTheory* gt, Structure* struc, std::unordered_map<unsigned int, unsigned int>& groundSym, std::vector<unsigned int>& groundOrder){
    for(auto symbarg: argpositions.argPositions) {
      PFSymbol* symb = symbarg.first;
      if (struc->inter(symb)->approxTwoValued()) {
        continue; // no need to construct sym breaking constraints for this symbol
      }
      
      // run over pt get unknown tuples
      auto inter = struc->inter(symb);      
      auto ptit = inter->pt()->begin();
      for(;!ptit.isAtEnd(); ++ptit){  // iterating over possibly true tuples
        ElementTuple eltup = *ptit;
        ElementTuple symtup = eltup;
        if(!inter->isUnknown(eltup,true) || !transformToSymmetric(symtup,symbarg.second)){
          continue; // no use adding constraints for known tuples or for tuples mapping to themselves
        }
        Assert(inter->isUnknown(symtup,true));
        unsigned int groundtup = gt->translator()->translateReduced(symb, eltup, false);
        groundOrder.push_back(groundtup);
        groundSym[groundtup]=gt->translator()->translateReduced(symb, symtup, false);
      }
    }
}

void Symmetry::getUsefulAtoms(std::unordered_map<unsigned int, unsigned int>& groundSym, std::vector<unsigned int>& groundOrder, std::vector<int>& out_orig, std::vector<int>& out_sym){
  std::unordered_set<unsigned int> useful;
  for(int i=groundOrder.size()-1; i>=0; --i){
    unsigned int last = groundOrder[i]; 
    if(useful.count(last)){
      continue; // not last in the cycle
    }
    // last in the cycle
    unsigned int sym = groundSym[last];
    while(sym!=last){
      useful.insert(sym); // images of last are useful, but not last itself
      sym=groundSym[sym];
    }
  }
  for(auto atom: groundOrder){
    if(useful.count(atom)>0){
      out_orig.push_back(atom);
      out_sym.push_back(groundSym[atom]);
    }
  }
}

void Symmetry::addBreakingClauses(AbstractGroundTheory* gt, Structure* struc, bool nbModelsEquivalent){
  std::unordered_map<unsigned int, unsigned int> groundSym;
  std::vector<unsigned int> groundOrder;
  getGroundSymmetry(gt,struc,groundSym,groundOrder);
  std::vector<int> lits;
  std::vector<int> sym_lits;
  getUsefulAtoms(groundSym,groundOrder,lits,sym_lits);
  if (nbModelsEquivalent) {
    addSymBreakingClausesToGroundTheory(gt, lits, sym_lits);
  } else {
    addSymBreakingClausesToGroundTheoryShortest(gt, lits, sym_lits);
  }
}