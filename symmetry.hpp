/************************************
	symmetry.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <set>
#include <map>
#include <vector>
#include <string>
#include <list>

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
 * 		occurring in the domain of all the sorts of a set of sorts sorts_, 
 * 	and a set of PFSymbols relations_, 
 * 		which contains all the relations occurring in a theory who hava as argument at least one of the elements of sorts_
 * TODO: this must be checked more thoroughly, which requires a bit of refactoring...
 */
class IVSet {
	
	private:
		// Attributes
		const AbstractStructure*			structure_;				//!< The structure over which this symmetry ranges
		const std::set<const DomainElement*>elements_;		//!< Elements which are permuted by the symmetry's
		const std::set<Sort*>				sorts_;			//!< Sorts considered for the elements 
		const std::set<PFSymbol*>			relations_;		//!< Relations which are permuted by the symmetry's
		
		// Inspectors
		const std::set<const DomainElement*>& getElements() const;
		const std::set<Sort*>& getSorts() const;
		const std::set<PFSymbol*>& getRelations() const;
		
		std::pair<std::list<int>,std::list<int> > getSymmetricLiterals(AbstractGroundTheory*, const DomainElement*, const DomainElement*) const;

	public:
		// Constructors 
		IVSet(const AbstractStructure*, const std::set<const DomainElement*>, const std::set<Sort*>, const std::set<PFSymbol*>);  

		// Destructor
		~IVSet();	//!< Destructor
		
		// Mutators
		// none should come here, class is immutable :)

		// Inspectors
		const AbstractStructure* 	getStructure() const;
		
		bool 						containsMultipleElements() const;
		bool						hasRelevantRelationsAndSorts() const;
		bool						isRelevantSymmetry() const;
		bool 						isDontCare() const;
		bool						isEnkelvoudig() const;
		std::vector<const IVSet*>	splitBasedOnOccurrences(OccurrencesCounter*) const;
		std::vector<const IVSet*>	splitBasedOnBinarySymmetries() const;
		
		void addSymBreakingPreds(AbstractGroundTheory*) const;
		
		
		// Output
		std::string		to_string()									const;
};

std::vector<const IVSet*> findIVSets(const AbstractTheory*, const AbstractStructure*);

void addSymBreakingPredicates(AbstractGroundTheory*, std::vector<const IVSet*>);
