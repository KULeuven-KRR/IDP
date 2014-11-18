/* 
 * File:   ModelIterator.hpp
 * Author: rupsbant
 *
 * Created on October 3, 2014, 9:56 AM
 */

#ifndef MODELITERATOR_HPP
#define	MODELITERATOR_HPP

#include "common.hpp"
#include "../modelexpansion/ModelExpansion.hpp"
#include "inferences/SolverInclude.hpp"

class Definition;
class Structure;
class AbstractTheory;
class AbstractGroundTheory;
class StructureExtender;
class Theory;
class TraceMonitor;
class Term;
class Vocabulary;
class PredForm;
class Predicate;
class PFSymbol;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

class ModelIterator {
public:
    ModelIterator(Structure*, Theory*, Vocabulary*, TraceMonitor*, const MXAssumptions&);
    ModelIterator(const ModelIterator& orig);
    virtual ~ModelIterator();
    void init();
    MXResult calculate();
private:
    std::vector<Definition*> preprocess(Theory*);
    void ground(Theory*);
    MXResult getStructure(PCModelExpand*, MXResult, clock_t);
    
    Structure* _structure;
    Theory* _theory;
    TraceMonitor* _tracemonitor;
    std::vector<Definition*> postprocessdefs;

    Vocabulary* _outputvoc; // if not NULL, mx is allowed to return models which are only two-valued on the outputvoc.
    Vocabulary* _currentVoc;
    MXAssumptions _assumeFalse;
    

    PCSolver*  _data;
    AbstractGroundTheory* _grounding;
    StructureExtender* _extender;
    litlist _assumptions;
};

#endif	/* MODELITERATOR_HPP */

