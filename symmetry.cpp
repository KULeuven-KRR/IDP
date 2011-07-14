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
#include <sstream>
#include <iostream>  //TODO: wissen na debuggen :)

using namespace std;

/**********
 * Miscellaneous methods
 **********/

bool isSortForSymmetry(Sort* sort, const AbstractStructure* s){
	return s->inter(sort)->approxfinite() && !s->inter(sort)->approxempty();
}

bool hasTrivialInterpretation(const AbstractStructure* s, PFSymbol* relation){
	return s->inter(relation)->pt()->approxempty() || s->inter(relation)->pf()->approxempty();
}

bool hasInterpretation(const AbstractStructure* s, PFSymbol* relation){
	return !(s->inter(relation)->ct()->approxempty() && s->inter(relation)->cf()->approxempty());
}

bool isBinarySymmetryInPredTable(const PredTable* table, const vector<unsigned int>& argumentNrs, const DomainElement* first, const DomainElement* second){
	bool isSymmetry = true;
	for(TableIterator table_it = table->begin(); table_it.hasNext() && isSymmetry; ++table_it){
		bool symmetricalIsDifferent = false;
		ElementTuple tuple = *table_it;
		ElementTuple symmetricalTuple = tuple;
		for(unsigned int i=0; i<argumentNrs.size(); ++i){
			if(tuple[i]==first){
				symmetricalTuple[i]=second;
				symmetricalIsDifferent=true;
			}else if(tuple[i]==second){
				symmetricalTuple[i]=first;
				symmetricalIsDifferent=true;
			}
		}
		if(symmetricalIsDifferent){
			isSymmetry=table->contains(symmetricalTuple);
		}
	}
	return isSymmetry;
}

bool isBinarySymmetry(const AbstractStructure* s, const DomainElement* first, const DomainElement* second, PFSymbol* relation, Sort* sort){
	if(!hasInterpretation(s,relation)){
		return true;
	}
	vector<unsigned int> argumentNrs = relation->argumentNrs(sort);
	if(argumentNrs.size()==0){
		return true;
	}
	const PredInter* inter = s->inter(relation);
	const PredTable* table = inter->ct();
	bool result = isBinarySymmetryInPredTable(table, argumentNrs, first, second);
	if(!inter->approxtwovalued() && result==true){
		table = inter->cf();
		result = isBinarySymmetryInPredTable(table, argumentNrs, first, second);
	}
	return result;
}

/**********
 * Object to count occurrences using memoization
 **********/

class OccurrencesCounter{
	private:
		// Attributes
		const AbstractStructure* 							structure_;
		map<pair<const PFSymbol*,const Sort*>,map<const DomainElement*,pair<int,int> > >	occurrences_; //<! a pair of ints for each domain element representing its occurrences in ct and cf for each relation-sort combination
		
		// Mutators
		map<const DomainElement*,pair<int,int> > count(PFSymbol*, Sort*);
	
	public:
		// Constructors
		OccurrencesCounter(const AbstractStructure* s) : structure_(s) {}
		
		// Inspectors
		pair<int,int>							getOccurrences(const DomainElement*, PFSymbol*, Sort* );
		map<const DomainElement*, vector<int> > getOccurrences(const set<const DomainElement*>&, const set<PFSymbol*>&, const set<Sort*>& );
		
		string									to_string() const;
};

// @pre: !relation->argumentNrs(sort).empty()
map<const DomainElement*,pair<int,int> > OccurrencesCounter::count(PFSymbol* relation, Sort* sort){
	assert(!relation->argumentNrs(sort).empty());
	vector<unsigned int> argumentNrs = relation->argumentNrs(sort);
	map<const DomainElement*,pair<int,int> > result;
	const PredTable* ct = structure_->inter(relation)->ct();
	for(TableIterator ct_it = ct->begin(); ct_it.hasNext(); ++ct_it){
		for(unsigned int i=0; i<argumentNrs.size(); ++i){
			const DomainElement* element = (*ct_it)[i];
			map<const DomainElement*,pair<int,int> >::iterator result_it = result.find(element);
			if(result_it==result.end()){
				result[element]=pair<int,int>(1,0);
			}else{
				++(result_it->second.first);
			}
		}
	}
	const PredTable* cf = structure_->inter(relation)->cf();
	for(TableIterator cf_it = cf->begin(); cf_it.hasNext(); ++cf_it){
		for(unsigned int i=0; i<argumentNrs.size(); ++i){
			const DomainElement* element = (*cf_it)[i];
			map<const DomainElement*,pair<int,int> >::iterator result_it = result.find(element);
			if(result_it==result.end()){
				result[element]=pair<int,int>(0,1);
			}else{
				++(result_it->second.second);
			}
		}
	}
	occurrences_[pair<const PFSymbol*,const Sort*>(relation,sort)]=result;
	return result;
}

// @pre: !relation->argumentNrs(sort).empty()
pair<int,int> OccurrencesCounter::getOccurrences(const DomainElement* element, PFSymbol* relation, Sort* sort){
	assert(!relation->argumentNrs(sort).empty());
	map<pair<const PFSymbol*,const Sort*>,map<const DomainElement*,pair<int,int> > >::iterator occurrences_it = occurrences_.find(pair<const PFSymbol*,const Sort*>(relation,sort) );
	if(occurrences_it!=occurrences_.end()){
		map<const DomainElement*,pair<int,int> >::const_iterator result_it = occurrences_it->second.find(element);
		if(result_it!=occurrences_it->second.end()){
			return result_it->second;
		}else{
			return pair<int,int>(0,0);
		}		
	}else{
		map<const DomainElement*,pair<int,int> > counts = count(relation, sort);
		map<const DomainElement*,pair<int,int> >::const_iterator counts_it = counts.find(element);
		if(counts_it!=counts.end()){
			return counts_it->second;
		}else{
			return pair<int,int>(0,0);
		}
	}
}

map<const DomainElement*, vector<int> > OccurrencesCounter::getOccurrences(const set<const DomainElement*>& elements, const set<PFSymbol*>& relations, const set<Sort*>& sorts){
	map<const DomainElement*, vector<int> > result;
	for(set<const DomainElement*>::const_iterator elements_it=elements.begin(); elements_it!=elements.end(); ++elements_it){
		vector<int> values;
		for(set<PFSymbol*>::const_iterator relations_it=relations.begin(); relations_it!=relations.end(); ++relations_it){
			if(hasInterpretation(structure_,*relations_it)){
				for(set<Sort*>::const_iterator sorts_it=sorts.begin(); sorts_it!=sorts.end(); ++sorts_it){
					if(!(*relations_it)->argumentNrs(*sorts_it).empty()){
						pair<int,int> temp = getOccurrences(*elements_it,*relations_it,*sorts_it); 
						values.push_back(temp.first);
						values.push_back(temp.second);
					}
				}
			}
		}
		result[*elements_it]=values;
	}
	return result;
}

string OccurrencesCounter::to_string() const{
	stringstream ss;
	ss << "COUNTER:" << endl;
	for(map<pair<const PFSymbol*,const Sort*>,map<const DomainElement*,pair<int,int> > >::const_iterator occurrences_it= occurrences_.begin(); occurrences_it!=occurrences_.end(); ++occurrences_it){
		ss << occurrences_it->first.first->to_string() << "-" << occurrences_it->first.second->to_string() << endl;
		for(map<const DomainElement*,pair<int,int> >::const_iterator element_it=occurrences_it->second.begin(); element_it!=occurrences_it->second.end(); ++element_it){
			ss << "   " << element_it->first->to_string() << ": " << element_it->second.first << "," << element_it->second.second << endl;
		}
	}
	return ss.str();

}

/**********
 * Implementation of IVSet methods
 **********/

const set<const DomainElement*>& IVSet::getElements() const{
	return elements_;
}

const set<Sort*>& IVSet::getSorts() const{
	return sorts_;
}

const set<PFSymbol*>& IVSet::getRelations() const{
	return relations_;
}

IVSet::~IVSet(){};

IVSet::IVSet(const set<const DomainElement*> elements, const set<Sort*> sorts, const set<PFSymbol*> relations) 
	: elements_(elements), sorts_(sorts), relations_(relations){}

string IVSet::to_string() const{
	stringstream ss;
	for(set<Sort*>::iterator sorts_it = getSorts().begin(); sorts_it!=getSorts().end(); ++sorts_it){
		ss << (*sorts_it)->to_string() << " | ";
	}
	ss << endl;
	for(set<PFSymbol*>::const_iterator relations_it = getRelations().begin(); relations_it!=getRelations().end(); ++relations_it){
		ss << (*relations_it)->to_string() << " | ";
	}
	ss << endl;
	ss << getElements().size() << ": ";
	for(set<const DomainElement*>::const_iterator elements_it = getElements().begin(); elements_it!=getElements().end(); ++elements_it){
		ss << (*elements_it)->to_string() << " | ";
	}
	ss << endl;
	return ss.str();
}

bool IVSet::containsMultipleElements() const{
	return getElements().size()>1 && getRelations().size()>0;
}

bool IVSet::isDontCare(const AbstractStructure* s) const{
	bool result = true;
	set<PFSymbol*> relations = getRelations();
	for(set<PFSymbol*>::const_iterator relation_it = relations.begin(); relation_it!=relations.end() && result; ++relation_it){
		result=!hasInterpretation(s, *relation_it);
	}
	return result;
}

vector<const IVSet*> IVSet::splitBasedOnOccurrences(OccurrencesCounter* counter) const{
	map<vector<int>,set<const DomainElement*> > subSets;
	map<const DomainElement*, vector<int> > occurrences = counter->getOccurrences(getElements(), getRelations(), getSorts());
	for(map<const DomainElement*, vector<int> >::const_iterator occurrences_it=occurrences.begin(); occurrences_it!=occurrences.end(); ++occurrences_it){
		map<vector<int>,set<const DomainElement*> >::iterator subSets_it= subSets.find(occurrences_it->second);
		if(subSets_it!=subSets.end()){
			subSets_it->second.insert(occurrences_it->first);
		}else{
			set<const DomainElement*> temp;
			temp.insert(occurrences_it->first);
			subSets[occurrences_it->second]=temp;
		}
	}	
	vector<const IVSet*> result;
	for(map<vector<int>,set<const DomainElement*> >::const_iterator subSets_it=subSets.begin(); subSets_it!=subSets.end(); ++subSets_it){
		IVSet* ivset = new IVSet(subSets_it->second,getSorts(),getRelations());
		if(ivset->containsMultipleElements()){
			result.push_back(ivset);			
		}else{
			delete ivset;
		}
	}
	return result;
}

vector<const IVSet*> IVSet::splitBasedOnBinarySymmetries(const AbstractStructure* s) const{
	vector<const IVSet*> result;
	set<const DomainElement*> elements = getElements();
	set<const DomainElement*>::iterator elements_it=elements.begin();
	while(elements_it!=elements.end()){
		set<const DomainElement*> elementsIVSet;
		elementsIVSet.insert(*elements_it);
		set<const DomainElement*>::iterator elements_it2 = elements_it;
		++elements_it2;
		while(elements_it2!=elements.end()){
			bool isSymmetry = true;
			for(set<PFSymbol*>::const_iterator relations_it=getRelations().begin(); relations_it!=getRelations().end() && isSymmetry; ++relations_it){
				for(set<Sort*>::const_iterator sorts_it=getSorts().begin(); sorts_it!=getSorts().end() && isSymmetry; ++sorts_it){
					isSymmetry = isBinarySymmetry(s, *elements_it, *elements_it2, *relations_it, *sorts_it);
				}
			}
			set<const DomainElement*>::iterator erase_it2=elements_it2++;
			if(isSymmetry){
				elementsIVSet.insert(*erase_it2);
				elements.erase(erase_it2);
			}
		}
		IVSet* ivset = new IVSet(elementsIVSet, getSorts(), getRelations());
		if(ivset->containsMultipleElements()){
			result.push_back(ivset);
			cout << "S;dlfkjas;dlfkj" << ivset->to_string() << endl;
		}else{
			delete ivset;
		}		
		set<const DomainElement*>::iterator erase_it=elements_it++;
		elements.erase(erase_it);
	}
	return result;
}

/**********
 * Visitor analyzing theory for symmetry relevant information
 **********/

class TheorySymmetryAnalyzer : public TheoryVisitor {
	private:
		const AbstractStructure* 				structure_;
		set<const Sort*> 						forbiddenSorts_;
		map<const DomainElement*,const Sort*> 	forbiddenElements_;
		set<PFSymbol*>					usedRelations_;
		
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
		
		const set<const Sort*>& 						getForbiddenSorts() 	const { return forbiddenSorts_; }
		const map<const DomainElement*,const Sort*>& 	getForbiddenElements() 	const { return forbiddenElements_; }
		const set<PFSymbol*>& 					getUsedRelations()	 	const { return usedRelations_; }
		
		void addForbiddenSort(const Sort* sort) 			{ forbiddenSorts_.insert(sort); }
		void addUsedRelation(PFSymbol* relation) 	{ usedRelations_.insert(relation); }
};

void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const vector<Term*>& subTerms){
	for(unsigned int it=0; it<subTerms.size(); it++){
		markAsUnfitForSymmetry(subTerms.at(it)->sort());
	}
}

void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const Sort* s){
	addForbiddenSort(s);
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
	}else{
		addUsedRelation(f->symbol());
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
	}else{
		addUsedRelation(t->function());
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

/**********
 * Symmetry detection methods
 **********/

map<Sort*,set<const DomainElement*> > findElementsForSorts(const AbstractStructure* s, set<Sort*>& sorts, const map<const DomainElement*,const Sort*>& forbiddenElements){
	map<Sort*,set<const DomainElement*> > result; 
	for(set<Sort*>::const_iterator sort_it = sorts.begin(); sort_it != sorts.end(); ++sort_it){
		const SortTable* parent = s->inter(*sort_it);
		set<const DomainElement*> uniqueDomainElements;
		vector<const SortTable*> kids;
		for(set<Sort*>::const_iterator kids_it = (*sort_it)->children().begin(); kids_it != (*sort_it)->children().end(); ++kids_it){
			kids.push_back(s->inter(*kids_it));
		}
		for(SortIterator parent_it = parent->sortbegin(); parent_it.hasNext(); ++parent_it){
			map<const DomainElement*,const Sort*>::const_iterator forbiddenElement = forbiddenElements.find(*parent_it);
			bool isUnique = (forbiddenElement == forbiddenElements.end() || forbiddenElement->second != *sort_it);
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
	return result;
}

set<PFSymbol*> findNonTrivialRelationsWithSort(const AbstractStructure* s, const set<Sort*>& sorts, const set<PFSymbol*>& relations){
	set<PFSymbol*> result;
	for(set<PFSymbol*>::const_iterator relation_it=relations.begin(); relation_it!=relations.end(); ++relation_it){
		bool rangesOverSorts = hasTrivialInterpretation(s, *relation_it);
		for(vector<Sort*>::const_iterator sort_it=(*relation_it)->sorts().begin(); sort_it!=(*relation_it)->sorts().end() && !rangesOverSorts; ++sort_it){
			rangesOverSorts = sorts.count(*sort_it);
		}
		if(rangesOverSorts){
			result.insert(*relation_it);
		}
	}
	return result;
}

set<const IVSet*> initializeIVSets(const AbstractStructure* s, const AbstractTheory* t){
	TheorySymmetryAnalyzer tsa(s);
	t->accept(&tsa);	
	set<const Sort*> forbiddenSorts;
	for(set<const Sort*>::const_iterator sort_it = tsa.getForbiddenSorts().begin(); sort_it!=tsa.getForbiddenSorts().end(); ++sort_it){
		forbiddenSorts.insert(*sort_it);
		set<Sort*> descendents = (*sort_it)->descendents();
		for(set<Sort*>::const_iterator sort_it2 = descendents.begin(); sort_it2 != descendents.end(); ++sort_it2){
			forbiddenSorts.insert(*sort_it2);
		}
	}
	set<Sort*> allowedSorts;
	for(set<PFSymbol*>::const_iterator relation_it=tsa.getUsedRelations().begin(); relation_it!=tsa.getUsedRelations().end(); ++relation_it){
		for(vector<Sort*>::const_iterator sort_it=(*relation_it)->sorts().begin(); sort_it!=(*relation_it)->sorts().end(); ++sort_it){
			if(!forbiddenSorts.count(*sort_it) && isSortForSymmetry(*sort_it, s)){
				allowedSorts.insert(*sort_it);
				set<Sort*> ancestors = (*sort_it)->ancestors();
				for(set<Sort*>::const_iterator sort_it2 = ancestors.begin(); sort_it2 != ancestors.end(); ++sort_it2){
					if(isSortForSymmetry(*sort_it2, s)){
						allowedSorts.insert(*sort_it2);
					}
				}
			}
		}
	}
	map<Sort*,set<const DomainElement*> > elementsForSorts = findElementsForSorts(s, allowedSorts, tsa.getForbiddenElements());
	set<const IVSet*> result;
	for(map<Sort*,set<const DomainElement*> >::const_iterator ivset_it = elementsForSorts.begin(); ivset_it!=elementsForSorts.end(); ++ivset_it){
		set<Sort*> sorts = ivset_it->first->ancestors();
		sorts.insert(ivset_it->first);
		set<PFSymbol*> relations = findNonTrivialRelationsWithSort(s, sorts, tsa.getUsedRelations());
		IVSet* ivset = new IVSet(ivset_it->second, sorts, relations);
		if(ivset->containsMultipleElements()){
			result.insert(ivset);
		}else{
			delete ivset;
		}
	}
	return result;
}

vector<const IVSet*> extractDontCares(const AbstractStructure* s, set<const IVSet*>& potentials){
	vector<const IVSet*> result;
	set<const IVSet*>::iterator potentials_it=potentials.begin();
	while(potentials_it!=potentials.end()){
		set<const IVSet*>::iterator erase_it = potentials_it++;
		if((*erase_it)->isDontCare(s)){
			result.push_back(*erase_it);
			potentials.erase(erase_it);
		}
	}
	return result;
}

void splitByOccurrences(const AbstractStructure* s, set<const IVSet*>& potentials){
	OccurrencesCounter counter(s);
	vector<vector<const IVSet*> > splittedSets;
	set<const IVSet*>::iterator potentials_it=potentials.begin(); 
	while(potentials_it!=potentials.end()){
		splittedSets.push_back((*potentials_it)->splitBasedOnOccurrences(&counter));
		delete (*potentials_it);
		set<const IVSet*>::iterator erase_it=potentials_it++;
		potentials.erase(erase_it);
	}
	for(vector<vector<const IVSet*> >::const_iterator it1=splittedSets.begin(); it1!=splittedSets.end(); ++it1){
		for(vector<const IVSet*>::const_iterator it2=it1->begin(); it2!=it1->end(); ++it2){
			potentials.insert(*it2);
		}
	}
	cout << counter.to_string() << endl;
}

void splitByBinarySymmetries(const AbstractStructure* s, set<const IVSet*>& potentials){
	vector<vector<const IVSet*> > splittedSets;
	set<const IVSet*>::iterator potentials_it=potentials.begin(); 
	while(potentials_it!=potentials.end()){
		splittedSets.push_back((*potentials_it)->splitBasedOnBinarySymmetries(s));
		delete (*potentials_it);
		set<const IVSet*>::iterator erase_it=potentials_it++;
		potentials.erase(erase_it);
	}
	for(vector<vector<const IVSet*> >::const_iterator it1=splittedSets.begin(); it1!=splittedSets.end(); ++it1){
		for(vector<const IVSet*>::const_iterator it2=it1->begin(); it2!=it1->end(); ++it2){
			potentials.insert(*it2);
		}
	}
}

vector<const IVSet*> findIVSets(const AbstractTheory* t, const AbstractStructure* s){
	set<const IVSet*> potentials = initializeIVSets(s,t);
	vector<const IVSet*> result = extractDontCares(s, potentials); 
	splitByOccurrences(s, potentials);
	for(vector<const IVSet*>::const_iterator result_it=result.begin(); result_it!=result.end(); ++result_it){
		cout << "##########" << endl << (*result_it)->to_string() << endl;
	}
	for(set<const IVSet*>::const_iterator potentials_it=potentials.begin(); potentials_it!=potentials.end(); ++potentials_it){
		cout << "@@@@@@@@@@" << endl << (*potentials_it)->to_string() << endl;
	}
	splitByBinarySymmetries(s, potentials);
	for(vector<const IVSet*>::const_iterator result_it=result.begin(); result_it!=result.end(); ++result_it){
		cout << "##########" << endl << (*result_it)->to_string() << endl;
	}
	for(set<const IVSet*>::const_iterator potentials_it=potentials.begin(); potentials_it!=potentials.end(); ++potentials_it){
		cout << "@@@@@@@@@@" << endl << (*potentials_it)->to_string() << endl;
	}
	return result;
}
