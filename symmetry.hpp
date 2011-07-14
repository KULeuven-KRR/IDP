/************************************
	symmetry.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <set>
#include <map>
#include <vector>
#include <string>

class DomainElement;
class AbstractStructure;
class AbstractTheory;
class Sort;
class PFSymbol;
class OccurrencesCounter;

/** 
 *	\brief	Abstract base class to represent a symmetry group
 */
class IVSet {
	private:
		// Attributes
		const AbstractStructure*			structure_;				//!< The structure over which this symmetry ranges
		const std::set<const DomainElement*>elements_;		//!< Elements which are permuted by the symmetry's
		const std::set<Sort*>				sorts_;			//!< Sorts considered for the elements 
		const std::set<PFSymbol*>			relations_;		//!< Relations which are permuted by the symmetry's
		
		// Inspectors
		const AbstractStructure* getStructure() const;
		const std::set<const DomainElement*>& getElements() const;
		const std::set<Sort*>& getSorts() const;
		const std::set<PFSymbol*>& getRelations() const;

	public:
		// Constructors 
		IVSet(const std::set<const DomainElement*>, const std::set<Sort*>, const std::set<PFSymbol*>);  

		// Destructor
		~IVSet();	//!< Destructor
		
		// Mutators

		// Inspectors
		bool 						containsMultipleElements() const;
		bool 						isDontCare(const AbstractStructure*) const;
		std::vector<const IVSet*>	splitBasedOnOccurrences(OccurrencesCounter*) const;
		std::vector<const IVSet*>	splitBasedOnBinarySymmetries(const AbstractStructure*) const;
		
		
		// Output
		std::string		to_string()									const;
};

std::vector<const IVSet*> findIVSets(const AbstractTheory*, const AbstractStructure*);
