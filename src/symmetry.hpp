/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SYMMETRY_HPP_
#define SYMMETRY_HPP_

#include <set>
#include <map>
#include <vector>
#include <string>
#include <list>
#include <ostream>

class DomainElement;
class AbstractStructure;
class AbstractTheory;
class AbstractGroundTheory;
class Sort;
class PFSymbol;
class OccurrencesCounter;

/** 
 *	\brief	Abstract base class to represent a symmetry group
 */

/**
 * Definition of an IVSet: 
 * 	a set of domain elements elements_, 
 * 		occurring in the domain of all the sorts of sorts_,
 * 		not occurring in the domain of all sorts not in sorts_, but whose parents are in sorts_.
 *	a set of sorts sorts_,
 *		which is a fixpoint under the "add parent sorts" operation.
 * 	and a set of PFSymbols relations_, 
 * 		which contains all the relations occurring in a theory who have as argument at least one of the elements of sorts_
 *
 * INVARIANT: every IVSet has getSize()>1.
 * INVARIANT: the domain elements of an IVSet all belong to the youngest sort in sorts_. The other sorts in sorts_ are parents of this youngest sort
 * (unfortunately, the last invariant is not enforced by an assertion, even though it is assumed it holds)
 */
class IVSet {
	
	private:
		// Attributes
		const AbstractStructure*			structure_;				//!< The structure over which this symmetry ranges
		const std::set<const DomainElement*>elements_;		//!< Elements which are permuted by the symmetry's
		const std::set<Sort*>				sorts_;			//!< Sorts considered for the elements 
		const std::set<PFSymbol*>			relations_;		//!< Relations which are permuted by the symmetry's
		
		// Inspectors
		std::pair<std::list<int>,std::list<int> > getSymmetricLiterals(AbstractGroundTheory*, const DomainElement*, const DomainElement*) const;

	public:
		IVSet(const AbstractStructure*, const std::set<const DomainElement*>, const std::set<Sort*>, const std::set<PFSymbol*>);  
		~IVSet(){}
		
		// Mutators
		// none should come here, class is immutable :)

		// Inspectors
		const AbstractStructure* 	getStructure() const;
		const std::set<const DomainElement*>& getElements() const;
		const std::set<Sort*>& getSorts() const;
		const std::set<PFSymbol*>& getRelations() const;
		
		int							getSize() const {return getElements().size();}
		bool 						containsMultipleElements() const;
		bool						hasRelevantRelationsAndSorts() const;
		bool						isRelevantSymmetry() const;
		bool 						isDontCare() const;
		bool						isEnkelvoudig() const;
		std::vector<const IVSet*>	splitBasedOnOccurrences(OccurrencesCounter*) const;
		std::vector<const IVSet*>	splitBasedOnBinarySymmetries() const;
		
		void 						addSymBreakingPreds(AbstractGroundTheory*) const;
		std::vector<std::map<int,int> >	getBreakingSymmetries(AbstractGroundTheory*) const;
		std::vector<std::list<int> >getInterchangeableLiterals(AbstractGroundTheory*) const;
		
		// Output
		std::ostream& put(std::ostream& output) const;
};

std::vector<const IVSet*> findIVSets(const AbstractTheory*, const AbstractStructure*);

void addSymBreakingPredicates(AbstractGroundTheory*, std::vector<const IVSet*>);

#endif /* SYMMETRY_HPP_ */