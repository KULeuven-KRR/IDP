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
#include "ecnf.hpp"
#include "ground.hpp"
#include <list>
#include <sstream>
#include <iostream>  //TODO: wissen na debuggen :)

using namespace std;

//TODO: wat doet ancestors? zie nqueens :/

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

set<unsigned int> argumentNrs(const PFSymbol* relation, const set<Sort*>& sorts){
	set<unsigned int> result;
	for(set<Sort*>::const_iterator sorts_it=sorts.begin(); sorts_it!=sorts.end(); ++sorts_it){
		vector<unsigned int> argumentPlaces=relation->argumentNrs(*sorts_it);
		result.insert(argumentPlaces.begin(), argumentPlaces.end());
	}
	return result;
}

ElementTuple symmetricalTuple(const ElementTuple& original, const DomainElement* first, const DomainElement* second, const set<unsigned int>& argumentPlaces){
	ElementTuple symmetrical = original;
	for(set<unsigned int>::const_iterator argumentPlaces_it = argumentPlaces.begin(); argumentPlaces_it!=argumentPlaces.end(); ++argumentPlaces_it){
		if(symmetrical[*argumentPlaces_it]==first){
			symmetrical[*argumentPlaces_it]=second;
		}else if(symmetrical[*argumentPlaces_it]==second){
			symmetrical[*argumentPlaces_it]=first;
		}
	}
	return symmetrical;
}

bool isBinarySymmetryInPredTable(const PredTable* table, const set<unsigned int>& argumentPlaces, const DomainElement* first, const DomainElement* second){
	bool isSymmetry = true;
	for(TableIterator table_it = table->begin(); table_it.hasNext() && isSymmetry; ++table_it){
		ElementTuple symmetrical = symmetricalTuple(*table_it, first, second, argumentPlaces);
		if(symmetrical!=*table_it){
			isSymmetry=table->contains(symmetrical);
		}
	}
	return isSymmetry;
}

bool isBinarySymmetry(const AbstractStructure* s, const DomainElement* first, const DomainElement* second, PFSymbol* relation, const set<Sort*>& sorts){
	if(!hasInterpretation(s,relation)){
		return true;
	}
	set<unsigned int> argumentPlaces = argumentNrs(relation, sorts);
	if(argumentPlaces.size()==0){
		return true;
	}
	const PredInter* inter = s->inter(relation);
	const PredTable* table = inter->ct();
	bool result = isBinarySymmetryInPredTable(table, argumentPlaces, first, second);
	if(!inter->approxtwovalued() && result==true){
		table = inter->cf();
		result = isBinarySymmetryInPredTable(table, argumentPlaces, first, second);
	}
	return result;
}

void addSymBreakingClausesToGroundTheory(AbstractGroundTheory* gt, const list<int>& literals, const list<int>& symLiterals){
	//the first two steps are initializers, initializing the first constraints and auxiliary variables
	list<int>::const_iterator literals_it = literals.begin();
	list<int>::const_iterator symLiterals_it = symLiterals.begin();
	int currentAuxVar;
	int previousAuxVar;
	if(literals.size()>0){
		// (~V1 | V1*)
		vector<int> firstClause (2);
		firstClause[0]= -(*literals_it); 
		firstClause[1]=  (*symLiterals_it);
		gt->addClause(firstClause, false);
	}
	if(literals.size()>1){
		currentAuxVar = gt->getFreeTseitin();
		// (~A2 | V1 | ~V1*)
		vector<int> clause2 (3);
		clause2[0]= -currentAuxVar;
		clause2[1]=  (*literals_it);
		clause2[2]= -(*symLiterals_it);
		gt->addClause(clause2, true);
		// (A2 | ~V1) 
		vector<int> clause3 (2);
		clause3[0]=  currentAuxVar;
		clause3[1]= -(*literals_it);
		gt->addClause(clause3, true);
		// (A2 | V1*)
		vector<int> clause4 (2);
		clause4[0]=  currentAuxVar;
		clause4[1]=  (*symLiterals_it);
		gt->addClause(clause4, true);
		// (~A2 | ~V2 | V2*)
		++literals_it;
		++symLiterals_it;
		vector<int> clause5 (3);
		clause5[0]= -currentAuxVar;
		clause5[1]= -(*literals_it);
		clause5[2]=  (*symLiterals_it);
		gt->addClause(clause5, true);
	}
	list<int>::const_iterator oneButLast_it = literals.end();
	--oneButLast_it;
	while(literals_it != oneButLast_it){
		previousAuxVar = currentAuxVar;
		currentAuxVar = gt->getFreeTseitin();
		// ( ~A_n | A_n-1 )
		vector<int> clause1 (2);
		clause1[0]= -currentAuxVar;
		clause1[1]=  previousAuxVar;
		gt->addClause(clause1, true);
		// ( ~An | ~A_n-1 | V_n-1 | ~V_n-1* )
		vector<int> clause4 (4);
		clause4[0]= -currentAuxVar; 
		clause4[1]= -previousAuxVar;
		clause4[2]=  (*literals_it);
		clause4[3]= -(*symLiterals_it);
		gt->addClause(clause4, true);
		// ( A_n | ~A_n-1 | ~V_n-1)
		vector<int> clause2 (3);
		clause2[0]=  currentAuxVar;
		clause2[1]= -previousAuxVar; 
		clause2[2]= -(*literals_it);
		gt->addClause(clause2, true);
		// ( A_n | ~A_n-1 | V_n-1*)
		vector<int> clause3 (3);
		clause3[0]=  currentAuxVar;
		clause3[1]= -previousAuxVar;
		clause3[2]=  (*symLiterals_it);
		gt->addClause(clause3, true);
		// ( ~An | ~Vn | Vn* )
		++literals_it;
		++symLiterals_it;
		vector<int> clause6 (3);
		clause6[0]= -currentAuxVar;
		clause6[1]= -(*literals_it);
		clause6[2]=  (*symLiterals_it);
		gt->addClause(clause6, true);
	}
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
		const AbstractStructure* 				getStructure() const;
		
		pair<int,int>							getOccurrences(const DomainElement*, PFSymbol*, Sort* );
		map<const DomainElement*, vector<int> > getOccurrences(const set<const DomainElement*>&, const set<PFSymbol*>&, const set<Sort*>& );
		
		string									to_string() const;
};

const AbstractStructure* OccurrencesCounter::getStructure() const {
	return structure_;
}

// @pre: !relation->argumentNrs(sort).empty()
map<const DomainElement*,pair<int,int> > OccurrencesCounter::count(PFSymbol* relation, Sort* sort){
	assert(!relation->argumentNrs(sort).empty());
	vector<unsigned int> arguments = relation->argumentNrs(sort);
	map<const DomainElement*,pair<int,int> > result;
	const PredTable* ct = getStructure()->inter(relation)->ct();
	for(TableIterator ct_it = ct->begin(); ct_it.hasNext(); ++ct_it){
		for(vector<unsigned int>::const_iterator it=arguments.begin(); it!=arguments.end(); ++it){
			const DomainElement* element = (*ct_it)[*it];
			map<const DomainElement*,pair<int,int> >::iterator result_it = result.find(element);
			if(result_it==result.end()){
				result[element]=pair<int,int>(1,0);
			}else{
				++(result_it->second.first);
			}
		}
	}
	const PredTable* cf = getStructure()->inter(relation)->cf();
	for(TableIterator cf_it = cf->begin(); cf_it.hasNext(); ++cf_it){
		for(vector<unsigned int>::const_iterator it=arguments.begin(); it!=arguments.end(); ++it){
			const DomainElement* element = (*cf_it)[*it];
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
			if(hasInterpretation(getStructure(),*relations_it)){
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
	ss << "structure: " << getStructure()->name() << endl;
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

const AbstractStructure* IVSet::getStructure() const{
	return structure_;
}

IVSet::~IVSet(){};

IVSet::IVSet(const AbstractStructure* s, const set<const DomainElement*> elements, const set<Sort*> sorts, const set<PFSymbol*> relations) 
	: structure_(s), elements_(elements), sorts_(sorts), relations_(relations){}

string IVSet::to_string() const{
	stringstream ss;
	ss << "structure: " << getStructure()->name() << endl;
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
	ss << "Enkelvoudig? " << isEnkelvoudig() << endl;
	return ss.str();
}

bool IVSet::hasRelevantRelationsAndSorts() const{
	if(getSorts().size()==0 || getRelations().size()==0){
		return false;
	}
	bool hasOnlyInterpretedRelations = true;
	for(set<PFSymbol*>::const_iterator relations_it=getRelations().begin(); relations_it!=getRelations().end() && hasOnlyInterpretedRelations; ++relations_it){
		hasOnlyInterpretedRelations = hasInterpretation(getStructure(), *relations_it);
	}
	if(hasOnlyInterpretedRelations){
		return false;
	}
	bool everyRelationHasASort = true;
	for(set<PFSymbol*>::const_iterator relations_it=getRelations().begin(); relations_it!=getRelations().end() && everyRelationHasASort; ++relations_it){
		everyRelationHasASort = argumentNrs(*relations_it, getSorts()).size()>0;
	}
	if(!everyRelationHasASort){
		return false;
	}
	
	return true;
}

bool IVSet::containsMultipleElements() const{
	return getElements().size()>1;
}

bool IVSet::isRelevantSymmetry() const{
	return containsMultipleElements() && hasRelevantRelationsAndSorts();
}

bool IVSet::isDontCare() const{
	bool result = true;
	set<PFSymbol*> relations = getRelations();
	for(set<PFSymbol*>::const_iterator relations_it = relations.begin(); relations_it!=relations.end() && result; ++relations_it){
		result=!hasInterpretation(getStructure(), *relations_it);
	}
	return result;
}

bool IVSet::isEnkelvoudig() const{
	bool result = true;
	for(set<PFSymbol*>::const_iterator relations_it=getRelations().begin(); relations_it!=getRelations().end() && result==true; ++relations_it){
		if(!hasInterpretation(getStructure(), *relations_it)){
			result=argumentNrs(*relations_it, getSorts()).size()==1;
		}
	}
	return result;
}

vector<const IVSet*> IVSet::splitBasedOnOccurrences(OccurrencesCounter* counter) const{
	assert(counter->getStructure()==this->getStructure());
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
		IVSet* ivset = new IVSet(getStructure(),subSets_it->second,getSorts(),getRelations());
		if(ivset->isRelevantSymmetry()){
			result.push_back(ivset);			
		}else{
			delete ivset;
		}
	}
	return result;
}

vector<const IVSet*> IVSet::splitBasedOnBinarySymmetries() const{
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
					isSymmetry = isBinarySymmetry(getStructure(), *elements_it, *elements_it2, *relations_it, getSorts());
				}
			}
			set<const DomainElement*>::iterator erase_it2=elements_it2++;
			if(isSymmetry){
				elementsIVSet.insert(*erase_it2);
				elements.erase(erase_it2);
			}
		}
		IVSet* ivset = new IVSet(getStructure(), elementsIVSet, getSorts(), getRelations());
		if(ivset->isRelevantSymmetry()){
			result.push_back(ivset);
		}else{
			delete ivset;
		}		
		set<const DomainElement*>::iterator erase_it=elements_it++;
		elements.erase(erase_it);
	}
	return result;
}

// should be an inner method in IVSet::getSymmetricLiterals()
vector<vector<const DomainElement*> > fillGroundElementsOneRank(vector<vector<const DomainElement*> >& groundElements, const SortTable* domainTable, const int rank, const set<const DomainElement*>& excludedElements){
	set<const DomainElement*> domain; //set to order the elements
	for(SortIterator domain_it=domainTable->sortbegin(); domain_it.hasNext(); ++domain_it){
		if(!excludedElements.count(*domain_it)){
			domain.insert(*domain_it);
		}
	}
	vector<vector<const DomainElement*> > newGroundElements(groundElements.size()*domain.size());
	for(unsigned int ge=0; ge<groundElements.size(); ++ge){
		int index=0;
		for(set<const DomainElement*>::const_iterator domain_it=domain.begin(); domain_it!=domain.end(); ++domain_it){
			newGroundElements[ge*domain.size()+index]=groundElements[ge];
			newGroundElements[ge*domain.size()+index][rank]=(*domain_it);
			++index;
		}
	}
	return newGroundElements;
}

// Order is based on the pointers, not on the order given by for instance a SortIterator!
pair<list<int>,list<int> > IVSet::getSymmetricLiterals(AbstractGroundTheory* gt, const DomainElement* smaller, const DomainElement* bigger) const {
	set<const DomainElement*> excludedSet; excludedSet.insert(smaller); excludedSet.insert(bigger);
	const set<const DomainElement*> emptySet;
	
	list<int> originals;
	list<int> symmetricals;
	for(set<PFSymbol*>::const_iterator relations_it=getRelations().begin(); relations_it!=getRelations().end(); ++relations_it){
		if(!hasInterpretation(getStructure(), *relations_it)){
			set<unsigned int> argumentPlaces=argumentNrs(*relations_it,getSorts());
			for(set<unsigned int>::const_iterator arguments_it=argumentPlaces.begin(); arguments_it!=argumentPlaces.end(); ++arguments_it){
				vector<vector<const DomainElement*> > groundElements(1);
				groundElements[0] = vector<const DomainElement*>((*relations_it)->nrSorts());
				for(unsigned int argument = 0; argument<*arguments_it; ++argument){
					Sort* currSort = (*relations_it)->sort(argument);
					if(getSorts().count(currSort) ){
						groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, excludedSet);
					}else{
						groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);
					}
				}
				for(unsigned int it=0; it<groundElements.size(); it++){
					groundElements[it][*arguments_it]=smaller;
				}
				for(unsigned int argument = (*arguments_it)+1; argument<(*relations_it)->nrSorts(); ++argument){
					Sort* currSort = (*relations_it)->sort(argument);
					groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);			
				}
				for(vector<vector<const DomainElement*> >::const_iterator ge_it=groundElements.begin(); ge_it!=groundElements.end(); ++ge_it){
					ElementTuple original = *ge_it;
					ElementTuple symmetrical = symmetricalTuple(original, smaller, bigger, argumentPlaces);
					originals.push_back(gt->translator()->translate(*relations_it, original));
					symmetricals.push_back(gt->translator()->translate(*relations_it, symmetrical));
				}
			}
		}
	}
	return pair<list<int>,list<int> >(originals,symmetricals);
}

void IVSet::addSymBreakingPreds(AbstractGroundTheory* gt) const{
	set<const DomainElement*>::const_iterator smaller = getElements().begin();
	set<const DomainElement*>::const_iterator bigger = getElements().begin(); ++bigger;
	for(; bigger!=getElements().end(); ++bigger, ++smaller){
		pair<list<int>,list<int> > literals = getSymmetricLiterals(gt, *smaller, *bigger);
		addSymBreakingClausesToGroundTheory(gt, literals.first, literals.second);
	}
}

vector<map<int,int> > IVSet::getLiteralsSymmetries(AbstractGroundTheory* gt) const{
	vector<map<int,int> > literalSymmetries;
	set<const DomainElement*>::const_iterator smaller = getElements().begin();
	set<const DomainElement*>::const_iterator bigger = getElements().begin(); ++bigger;
	for(; bigger!=getElements().end(); ++bigger, ++smaller){
		map<int,int> literalSymmetry;
		pair<list<int>,list<int> > symmetricLiterals = getSymmetricLiterals(gt, *smaller, *bigger);
		list<int>::const_iterator first_it=symmetricLiterals.first.begin();
		for(list<int>::const_iterator second_it=symmetricLiterals.second.begin(); second_it!=symmetricLiterals.second.end(); ++first_it, ++second_it){
			literalSymmetry[*first_it]=*second_it;
			literalSymmetry[*second_it]=*first_it;
		}
		literalSymmetries.push_back(literalSymmetry);
	}
	return literalSymmetries;
}

// pre: this->isEnkelvoudig()
vector<list<int> > IVSet::getInterchangeableLiterals(AbstractGroundTheory* gt) const{
	assert(this->isEnkelvoudig());
	
	const set<const DomainElement*> emptySet;
	vector<list<int> > result;
	const DomainElement* smallest = *(getElements().begin());
	for(set<PFSymbol*>::const_iterator relations_it=getRelations().begin(); relations_it!=getRelations().end(); ++relations_it){
		if(!hasInterpretation(getStructure(), *relations_it)){
			set<unsigned int> argumentPlaces = argumentNrs(*relations_it,getSorts());
			unsigned int argumentPlace = *(argumentPlaces.begin());
			vector<vector<const DomainElement*> > groundElements(1);
			groundElements[0] = vector<const DomainElement*>((*relations_it)->nrSorts());
			for(unsigned int argument = 0; argument<argumentPlace; ++argument){
				Sort* currSort = (*relations_it)->sort(argument);
				groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);
			}
			for(unsigned int it=0; it<groundElements.size(); it++){
				groundElements[it][argumentPlace]=smallest;
			}
			for(unsigned int argument = argumentPlace+1; argument<(*relations_it)->nrSorts(); ++argument){
				Sort* currSort = (*relations_it)->sort(argument);
				groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);			
			}
			for(set<const DomainElement*>::const_iterator elements_it= getElements().begin(); elements_it!=getElements().end(); ++elements_it){
				list<int> literals;
				for(vector<vector<const DomainElement*> >::const_iterator ge_it=groundElements.begin(); ge_it!=groundElements.end(); ++ge_it){
					ElementTuple original = *ge_it;
					if(*elements_it==smallest){
						literals.push_back(gt->translator()->translate(*relations_it, original));
					}else{
						ElementTuple symmetrical = symmetricalTuple(original, smallest, *elements_it, argumentPlaces);
						literals.push_back(gt->translator()->translate(*relations_it, symmetrical));
					}
				}
				result.push_back(literals);
			}
		}
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

// back-up code. If after a year, this has not been used, delete it. 15/07/2011
/*map<Sort*,set<const DomainElement*> > findElementsForSorts(const AbstractStructure* s, set<Sort*>& sorts, const map<const DomainElement*,const Sort*>& forbiddenElements){
	map<Sort*,set<const DomainElement*> > result; 
	for(set<Sort*>::const_iterator sorts_it = sorts.begin(); sorts_it != sorts.end(); ++sorts_it){
		const SortTable* domain = s->inter(*sorts_it);
		set<const DomainElement*> allowedDomainElements;
		for(SortIterator domain_it = domain->sortbegin(); domain_it.hasNext(); ++domain_it){
			if(!forbiddenElements.count(*domain_it)){
				allowedDomainElements.insert(*domain_it);
			}
		}
		if(allowedDomainElements.size()>1){
			result[*sorts_it]=allowedDomainElements;
		}
	}
	return result;
}*/


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

// pre: t->vocabulary()==s->vocabulary()
set<const IVSet*> initializeIVSets(const AbstractStructure* s, const AbstractTheory* t){
	assert(t->vocabulary()==s->vocabulary());
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
			}
		}
	}
	map<Sort*,set<const DomainElement*> > elementsForSorts = findElementsForSorts(s, allowedSorts, tsa.getForbiddenElements());
	set<const IVSet*> result;
	for(map<Sort*,set<const DomainElement*> >::const_iterator ivset_it = elementsForSorts.begin(); ivset_it!=elementsForSorts.end(); ++ivset_it){
		set<Sort*> sorts;
		sorts.insert(ivset_it->first);
		set<Sort*> ancestors = ivset_it->first->ancestors(t->vocabulary());
		for(set<Sort*>::const_iterator ancestors_it=ancestors.begin(); ancestors_it!=ancestors.end(); ++ancestors_it){
			if(allowedSorts.count(*ancestors_it)){
				sorts.insert(*ancestors_it);
			}
		}
		set<PFSymbol*> relations = findNonTrivialRelationsWithSort(s, sorts, tsa.getUsedRelations());
		IVSet* ivset = new IVSet(s, ivset_it->second, sorts, relations);
		if(ivset->isRelevantSymmetry()){
			result.insert(ivset);
		}else{
			delete ivset;
		}
	}
	return result;
}

vector<const IVSet*> extractDontCares(set<const IVSet*>& potentials){
	vector<const IVSet*> result;
	set<const IVSet*>::iterator potentials_it=potentials.begin();
	while(potentials_it!=potentials.end()){
		set<const IVSet*>::iterator erase_it = potentials_it++;
		if((*erase_it)->isDontCare()){
			result.push_back(*erase_it);
			potentials.erase(erase_it);
		}
	}
	return result;
}

void splitByOccurrences(set<const IVSet*>& potentials){
	if(!potentials.empty()){
		OccurrencesCounter counter((*potentials.begin())->getStructure());
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
}

void splitByBinarySymmetries(set<const IVSet*>& potentials){
	vector<vector<const IVSet*> > splittedSets;
	set<const IVSet*>::iterator potentials_it=potentials.begin(); 
	while(potentials_it!=potentials.end()){
		splittedSets.push_back((*potentials_it)->splitBasedOnBinarySymmetries());
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


// pre: t->vocabulary()==s->vocabulary()
vector<const IVSet*> findIVSets(const AbstractTheory* t, const AbstractStructure* s){
	assert(t->vocabulary()==s->vocabulary());
	
	cout << "initialize ivsets..." << endl;
	
	set<const IVSet*> potentials = initializeIVSets(s,t);
	
	cout << "extract dont cares..." << endl;
	
	vector<const IVSet*> result = extractDontCares(potentials);
	for(vector<const IVSet*>::const_iterator result_it=result.begin(); result_it!=result.end(); ++result_it){
		cout << "##########" << endl << (*result_it)->to_string() << endl;
	}
	splitByOccurrences(potentials);
	splitByBinarySymmetries(potentials);
	for(set<const IVSet*>::const_iterator result_it=potentials.begin(); result_it!=potentials.end(); ++result_it){
		cout << "@@@@@@@@@@" << endl << (*result_it)->to_string() << endl;
	}
	result.insert(result.end(),potentials.begin(),potentials.end());
	return result;
}


// warning: the ivsets will be deleted afterwards!
void addSymBreakingPredicates(AbstractGroundTheory* gt, vector<const IVSet*> ivsets){
	for(vector<const IVSet*>::const_iterator ivsets_it=ivsets.begin(); ivsets_it!=ivsets.end(); ++ivsets_it){
		(*ivsets_it)->addSymBreakingPreds(gt);
		delete (*ivsets_it);
	}
}


