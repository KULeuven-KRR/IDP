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
    Assert(symLiterals.size()==literals.size());
    if(literals.size()==0){
      return;
    }
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
    Assert(symLiterals.size()==literals.size());
    if(literals.size()==0){
      return;
    }
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
        delete pair.second;
	}
	for (auto pair : VARnodes) {
		delete pair.second;
	}
	for (auto node : DEnodes) {
		delete node;
	}
	delete forbiddenNode;
}

UFNode* UFSymbolArg::get(DecArgPos dap) {
	if (SAnodes.count(dap) == 0) {
      SAnodes.insert({dap, new SymbolArgumentNode(dap)});
	}
	return SAnodes.at(dap);
}

UFNode* UFSymbolArg::get(const Variable* var) {
	if (VARnodes.count(var) == 0) {
		VARnodes.insert({var, new VariableNode(var)});
	}
	return VARnodes.at(var);
}

UFNode* UFSymbolArg::get(const Sort* s, const DomainElement* de) {
	DomainElementNode* result = new DomainElementNode(s, de);
	DEnodes.push_back(result);
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
        out.insert({find(pair.second), pair.second});    
	}
	for (auto pair : VARnodes) {
		out.insert({find(pair.second), pair.second});
	}
	for (auto node : DEnodes) {
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
				ostr << "}" << std::endl;
			}
			currentrep = pair.first;
			ostr << "{";
		}
		pair.second->put(ostr);
		ostr << ",";
	}
	ostr << "}" << std::endl;
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
        unsigned int decomposition=0;
        if(_structure->inter(f->symbol())->approxTwoValued()){
          decomposition=decomposition_counter;
          ++decomposition_counter;
        }
		for (unsigned int n = 0; n < f->subterms().size(); ++n) {
			f->subterms()[n]->accept(this);
			disjointSet.merge(disjointSet.get({decomposition,f->symbol(), n}), subNode);
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
        unsigned int decomposition=0;
        if(_structure->inter(t->function())->approxTwoValued()){
          decomposition=decomposition_counter;
          ++decomposition_counter;
        }
		unsigned int n;
		for (n = 0; n < t->subterms().size(); ++n) {
			t->subterms()[n]->accept(this);
			disjointSet.merge(disjointSet.get({decomposition,t->function(), n}), subNode);
		}
		Assert(n == t->function()->nrSorts() - 1);
		subNode = disjointSet.get({decomposition,t->function(), n}); // NOTE: output argument position is not 0 but arity+1 because of PredTable implementation...
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

void detectSymmetry(std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms, const AbstractTheory* t, const Structure* s, const Term* obj) {
	AbstractTheory* theo = t->clone();
    
    // hack: add objective function to theory
	if (obj!=nullptr) {
		Term* obj_clone = obj->clone();
		DomainTerm* dummyTerm = new DomainTerm(obj->sort(),s->inter(obj->sort())->last(),TermParseInfo());
		theo->add(new PredForm(SIGN::POS,get(STDPRED::LT,obj_clone->sort()),{obj_clone,dummyTerm},FormulaParseInfo()));
	}
	
    if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		clog << "pushing quantifiers completely..." << std::endl;
	}
	theo = FormulaUtils::pushQuantifiersCompletely(theo);
	if (getOption(IntType::VERBOSE_SYMMETRY) > 2) {
		theo->put(clog);
	}
	
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		clog << "partitioning connected argument positions in decomposed theory..." << std::endl;
	}
	InterchangeabilityAnalyzer ia = InterchangeabilityAnalyzer(s);
	ia.analyze(theo);
	
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		ia.disjointSet.printPartition(clog);
	}
	if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
		clog << "for each partition, detecting interchangeable subdomains and/or generator symmetries..." << std::endl;
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
        bool allTwoValued = true;
        for(auto dap: ichset->argpositions){
          if(!s->inter(dap.symbol)->approxTwoValued()){
            allTwoValued=false;
            break;
          }
        }
        if(allTwoValued){
          continue; // no symmetry breaking possible
        }

        ichset->initializeSymbargs();
		ichset->calculateInterchangeableSets();
		if (getOption(IntType::VERBOSE_SYMMETRY) > 2) {
			ichset->print(clog);
		}
		ichset->getIntchGroups(out_groups);
        
        bool hasNonTrivialSymbol = false;
        for(auto argpos: ichset->symbargs){
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
	theo->recursiveDelete();
}

void ArgPosSet::addArgPos(PFSymbol* symb, unsigned int arg){
  if(argPositions.count(symb)==0){
    std::set<unsigned int> newSet;
    argPositions[symb]=newSet;
    symbols.insert(symb);
  }
  argPositions.at(symb).insert(arg);
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
    for(auto arg: paar.second) {
        ostr << "|" << arg;
    }
    ostr << " ";
  }
}

InterchangeabilityGroup::InterchangeabilityGroup(std::vector<const DomainElement*>& domels, ArgPosSet& symbargs) {
	for (auto de : domels) {
		elements.insert(de);
	}
    symbolargs=symbargs;
}

void InterchangeabilityGroup::print(std::ostream& ostr) {
	symbolargs.print(ostr);
	ostr << "-> {";
	for (auto de : elements) {
		de->put(ostr);
		ostr << ",";
	}
	ostr << "}" << std::endl;
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

void DecArgPos::print(std::ostream& ostr){
  symbol->put(ostr);
  ostr << "_" << decomposition << "|" << argument;
}
bool DecArgPos::equals(const DecArgPos& other) const{
  return symbol==other.symbol && decomposition==other.decomposition && argument==other.argument;
}
size_t DecArgPos::getHash() const{
  size_t hash = (size_t) symbol;
  hash^= argument;
  hash^=decomposition << 10;
  return hash;
}

bool InterchangeabilitySet::add(DecArgPos& dap) {
    argpositions.insert(dap);
    add(dap.symbol->sort(dap.argument)); // add sort
	return true;
}

bool InterchangeabilitySet::add(const Sort* s) {
	sorts.insert(s);
    argpositions.insert({0,s->pred(),0}); // don't forget to add the sort predicates as well
    // we can safely say this is the 0th decomposition of this sort symbol, as sort symbols have arity 0.
    for(auto sub: s->children()){
      add(sub);
    }
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

void InterchangeabilitySet::initializeSymbargs(){
    symbargs.clear();
  
    // first sort all entries based on symbol and decomposition
    std::unordered_map<PFSymbol*, unordered_multimap<unsigned int, unsigned int> > symb2decs2args;
    for(auto dap: argpositions){
      if(symb2decs2args.count(dap.symbol)==0){
        symb2decs2args[dap.symbol]=unordered_multimap<unsigned int, unsigned int>();
      }
      symb2decs2args[dap.symbol].insert({dap.decomposition,dap.argument});
    }
    
    for(auto p2d2a: symb2decs2args){
      // for a given symbol, deduplicate decompositions (see http://stackoverflow.com/questions/13982763/removing-duplicates-from-a-list-of-sets)
      std::list<std::set<unsigned int> > set_of_sets;
      auto myumm = p2d2a.second;
      for ( unsigned int i = 0; i < myumm.bucket_count(); ++i) {
        std::set<unsigned int> current_set;
        for ( auto local_it = myumm.begin(i); local_it!= myumm.end(i); ++local_it ){
          current_set.insert(local_it->second);
        }
        if(current_set.size()>0){
          set_of_sets.push_back(current_set);
        }
      }
      set_of_sets.sort();
      set_of_sets.unique();
    
      // add deduplicated argument lists for the symbol
      for(auto args: set_of_sets){
        symbargs.push_back({p2d2a.first, args});
      }
    }
}

void InterchangeabilitySet::getThreeValuedArgPositions(ArgPosSet& out){
    for(auto dap: argpositions){
      if (!_struct->inter(dap.symbol)->approxTwoValued()) {
        out.addArgPos(dap.symbol,dap.argument);
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
    // Make an ArgPosSet based on only three-valued symbols
    ArgPosSet threeval;
    getThreeValuedArgPositions(threeval);
    if(threeval.symbols.size()==0){
      return; // no symbols to break symmetry for
    }
	for (auto it : partition) {
		if (it.second->size() < 2) {
			continue; // only one element to generate locdomsym with, skip this trivial group
		}
        auto newgroup = new InterchangeabilityGroup(*it.second,threeval);
		out.push_back(newgroup);
        if (getOption(IntType::VERBOSE_SYMMETRY) > 1) {
          newgroup->print(std::clog);
        }
	}
}

void InterchangeabilitySet::print(std::ostream& ostr) {
    for(auto argpos: argpositions){
      argpos.print(ostr);
      ostr << " ";
    }
	ostr << ": ";
	for (auto paar : partition) {
		for (auto de : *paar.second) {
			de->put(ostr);
			ostr << " ";
		}
		ostr << "| ";
	}
	ostr << std::endl;
}

size_t getTableHash(TableIterator table_it, const DomainElement* de, std::set<unsigned int>& permutedArgs){
  size_t permutedHash = 0; // hash based on occurrence count of de at permuted positions
  size_t unpermutedHash = 0; // hash based on domain elements at unpermuted positions in a tuple containing de
  while (!table_it.isAtEnd()) {
    const ElementTuple& origTuple = *table_it;
    bool containsDE = false;
    size_t tupleHash=0;
    for(unsigned int i=0; i<origTuple.size(); ++i){
      if(permutedArgs.count(i)>0){
        if(origTuple[i]==de){
          permutedHash+= (1 + i); // add i parameter to take position in tuple into account
          containsDE=true;
        }
      }else{
        tupleHash+= (((size_t) origTuple[i]) + i); // add i parameter to take position in tuple into account
      }
    }
    if(containsDE){
      unpermutedHash^=tupleHash;
    }
    ++table_it;
  }
  return unpermutedHash^permutedHash;
}

ElementOccurrence::ElementOccurrence(InterchangeabilitySet* intset, const DomainElement* de) : ics(intset), domel(de) {
	hash = (size_t) ics;
    ElementTuple deTup;
    deTup.push_back(de);
	for (auto sa: ics->symbargs) {
		PFSymbol* symb = sa.first;
        PredInter* pi = ics->_struct->inter(symb);
		if (symb->nrSorts() == 1) { // use unary symbols to create hash function
            Assert(sa.second.count(0)>0); // the one sort is also the argument of concern
			if (pi->isTrue(deTup)) {
				hash^= ((size_t) symb) >> 1;
			}else if (pi->isFalse(deTup)){
                hash^= ((size_t) symb) << 1;
            }
		}else{ // symb->nrSorts()>1
            hash^=(size_t) symb;
            hash^=getTableHash(pi->ct()->begin(), de, sa.second);
            if(!pi->approxTwoValued()){
              hash^=getTableHash(pi->cf()->begin(), de, sa.second);
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
	if (ics != other.ics) {
		return false;
	}
	for (auto sa : ics->symbargs) {
		PredInter* pi = ics->_struct->inter(sa.first);
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

void SymbolArgumentNode::put(std::ostream& outstr) {
    dap.print(outstr);
}

bool SymbolArgumentNode::addTo(InterchangeabilitySet * ichset) {
	return ichset->add(dap);
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
  ostr << "-> ";
  for(auto paar: image){
    ostr << "(";
    paar.first->put(ostr); 
    ostr << " "; 
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