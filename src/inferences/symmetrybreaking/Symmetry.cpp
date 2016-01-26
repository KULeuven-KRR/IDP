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
#include <list>
#include <bitset>
#include "utils/ListUtils.hpp"
#include "inferences/modelexpansion/DefinitionPostProcessing.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

/**********
 * Miscellaneous methods
 **********/

/**
 * 	returns true if a sort has a domain fit to detect meaningful invariant permutations.
 */
bool isSortForSymmetry(Sort* sort, const Structure* s) {
	return s->inter(sort)->approxFinite() && not s->inter(sort)->approxEmpty();
}

/**
 *	returns true if an interpretation is trivial, which means either all ground elements are true or all are false for the given PFSymbol
 */
bool hasTrivialInterpretation(const Structure* s, PFSymbol* relation) {
	return s->inter(relation)->pt()->approxEmpty() || s->inter(relation)->pf()->approxEmpty();
}

/**
 *	returns true if a PFSymbol has an interpretation
 */
bool hasInterpretation(const Structure* s, PFSymbol* relation) {
	return not (s->inter(relation)->ct()->approxEmpty() && s->inter(relation)->cf()->approxEmpty());
}

/**
 *	returns a set of int's where each int represents the nth argument of the PFSymbol, such that the sort of the nth argument is in the set of sorts.
 */
set<unsigned int> argumentNrs(PFSymbol* relation, const set<Sort*>& sorts) {
	set<unsigned int> result;
	for (auto sorts_it = sorts.cbegin(); sorts_it != sorts.cend(); ++sorts_it) {
		vector<unsigned int> argumentPlaces = relation->argumentNrs(*sorts_it);
		result.insert(argumentPlaces.cbegin(), argumentPlaces.cend());
	}
	return result;
}

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

/**
 *	returns true iff a binary permutation of domain elements is an invariant permutation in an interpretation of a PFSymbol, given the arguments of the PFSymbol to check
 */
bool isBinarySymmetryInPredTable(const PredTable* table, const set<unsigned int>& argumentPlaces, const DomainElement* first, const DomainElement* second) {
	bool isSymmetry = true;
	for (TableIterator table_it = table->begin(); not table_it.isAtEnd() && isSymmetry; ++table_it) {
		ElementTuple symmetrical = symmetricalTuple(*table_it, first, second, argumentPlaces);
		if (symmetrical != *table_it) {
			isSymmetry = table->contains(symmetrical);
		}
	}
	return isSymmetry;
}

/**
 *	returns true iff a binary permutation of domain elements of a set of sorts is an invariant permutation for a certain PFSymbol
 */
bool isBinarySymmetry(const Structure* s, const DomainElement* first, const DomainElement* second, PFSymbol* relation, const set<Sort*>& sorts) {
	if (not hasInterpretation(s, relation)) {
		return true;
	}
	set<unsigned int> argumentPlaces = argumentNrs(relation, sorts);
	if (argumentPlaces.size() == 0) {
		return true;
	}
	const PredInter* inter = s->inter(relation);
	const PredTable* table = inter->ct();
	bool result = isBinarySymmetryInPredTable(table, argumentPlaces, first, second);
	if (not inter->approxTwoValued() && result == true) {
		table = inter->cf();
		result = isBinarySymmetryInPredTable(table, argumentPlaces, first, second);
	}
	return result;
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
// TODO split up => Jo: how?

void addSymBreakingClausesToGroundTheory(AbstractGroundTheory* gt, const list<int>& literals, const list<int>& symLiterals) {
	list<int>::const_iterator literals_it = literals.cbegin();
	list<int>::const_iterator symLiterals_it = symLiterals.cbegin();
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
	list<int>::const_iterator oneButLast_it = literals.cend();
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
void addSymBreakingClausesToGroundTheoryShortest(AbstractGroundTheory* gt, const list<int>& literals, const list<int>& symLiterals) {
	list<int>::const_iterator literals_it = literals.cbegin();
	list<int>::const_iterator symLiterals_it = symLiterals.cbegin();
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
	list<int>::const_iterator oneButLast_it = literals.cend();
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

/**********
 * Object to count occurrences using memoization
 **********/

/**
 * This class counts the occurrences of a domain element in all ground elements mapped to true and in all ground elements mapped to false.
 * A necessary condition for two domain elements to form an invariant permutation is that their occurrences are equal.
 *
 * This class uses memoization / lazy counting: it will only start counting certain elements when triggered by a request, but afterwards it will store the results in memory to be of use at a later time.
 */

class OccurrencesCounter {
private:
	// Attributes
	const Structure* structure_;
	map<pair<PFSymbol*, const Sort*>, map<const DomainElement*, pair<int, int> > > occurrences_; //<! a pair of ints for each domain element representing its occurrences in ct and cf for each relation-sort combination

	// Static (should be static, is neither mutator nor inspector, counts stuff somewhere else)
	map<const DomainElement*, pair<int, int> > count(PFSymbol*, Sort*);

public:
	// Constructors

	OccurrencesCounter(const Structure* s)
	: structure_(s) {
	}

	// Inspectors
	const Structure* getStructure() const;
	ostream& put(ostream& output) const;

	//	Mutators
	pair<int, int> getOccurrences(const DomainElement*, PFSymbol*, Sort*);
	map<const DomainElement*, vector<int> > getOccurrences(const set<const DomainElement*>&, const set<PFSymbol*>&, const set<Sort*>&);
};

const Structure* OccurrencesCounter::getStructure() const {
	return structure_;
}

/**
 *	Given a PFSymbol and a sort, this method counts for all domain elements in the domain of the sort the total amount of occurrences in its interpretation, for both the ct and cf parts of the interpretations.
 *
 *	@pre: !relation->argumentNrs(sort).empty()
 */
map<const DomainElement*, pair<int, int> > OccurrencesCounter::count(PFSymbol* relation, Sort* sort) {
	Assert(!relation->argumentNrs(sort).empty());
	vector<unsigned int> arguments = relation->argumentNrs(sort);
	map<const DomainElement*, pair<int, int> > result;
	const PredTable* ct = getStructure()->inter(relation)->ct();
	for (TableIterator ct_it = ct->begin(); not ct_it.isAtEnd(); ++ct_it) {
		for (auto it = arguments.cbegin(); it != arguments.cend(); ++it) {
			const DomainElement* element = (*ct_it)[*it];
			map<const DomainElement*, pair<int, int> >::iterator result_it = result.find(element);
			if (result_it == result.cend()) {
				result[element] = pair<int, int>(1, 0);
			} else {
				++(result_it->second.first);
			}
		}
	}
	const PredTable* cf = getStructure()->inter(relation)->cf();
	for (TableIterator cf_it = cf->begin(); not cf_it.isAtEnd(); ++cf_it) {
		for (auto it = arguments.cbegin(); it != arguments.cend(); ++it) {
			const DomainElement* element = (*cf_it)[*it];
			map<const DomainElement*, pair<int, int> >::iterator result_it = result.find(element);
			if (result_it == result.cend()) {
				result[element] = pair<int, int>(0, 1);
			} else {
				++(result_it->second.second);
			}
		}
	}
	occurrences_[pair<PFSymbol*, const Sort*>(relation, sort)] = result;
	return result;
}

/**
 * 	Requests the occurrences of a certain domain element of a certain sort for a certain PFSymbol for both ct and cf tables.
 * 	If this value has already been calculated, it can be retrieved from memory or it is (0,0). Otherwise, count it.
 *
 *	@pre: !relation->argumentNrs(sort).empty()
 */
pair<int, int> OccurrencesCounter::getOccurrences(const DomainElement* element, PFSymbol* relation, Sort* sort) {
	Assert(!relation->argumentNrs(sort).empty());
	auto occurrences_it = occurrences_.find(pair<PFSymbol*, const Sort*>{relation, sort});
	if (occurrences_it != occurrences_.cend()) {
		auto result_it = occurrences_it->second.find(element);
		if (result_it != occurrences_it->second.cend()) {
			return result_it->second;
		} else {
			return pair<int, int>(0, 0);
		}
	} else {
		map<const DomainElement*, pair<int, int> > counts = count(relation, sort);
		map<const DomainElement*, pair<int, int> >::const_iterator counts_it = counts.find(element);
		if (counts_it != counts.cend()) {
			return counts_it->second;
		} else {
			return pair<int, int>(0, 0);
		}
	}
}

/**
 *	Requests the occurrences of some domain elements of certain sorts for certain PFSymbols for both ct and cf tables.
 */
map<const DomainElement*, vector<int> > OccurrencesCounter::getOccurrences(const set<const DomainElement*>& elements, const set<PFSymbol*>& relations,
		const set<Sort*>& sorts) {
	map<const DomainElement*, vector<int> > result;
	for (auto elements_it = elements.cbegin(); elements_it != elements.cend(); ++elements_it) {
		vector<int> values;
		for (auto relations_it = relations.cbegin(); relations_it != relations.cend(); ++relations_it) {
			if (hasInterpretation(getStructure(), *relations_it)) {
				for (auto sorts_it = sorts.cbegin(); sorts_it != sorts.cend(); ++sorts_it) {
					if (!(*relations_it)->argumentNrs(*sorts_it).empty()) {
						pair<int, int> temp = getOccurrences(*elements_it, *relations_it, *sorts_it);
						values.push_back(temp.first);
						values.push_back(temp.second);
					}
				}
			}
		}
		result[*elements_it] = values;
	}
	return result;
}

ostream& OccurrencesCounter::put(ostream& output) const {
	output << "COUNTER:\n";
	output << "structure: " << getStructure()->name() << "\n";
	for (auto occurrences_it = occurrences_.cbegin(); occurrences_it != occurrences_.cend(); ++occurrences_it) {
		output << print(occurrences_it->first.first) << "-" << print(occurrences_it->first.second) << "\n";
		for (auto element_it = occurrences_it->second.cbegin(); element_it != occurrences_it->second.cend(); ++element_it) {
			output << "   " << print(element_it->first) << ": " << element_it->second.first << "," << element_it->second.second << "\n";
		}
	}
	return output;
}

/**********
 * Implementation of IVSet methods
 **********/

const set<const DomainElement*>& IVSet::getElements() const {
	return elements_;
}

const set<Sort*>& IVSet::getSorts() const {
	return sorts_;
}

const set<PFSymbol*>& IVSet::getRelations() const {
	return relations_;
}

const Structure* IVSet::getStructure() const {
	return structure_;
}

/**
 * 	The constructor of IVSet.
 * 	Since an IVSet is immutable, it is sufficient to check its invariants in the constructor to enforce those invariants.
 * 	The second invariant however, is not checked, and thus not enforced :(
 */
IVSet::IVSet(const Structure* s, const set<const DomainElement*> elements, const set<Sort*> sorts, const set<PFSymbol*> relations)
: structure_(s), elements_(elements), sorts_(sorts), relations_(relations) {
	Assert(elements_.size() > 1);
}

ostream& IVSet::put(ostream& output) const {
	output << "Set of interchangeable domain elements (" << getElements().size() << " in size): \n";
	for (auto elements_it = getElements().cbegin(); elements_it != getElements().cend(); ++elements_it) {
		output << print(*elements_it) << " | ";
	}
	output << "\n";
	output << "Corresponding sorts:\n";
	for (auto sorts_it = getSorts().cbegin(); sorts_it != getSorts().cend(); ++sorts_it) {
		output << print(*sorts_it) << " | ";
	}
	output << "\n";
	if (isEnkelvoudig()) {
		output << "The resulting symmetry group is broken completely.\n";
	} else {
		output << "The resulting symmetry group is probably not broken completely.\n";
	}
	return output;
}

/**
 * 	returns true if it is useful to take this invariant set into account, considering certain properties of relations and sorts.
 */
bool IVSet::hasRelevantRelationsAndSorts() const {
	if (getSorts().size() == 0 || getRelations().size() == 0) {
		return false;
	}
	bool hasOnlyInterpretedRelations = true;
	for (auto relations_it = getRelations().cbegin(); relations_it != getRelations().cend() && hasOnlyInterpretedRelations; ++relations_it) {
		hasOnlyInterpretedRelations = hasInterpretation(getStructure(), *relations_it);
	}
	if (hasOnlyInterpretedRelations) {
		return false;
	}
	bool everyRelationHasASort = true;
	for (auto relations_it = getRelations().cbegin(); relations_it != getRelations().cend() && everyRelationHasASort; ++relations_it) {
		everyRelationHasASort = argumentNrs(*relations_it, getSorts()).size() > 0;
	}
	if (!everyRelationHasASort) {
		return false;
	}

	return true;
}

bool IVSet::containsMultipleElements() const {
	return getElements().size() > 1;
}

bool IVSet::isRelevantSymmetry() const {
	return containsMultipleElements() && hasRelevantRelationsAndSorts();
}

/**
 * 	returns whether an IVSet represents a don't care. If all relation symbols for this IVSet don't have an interpretation, this IVSet is a don't care and thus a trivial invariant set.
 */
bool IVSet::isDontCare() const {
	bool result = true;
	set<PFSymbol*> relations = getRelations();
	for (auto relations_it = relations.cbegin(); relations_it != relations.cend() && result; ++relations_it) {
		result = !hasInterpretation(getStructure(), *relations_it);
	}
	return result;
}

/**
 * 	Returns true if an IVSet is enkelvoudig (Eng: singular). An IVSet is enkelvoudig if each of the PFSymbols of the IVSet range over at most one sort of the IVSet.
 * 	Since the PFSymbols of an IVSet should range over at least one sort of the IVSet, this method checks whether each PFSymbol ranges over exactly one  of the sorts.
 */
bool IVSet::isEnkelvoudig() const {
	bool result = true;
	for (auto relations_it = getRelations().cbegin(); relations_it != getRelations().cend() && result == true; ++relations_it) {
		if (!hasInterpretation(getStructure(), *relations_it)) {
			result = argumentNrs(*relations_it, getSorts()).size() == 1;
		}
	}
	return result;
}

/**
 *	Return a partition of a potential IVSet based on the occurrences of its domain elements.
 *	The given counter keeps track of already counted domain elements, so no double work is done.
 */
vector<const IVSet*> IVSet::splitBasedOnOccurrences(OccurrencesCounter* counter) const {
	Assert(counter->getStructure() == this->getStructure());
	map<vector<int>, set<const DomainElement*> > subSets;
	auto occurrences = counter->getOccurrences(getElements(), getRelations(), getSorts());
	for (auto occurrences_it = occurrences.cbegin(); occurrences_it != occurrences.cend(); ++occurrences_it) {
		auto subSets_it = subSets.find(occurrences_it->second);
		if (subSets_it != subSets.cend()) {
			subSets_it->second.insert(occurrences_it->first);
		} else {
			set<const DomainElement*> temp;
			temp.insert(occurrences_it->first);
			subSets[occurrences_it->second] = temp;
		}
	}
	vector<const IVSet*> result;
	for (auto subSets_it = subSets.cbegin(); subSets_it != subSets.cend(); ++subSets_it) {
		if (subSets_it->second.size() > 1) {
			IVSet* ivset = new IVSet(getStructure(), subSets_it->second, getSorts(), getRelations());
			if (ivset->isRelevantSymmetry()) {
				result.push_back(ivset);
			} else {
				delete ivset;
			}
		}
	}
	return result;
}

/**
 *	Return a partition of a potential IVSet such that every partition certainly is an IVSet.
 *	This partition is calculated by checking if the necessary binary permutations are invariant permutations (which induce a binary symmetry).
 */
vector<const IVSet*> IVSet::splitBasedOnBinarySymmetries() const {
	vector<const IVSet*> result;
	set<const DomainElement*> elements = getElements();
	set<const DomainElement*>::iterator elements_it = elements.cbegin();
	while (elements_it != elements.cend()) {
		set<const DomainElement*> elementsIVSet;
		elementsIVSet.insert(*elements_it);
		set<const DomainElement*>::iterator elements_it2 = elements_it;
		++elements_it2;
		while (elements_it2 != elements.cend()) {
			bool isSymmetry = true;
			for (auto relations_it = getRelations().cbegin(); relations_it != getRelations().cend() && isSymmetry; ++relations_it) {
				for (auto sorts_it = getSorts().cbegin(); sorts_it != getSorts().cend() && isSymmetry; ++sorts_it) {
					isSymmetry = isBinarySymmetry(getStructure(), *elements_it, *elements_it2, *relations_it, getSorts());
				}
			}
			set<const DomainElement*>::iterator erase_it2 = elements_it2++;
			if (isSymmetry) {
				elementsIVSet.insert(*erase_it2);
				elements.erase(erase_it2);
			}
		}
		if (elementsIVSet.size() > 1) {
			IVSet* ivset = new IVSet(getStructure(), elementsIVSet, getSorts(), getRelations());
			if (ivset->isRelevantSymmetry()) {
				result.push_back(ivset);
			} else {
				delete ivset;
			}
		}
		set<const DomainElement*>::iterator erase_it = elements_it++;
		elements.erase(erase_it);
	}
	return result;
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

/**
 *	Given a binary symmetry S represented by two domain elements, this method generates two disjunct lists of SAT variables which represent S.
 *	The first list is ordered, and for the ith variable v in either of the lists, S(v) is the ith variable in the other list.
 *	This method is useful in creating short symmetry breaking formulae.
 *
 *	Order is based on the pointers of the domain elements, not on the order given by for instance a SortIterator!
 */
pair<list<int>, list<int> > IVSet::getSymmetricLiterals(AbstractGroundTheory* gt, const DomainElement* smaller, const DomainElement* bigger) const {
	set<const DomainElement*> excludedSet;
	excludedSet.insert(smaller);
	excludedSet.insert(bigger);
	const set<const DomainElement*> emptySet;

	list<int> originals;
	list<int> symmetricals;
	for (auto relations_it = getRelations().cbegin(); relations_it != getRelations().cend(); ++relations_it) {
		if (!hasInterpretation(getStructure(), *relations_it)) {
			set<unsigned int> argumentPlaces = argumentNrs(*relations_it, getSorts());
			for (auto arguments_it = argumentPlaces.cbegin(); arguments_it != argumentPlaces.cend(); ++arguments_it) {
				vector<vector<const DomainElement*> > groundElements(1);
				groundElements[0] = vector<const DomainElement*>((*relations_it)->nrSorts());
				for (unsigned int argument = 0; argument < *arguments_it; ++argument) {
					Sort* currSort = (*relations_it)->sort(argument);
					if (getSorts().count(currSort)) {
						groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, excludedSet);
					} else {
						groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);
					}
				}
				for (unsigned int it = 0; it < groundElements.size(); it++) {
					groundElements[it][*arguments_it] = smaller;
				}
				for (unsigned int argument = (*arguments_it) + 1; argument < (*relations_it)->nrSorts(); ++argument) {
					Sort* currSort = (*relations_it)->sort(argument);
					groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);
				}
				for (auto ge_it = groundElements.cbegin(); ge_it != groundElements.cend(); ++ge_it) {
					ElementTuple original = *ge_it;
					ElementTuple symmetrical = symmetricalTuple(original, smaller, bigger, argumentPlaces);
					originals.push_back(gt->translator()->translateReduced(*relations_it, original, false));
					symmetricals.push_back(gt->translator()->translateReduced(*relations_it, symmetrical, false));
				}
			}
		}
	}
	return pair < list<int>, list<int> > (originals, symmetricals);
}

/**
 * 	For every binary symmetry permuting two succeeding domain elements of an IVSet, this method adds the symmetry breaking clauses to a given theory.
 */
void IVSet::addSymBreakingPreds(AbstractGroundTheory* gt, bool nbModelsEquivalent) const {
	set<const DomainElement*>::const_iterator smaller = getElements().cbegin();
	set<const DomainElement*>::const_iterator bigger = getElements().cbegin();
	++bigger;
	for (; bigger != getElements().cend(); ++bigger, ++smaller) {
		pair<list<int>, list<int> > literals = getSymmetricLiterals(gt, *smaller, *bigger);
		if (nbModelsEquivalent) {
			addSymBreakingClausesToGroundTheory(gt, literals.first, literals.second);
		} else {
			addSymBreakingClausesToGroundTheoryShortest(gt, literals.first, literals.second);
		}
	}
}

/**
 *	The breaking symmetries of an IVSet are the symmetries induced by permuting two succeeding domain elements, and the symmetry induced by permuting the first and last domain element.
 *	The symmetries are given as maps of SAT variables. This method is used to generate input symmetries for the SAT solver.
 */
vector<map<int, int> > IVSet::getBreakingSymmetries(AbstractGroundTheory* gt) const {
	vector<map<int, int> > literalSymmetries;
	set<const DomainElement*>::const_iterator smaller = getElements().cbegin();
	set<const DomainElement*>::const_iterator bigger = getElements().cbegin();
	++bigger;
	for (int i = 1; i < getSize(); ++i) {
		if (bigger == getElements().cend()) {
			smaller = getElements().cbegin();
			--bigger;
		}
		map<int, int> literalSymmetry;
		pair<list<int>, list<int> > symmetricLiterals = getSymmetricLiterals(gt, *smaller, *bigger);
		list<int>::const_iterator first_it = symmetricLiterals.first.cbegin();
		for (auto second_it = symmetricLiterals.second.cbegin(); second_it != symmetricLiterals.second.cend(); ++first_it, ++second_it) {
			literalSymmetry[*first_it] = *second_it;
			literalSymmetry[*second_it] = *first_it;
		}
		literalSymmetries.push_back(literalSymmetry);
		++smaller;
		++bigger;
	}
	return literalSymmetries;
}

/**
 *	If an IVSet is enkelvoudig (Eng: singular), its symmetries are better represented in a SAT solver as interchangeable variable sequences,
 *	instead of a map of variables for each binary symmetry or each breaking symmetry.
 *
 *	For every nth variable v from a list, and for every other list l, there exists a binary symmetry S such that S(v) is the nth literal in l.
 *
 *	@pre: this->isEnkelvoudig()
 */
vector<list<int> > IVSet::getInterchangeableLiterals(AbstractGroundTheory* gt) const {
	Assert(this->isEnkelvoudig());

	const set<const DomainElement*> emptySet;
	vector<list<int> > result;
	for (auto elements_it = getElements().cbegin(); elements_it != getElements().cend(); ++elements_it) {
		result.push_back(list<int>());
	}
	const DomainElement* smallest = *(getElements().cbegin());
	for (auto relations_it = getRelations().cbegin(); relations_it != getRelations().cend(); ++relations_it) {
		if (!hasInterpretation(getStructure(), *relations_it)) {
			set<unsigned int> argumentPlaces = argumentNrs(*relations_it, getSorts());
			unsigned int argumentPlace = *(argumentPlaces.cbegin());
			vector<vector<const DomainElement*> > groundElements(1);
			groundElements[0] = vector<const DomainElement*>((*relations_it)->nrSorts());
			for (unsigned int argument = 0; argument < argumentPlace; ++argument) {
				Sort* currSort = (*relations_it)->sort(argument);
				groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);
			}
			for (unsigned int it = 0; it < groundElements.size(); it++) {
				groundElements[it][argumentPlace] = smallest;
			}
			for (unsigned int argument = argumentPlace + 1; argument < (*relations_it)->nrSorts(); ++argument) {
				Sort* currSort = (*relations_it)->sort(argument);
				groundElements = fillGroundElementsOneRank(groundElements, getStructure()->inter(currSort), argument, emptySet);
			}
			int list = 0;
			for (auto elements_it = getElements().cbegin(); elements_it != getElements().cend(); ++elements_it) {
				for (auto ge_it = groundElements.cbegin(); ge_it != groundElements.cend(); ++ge_it) {
					ElementTuple symmetrical;
					if (*elements_it != smallest) {
						symmetrical = symmetricalTuple(*ge_it, smallest, *elements_it, argumentPlaces);
					} else {
						symmetrical = *ge_it;
					}
					result[list].push_back(gt->translator()->translateReduced(*relations_it, symmetrical, false));
				}
				list++;
			}
		}
	}
	return result;
}

/**********
 * Implementation of TheorySymmetryAnalyzer methods
 **********/

void TheorySymmetryAnalyzer::analyze(const AbstractTheory* t) {
	t->accept(this);
}

void TheorySymmetryAnalyzer::analyzeForOptimization(const Term* t) {
	markAsUnfitForSymmetry(t->sort());
	t->accept(this);
}

/**
 * 	mark a certain sort as unfit for symmetry
 */
void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(Sort* s) {
	addForbiddenSort(s);
}

/**
 * 	mark a certain domain element as unfit for symmetry (when belonging to a certain sort)
 */
void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const DomainElement* e) {
	forbiddenElements_.insert(e);
}

/**
 * 	mark the sorts of a collection of terms as unfit for symmetry
 */
void TheorySymmetryAnalyzer::markAsUnfitForSymmetry(const vector<Term*>& subTerms) {
	for (unsigned int it = 0; it < subTerms.size(); it++) {
		markAsUnfitForSymmetry(subTerms.at(it)->sort());
	}
}

void TheorySymmetryAnalyzer::visit(const PredForm* f) {
	//	clog << "Predform: " << print(f) << "\n";
	if (f->symbol()->builtin() || f->symbol()->overloaded()) {
		if (not is(f->symbol(), STDPRED::EQ)) {
			for (unsigned int it = 0; it < f->args().size(); it++) {
				markAsUnfitForSymmetry(f->subterms().at(it)->sort());
			}
			markAsUnfitForSymmetry(f->args());
		}
	} else {
		addUsedRelation(f->symbol());
	}
	traverse(f);
}

void TheorySymmetryAnalyzer::visit(const FuncTerm* t) {
	if (t->function()->builtin() || t->function()->overloaded()) {
		if (is(t->function(), STDFUNC::MINELEM)) {
			auto st = getStructure()->inter(t->function()->outsort());
			if (not st->empty()) {
				markAsUnfitForSymmetry(st->first());
			}
		} else if (is(t->function(), STDFUNC::MAXELEM)) {
			auto st = getStructure()->inter(t->function()->outsort());
			if (not st->empty()) {
				markAsUnfitForSymmetry(st->last());
			}
		} else {
			markAsUnfitForSymmetry(t->args());
		}
	} else {
		addUsedRelation(t->function());
	}
	traverse(t);
}

void TheorySymmetryAnalyzer::visit(const DomainTerm* t) {
	markAsUnfitForSymmetry(t->value());
}

void TheorySymmetryAnalyzer::visit(const EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::splitComparisonChains(f, getStructure()->vocabulary());
	f->accept(this);
	f->recursiveDelete();
	traverse(ef);
}

void TheorySymmetryAnalyzer::visit(const AggForm* af) {
	markAsUnfitForSymmetry(af->getBound()->sort());
	traverse(af);
}

void TheorySymmetryAnalyzer::visit(const AggTerm* at) {
	if (at->function() != CARD) {
		markAsUnfitForSymmetry(at->sort());
	}
	traverse(at);
}

/**********
 * Symmetry detection methods
 **********/

/**
 * 	Given a collection of sorts and of forbidden elements, extract for each sort the allowed domain elements satisfying
 * 		every allowed domain element for a sort is not a forbidden element
 * 		every allowed domain element for a sort is not a domain element of a child sort
 */
map<Sort*, set<const DomainElement*> > findElementsForSorts(const Structure* s, set<Sort*>& sorts, const set<const DomainElement*>& forbiddenElements) {
	map<Sort*, set<const DomainElement*> > result;
	for (auto sort_it = sorts.cbegin(); sort_it != sorts.cend(); ++sort_it) {
		const SortTable* parent = s->inter(*sort_it);
		set<const DomainElement*> uniqueDomainElements;
		vector<const SortTable*> kids;
		for (auto kids_it = (*sort_it)->children().cbegin(); kids_it != (*sort_it)->children().cend(); ++kids_it) {
			kids.push_back(s->inter(*kids_it));
		}
		for (SortIterator parent_it = parent->sortBegin(); not parent_it.isAtEnd(); ++parent_it) {
			bool isUnique = forbiddenElements.find(*parent_it) == forbiddenElements.cend();
			for (auto kids_it2 = kids.cbegin(); kids_it2 != kids.cend() && isUnique; ++kids_it2) {
				isUnique = not (*kids_it2)->contains(*parent_it);
			}
			if (isUnique) {
				uniqueDomainElements.insert(*parent_it);
			}
		}
		if (uniqueDomainElements.size() > 1) {
			result[*sort_it] = uniqueDomainElements;
		}
	}
	return result;
}

/**
 *	extract the set of non-trivial relations in which every relation ranges over at least one sort of a given set of sorts.
 */
set<PFSymbol*> findNonTrivialRelationsWithSort(const Structure* s, const set<Sort*>& sorts, const set<PFSymbol*>& relations) {
	set<PFSymbol*> result;
	for (auto relation_it = relations.cbegin(); relation_it != relations.cend(); ++relation_it) {
		bool rangesOverSorts = hasTrivialInterpretation(s, *relation_it);
		for (auto sort_it = (*relation_it)->sorts().cbegin(); sort_it != (*relation_it)->sorts().cend() && !rangesOverSorts; ++sort_it) {
			rangesOverSorts = sorts.count(*sort_it);
		}
		if (rangesOverSorts) {
			result.insert(*relation_it);
		}
	}
	return result;
}

/**
 *	by analyzing the given theory and structure with a common vocabulary, initialize potential invariant sets which can be partitioned in real invariant sets.
 *
 *	@pre: t->vocabulary()==s->vocabulary()
 */
set<const IVSet*> initializeIVSets(const Structure* s, const AbstractTheory* t, const Term* minimizeTerm) {
	Assert(t->vocabulary() == s->vocabulary());
	//cout << "token" << print(t) << endl;
	TheorySymmetryAnalyzer tsa(s);
	auto newt = t->clone();
	FormulaUtils::graphFuncsAndAggs(newt, NULL,{}, true, false /*TODO check*/);
	tsa.analyze(newt);
	if (minimizeTerm != NULL) {
		tsa.analyzeForOptimization(minimizeTerm);
	}

	// Find out what sorts can not be used in symmetry:
	set<Sort*> forbiddenSorts;
	for (auto sort_it = tsa.getForbiddenSorts().cbegin(); sort_it != tsa.getForbiddenSorts().cend(); ++sort_it) {
		forbiddenSorts.insert(*sort_it);
		set<Sort*> descendents = (*sort_it)->descendents(s->vocabulary());
		for (auto sort_it2 = descendents.cbegin(); sort_it2 != descendents.cend(); ++sort_it2) {
			forbiddenSorts.insert(*sort_it2);
		}
	}

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Sorts occurring in asymmetric predicates or aggregates (no symmetry will be detected for their interpretations): ";
		for (auto it = forbiddenSorts.cbegin(); it != forbiddenSorts.cend(); ++it) {
			clog << print(*it) << " ";
		}
		clog << "\n";
	}

	// Extract domain elements from forbidden sorts:
	set<const DomainElement*> forbiddenElements;
	for (auto domEl : tsa.getForbiddenElements()) {
		forbiddenElements.insert(domEl);
	}

	for (auto sort_it = forbiddenSorts.cbegin(); sort_it != forbiddenSorts.cend(); ++sort_it) {
		if (isSortForSymmetry(*sort_it, s)) {
			for (SortIterator element_it = (s->inter(*sort_it))->sortBegin(); not element_it.isAtEnd(); ++element_it) {
				forbiddenElements.insert(*element_it);
			}
		}
	}

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Elements occurring in theory (no symmetry will be detected for these elements): ";
		for (auto it = forbiddenElements.cbegin(); it != forbiddenElements.cend(); ++it) {
			clog << print(*it) << " ";
		}
		clog << "\n";
	}

	// Find out what sorts are used in the given theory:
	set<Sort*> allowedSorts;
	for (auto relation_it = tsa.getUsedRelations().cbegin(); relation_it != tsa.getUsedRelations().cend(); ++relation_it) {
		for (auto sort_it = (*relation_it)->sorts().cbegin(); sort_it != (*relation_it)->sorts().cend(); ++sort_it) {
			if (!forbiddenSorts.count(*sort_it) && isSortForSymmetry(*sort_it, s)) {
				allowedSorts.insert(*sort_it);
			}
		}
	}

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Sorts for which symmetry will be detected: ";
		for (auto it = allowedSorts.cbegin(); it != allowedSorts.cend(); ++it) {
			clog << print(*it) << " ";
		}
		clog << "\n";
	}

	// Extract elements that occur in interpretations of allowed sorts, and are not used in the forbidden sorts:
	map<const DomainElement*, set<Sort*> > allowedElements;
	for (auto sort_it = allowedSorts.cbegin(); sort_it != allowedSorts.cend(); ++sort_it) {
		for (SortIterator element_it = (s->inter(*sort_it))->sortBegin(); not element_it.isAtEnd(); ++element_it) {
			if (!forbiddenElements.count(*element_it)) {
				if (!allowedElements.count(*element_it)) {
					allowedElements.insert(pair<const DomainElement*, set<Sort*> >(*element_it, set<Sort*>()));
				}
				allowedElements.at(*element_it).insert(*sort_it);
			}
		}
	}

	// Group allowed domain elements by the sorts they occur in:
	map<set<Sort*>, set<const DomainElement*> > initialIVSets;
	for (auto elements_it = allowedElements.cbegin(); elements_it != allowedElements.cend(); ++elements_it) {
		set<Sort*> sortset = elements_it->second;
		if (!initialIVSets.count(sortset)) {
			initialIVSets.insert(pair<set<Sort*>, set<const DomainElement*> >(sortset, set<const DomainElement*>()));
		}
		initialIVSets.at(sortset).insert(elements_it->first);
	}

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Candidate sets of interchangeable domain elements: \n";
		for (auto it = initialIVSets.cbegin(); it != initialIVSets.cend(); ++it) {
			clog << "The elements: ";
			for (auto elements_it = it->second.cbegin(); elements_it != it->second.cend(); ++elements_it) {
				clog << print(*elements_it) << " ";
			}
			clog << "\n";
			clog << "With as sorts: ";
			for (auto sorts_it = it->first.cbegin(); sorts_it != it->first.cend(); ++sorts_it) {
				clog << print(*sorts_it) << " ";
			}
			clog << "\n";
		}
	}

	// Create invariant sets based on the grouped domain elements:
	set<const IVSet*> result;
	for (auto ivset_it = initialIVSets.cbegin(); ivset_it != initialIVSets.cend(); ++ivset_it) {
		set<const DomainElement*> elements = ivset_it->second;
		if (elements.size() > 1) {
			set<Sort*> sorts = ivset_it->first;
			set<PFSymbol*> relations = findNonTrivialRelationsWithSort(s, sorts, tsa.getUsedRelations());
			IVSet* ivset = new IVSet(s, elements, sorts, relations);
			if (ivset->isRelevantSymmetry()) {
				result.insert(ivset);
			} else {
				delete ivset;
			}
		}
	}

	return result;
}

/**
 * 	check which of the IVSets are don't cares.
 * 	These IVSets are trivial invariant sets, and are separated from the list of potential invariant sets.
 */
vector<const IVSet*> extractDontCares(set<const IVSet*>& potentials) {
	vector<const IVSet*> result;
	set<const IVSet*>::iterator potentials_it = potentials.cbegin();
	while (potentials_it != potentials.cend()) {
		set<const IVSet*>::iterator erase_it = potentials_it++;
		if ((*erase_it)->isDontCare()) {
			result.push_back(*erase_it);
			potentials.erase(erase_it);
		}
	}
	return result;
}

/**
 *	Split a collection of potential invariant sets in new potential invariant sets by counting the occurrence of domain elements in the partial structure.
 */
void splitByOccurrences(set<const IVSet*>& potentials) {
	if (!potentials.empty()) {
		OccurrencesCounter counter((*potentials.cbegin())->getStructure());
		vector<vector<const IVSet*> > splittedSets;
		set<const IVSet*>::iterator potentials_it = potentials.cbegin();
		while (potentials_it != potentials.cend()) {
			splittedSets.push_back((*potentials_it)->splitBasedOnOccurrences(&counter));
			delete (*potentials_it);
			set<const IVSet*>::iterator erase_it = potentials_it++;
			potentials.erase(erase_it);
		}
		for (auto it1 = splittedSets.cbegin(); it1 != splittedSets.cend(); ++it1) {
			for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
				potentials.insert(*it2);
			}
		}
	}
}

/**
 * 	Split a collection of potential invariant sets in real invariant sets by checking for the relevant binary permutations whether they are invariant permutations, and thusly induce a binary symmetry.
 */
void splitByBinarySymmetries(set<const IVSet*>& potentials) {
	vector<vector<const IVSet*> > splittedSets;
	set<const IVSet*>::iterator potentials_it = potentials.cbegin();
	while (potentials_it != potentials.cend()) {
		splittedSets.push_back((*potentials_it)->splitBasedOnBinarySymmetries());
		delete (*potentials_it);
		set<const IVSet*>::iterator erase_it = potentials_it++;
		potentials.erase(erase_it);
	}
	for (auto it1 = splittedSets.cbegin(); it1 != splittedSets.cend(); ++it1) {
		for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
			potentials.insert(*it2);
		}
	}
}

/**
 * 	General method which, given a theory and structure, detects IVSets.
 *
 * 	@pre: t->vocabulary()==s->vocabulary()
 */

vector<const IVSet*> findIVSets(const AbstractTheory* t, const Structure* s, const Term* minimizeTerm) {
	Assert(t->vocabulary() == s->vocabulary());

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Starting symmetry detection.\n";
	}
	set<const IVSet*> potentials = initializeIVSets(s, t, minimizeTerm);

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Extracting don't cares (domain element sets not occurring in the interpretation of a relation symbol).\n";
	}

	vector<const IVSet*> result = extractDontCares(potentials);
	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		for (auto result_it = result.cbegin(); result_it != result.cend(); ++result_it) {
			clog << print(*result_it) << "\n";
		}
	}

	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		clog << "Extracting other interchangeable domain element sets.\n";
	}
	splitByOccurrences(potentials);
	splitByBinarySymmetries(potentials);
	if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		for (auto result_it = potentials.cbegin(); result_it != potentials.cend(); ++result_it) {
			clog << print(*result_it) << "\n";
		}
	}
	insertAtEnd(result, potentials);
	return result;
}

/**
 * 	Generates ground symmetry breaking CNF clauses for a ground theory and a collection of invariant sets.
 *
 * 	@post: the ivsets are deleted
 */
void addSymBreakingPredicates(AbstractGroundTheory* gt, vector<const IVSet*> ivsets, bool nbModelsEquivalent) {
	for (auto ivsets_it = ivsets.cbegin(); ivsets_it != ivsets.cend(); ++ivsets_it) {
		(*ivsets_it)->addSymBreakingPreds(gt, nbModelsEquivalent);
		delete (*ivsets_it);
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

void getIntchGroups2(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<std::pair<PFSymbol*, unsigned int> >& forcedSymbArgs){
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
	getIntchGroups2(theo, s, out_groups, tmp);
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
    
    std::list<int> l1;
    for(auto l:lits){
      l1.push_back(l);
    }
    std::list<int> l2;
    for(auto l:sym_lits){
      l2.push_back(l);
    }
    if (nbModelsEquivalent) {
      addSymBreakingClausesToGroundTheory(gt, l1, l2);
    } else {
      addSymBreakingClausesToGroundTheoryShortest(gt, l1,l2); // TODO: convert list to vector
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