/************************************
	symmetry.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/
#include "symmetry.hpp"
#include "structure.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

/**
 * This method creates an initial set of sorts and their corresponding IVSets (of domain elements) based on the sort hierarchy.
 * An initial IVSet is the domainelements of the sort not present in the domain of any of its children
 */
map<const Sort*, IVSet> initializeIVSets(const AbstractStructure* s){
	map<const Sort*, IVSet> result; 
	for(map<string, set<Sort*>>::const_iterator map_it= s->vocabulary()->firstsort(); map_it !=s->vocabulary()->lastsort(); ++map_it){
		for(set<Sort*>::const_iterator sort_it = map_it->second.begin(); sort_it != map_it->second.end(); ++sort_it){
			IVSet uniqueDomainElements;
			const SortTable* parent = s->inter(*sort_it);
			vector<const SortTable*> kids;
			for(set<Sort*>::const_iterator kids_it = (*sort_it)->children().begin(); kids_it != (*sort_it)->children().end(); ++kids_it){
				kids.push_back(s->inter(*kids_it));
			}			
			for(SortIterator parent_it = parent->sortbegin(); parent_it.hasNext(); ++parent_it){
				bool isUnique = true;
				for(vector<const SortTable*>::const_iterator kids_it2 = kids.begin(); kids_it2 != kids.end() && isUnique; ++kids_it2 ){
					isUnique = not (*kids_it2)->contains(*parent_it);
				}
				if(isUnique){
					uniqueDomainElements.insert(*parent_it);
				}
			}
			if(uniqueDomainElements.size()>1){
				result[*sort_it]=uniqueDomainElements;
			}
		}
	}
	return result;
}

class TheorySymmetryAnalyzer : public TheoryVisitor {
	private:
		const AbstractStructure* 						structure_;
		set<const Sort*> 						forbiddenSorts_;
		map<const DomainElement*,const Sort*> 	forbiddenElements_;
		
		void markAsUnfitForSymmetry(const vector<Term*>&);
		void markAsUnfitForSymmetry(const Sort*);
		void markAsUnfitForSymmetry(const DomainElement*, const Sort*);

		const AbstractStructure* getStructure() const {return structure_; }
		
	public:
		TheorySymmetryAnalyzer(const AbstractStructure* s): structure_(s) {};
		void visit(const PredForm*);
		void visit(const FuncTerm*);
		void visit(const DomainTerm*);
		void visit(const EqChainForm*);
		
		set<const Sort*> 						getForbiddenSorts() 	const { return forbiddenSorts_; }
		map<const DomainElement*,const Sort*> 	getForbiddenElements() 	const { return forbiddenElements_; }
};

void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const vector<Term*>& subTerms){
	for(unsigned int it=0; it<subTerms.size(); it++){
		markAsUnfitForSymmetry(subTerms.at(it)->sort());
	}
}

void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const Sort* s){
	forbiddenSorts_.insert(s);
}

void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const DomainElement* e, const Sort* s){
	forbiddenElements_[e]=s;
}

void TheorySymmetryAnalyzer::visit(const PredForm* f){
	if(f->symbol()->builtin() || f->symbol()->overloaded()){
		if(f->symbol()->name()!="=/2"){
			for(unsigned int it=0; it<f->args().size(); it++){
				markAsUnfitForSymmetry(f->subterms().at(it)->sort());
			}			
			markAsUnfitForSymmetry(f->args());
		}
	}
	traverse(f);
}

void TheorySymmetryAnalyzer::visit(const FuncTerm* t){
	if(t->function()->builtin() || t->function()->overloaded()){
		if(t->function()->name()=="MIN/0"){
			SortTable* st = getStructure()->inter(t->function()->outsort());
			if(not st->approxempty()){
				markAsUnfitForSymmetry(st->first(),t->function()->outsort());
			}
		}else if(t->function()->name()=="MAX/0"){
			SortTable* st = getStructure()->inter(t->function()->outsort());
			if(not st->approxempty()){
				markAsUnfitForSymmetry(st->last(),t->function()->outsort());
			}
		}else{
			markAsUnfitForSymmetry(t->args());
		}
	}
	traverse(t);
}

void TheorySymmetryAnalyzer::visit(const DomainTerm* t){
	markAsUnfitForSymmetry(t->value(),t->sort());
}

void TheorySymmetryAnalyzer::visit(const EqChainForm* ef){
	Formula* f = ef->clone();
	f = FormulaUtils::remove_eqchains(f,getStructure()->vocabulary());
	f->accept(this);
	f->recursiveDelete();
	
	/*
	 * old code:
	 * 	for(unsigned int i=0; i<f->comps().size(); i++){
	 *		if(f->comps().at(i)!='=' || f->comps().at(i)!='!='){
	 *			markAsUnfitForSymmetry(f->subterm(i)->sort());
	 *			markAsUnfitForSymmetry(f->subterm(i+1)->sort());
	 *		}
	 *	}
	 */
}


/**
 * This method adjusts the set of potential dontcares according to not strictly FO structures in the theory.
 * For now, 2 checks are made. The first one checks whether a domain element occurs in the theory. 
 * If so, the domain element will be removed from the IVSet of its Sort in dontcares. If the IVSet turns
 * empty, the IVSet and corresponding Sort is removed from dontcares. The second one checks whether a certain
 * Sort is used in asymmetrical built-in functions. If so, the Sort and its corresponding IVSet will be removed 
 * from dontcares. 
 */
void adjustIVSetsByTheory(const AbstractTheory* t, const AbstractStructure* s, map<const Sort*, IVSet>& dontcares){
	TheorySymmetryAnalyzer* tsa = new TheorySymmetryAnalyzer(s);
	t->accept(tsa);
	for(set<const Sort*>::iterator forbiddenSort_it=tsa->getForbiddenSorts().begin(); 
			forbiddenSort_it!=tsa->getForbiddenSorts().end(); 
			++forbiddenSort_it){
		dontcares.erase(*forbiddenSort_it);
	}
	for(map<const DomainElement*,const Sort*>::const_iterator forbiddenDE_it=tsa->getForbiddenElements().begin(); 
			forbiddenDE_it!=tsa->getForbiddenElements().end(); 
			++forbiddenDE_it){
		map<const Sort*, IVSet>::iterator IV_it = dontcares.find(forbiddenDE_it->second);
		if(IV_it != dontcares.end()){
			IV_it->second.erase(forbiddenDE_it->first);
			if(IV_it->second.size()<2){
				dontcares.erase(IV_it);
			}
		}
	}
	delete tsa;
}


void findCares(const AbstractStructure* s, PFSymbol* pf, map<const Sort*, IVSet>& dontcares, map<const Sort*, IVSet>& cares){
	const PredInter* inter = s->inter(pf);
	if(!inter->ct()->approxempty() || !inter->cf()->approxempty()){ // all sorts for this predicate are cares
		for(set<Sort*>::const_iterator sortIt = pf->allsorts().begin(); sortIt != pf->allsorts().end(); ++sortIt){
			map<const Sort*, IVSet>::iterator dc_it = dontcares.find(*sortIt);
			if(dc_it!=dontcares.end() && dc_it->second.size()>1){
				cares.insert(*dc_it); // pair<const Sort*, IVSet>
				dontcares.erase(dc_it);
			}
		}
	}
}

void findDontCares(const AbstractStructure* s, map<const Sort*,IVSet>& dontcares, map<const Sort*,IVSet>& cares, map<const Sort*,vector<IVSet> >& ivsets){	
	for(map<string, Predicate*>::const_iterator mapIt= s->vocabulary()->firstpred(); mapIt !=s->vocabulary()->lastpred(); ++mapIt){
		PFSymbol* pf = mapIt->second;
		findCares(s, pf, dontcares, cares);
	}
	for(map<string, Function*>::const_iterator mapIt= s->vocabulary()->firstfunc(); mapIt !=s->vocabulary()->lastfunc(); ++mapIt){
		PFSymbol* pf = mapIt->second;
		findCares(s, pf, dontcares, cares);
	}
	for(map<const Sort*,IVSet>::const_iterator dc_it = dontcares.begin(); dc_it!=dontcares.end(); ++dc_it){
		if(dc_it->second.size()>1){
			ivsets[dc_it->first].push_back(dc_it->second);			
		}
	}
}

void processCares(const AbstractStructure* s, map<const Sort*,IVSet>& cares, map<const Sort*,vector<IVSet> >& ivsets){
	//TODO
}

map<const Sort*,vector<IVSet> > findIVSets(const AbstractTheory* t, const AbstractStructure* s){
	map<const Sort*, IVSet> dontcares = initializeIVSets(s);
	adjustIVSetsByTheory(t, s, dontcares);
	map<const Sort*, IVSet> cares;
	map<const Sort*,vector<IVSet> > ivsets;
	findDontCares(s, dontcares, cares, ivsets);
	processCares(s, cares, ivsets);
	
	return ivsets;
}

