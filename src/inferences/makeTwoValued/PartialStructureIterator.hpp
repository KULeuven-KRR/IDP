/* 
 * File:   PartialStructureIterator.hpp
 * Author: rupsbant
 *
 * Created on December 5, 2014, 3:19 PM
 */

#ifndef PARTIALSTRUCTUREITERATOR_HPP
#define	PARTIALSTRUCTUREITERATOR_HPP

class PartialStructureIterator {
public:
    PartialStructureIterator();
    PartialStructureIterator(const PartialStructureIterator& orig);
    virtual ~PartialStructureIterator();
    
    Structure next();
private:
    
};

#endif	/* PARTIALSTRUCTUREITERATOR_HPP */

