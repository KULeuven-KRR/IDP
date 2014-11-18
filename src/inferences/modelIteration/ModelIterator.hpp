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
    ModelIterator();
    ModelIterator(const ModelIterator& orig);
    virtual ~ModelIterator();
private:
    Structure* getStructure(MXResult);
    int getMXVerbosity();
    void init();
    std::vector<Definition*> preprocess();
    void ground();
    void calculate();
    
    Structure* _structure;
    TraceMonitor* _tracemonitor;
    Term* _minimizeterm; // if NULL, no optimization is done
    std::vector<Definition*> postprocessdefs;

    Vocabulary* _outputvoc; // if not NULL, mx is allowed to return models which are only two-valued on the outputvoc.
    Vocabulary* _currentVoc;
    MXAssumptions _assumeFalse;
    

    PCSolver*  _data;
    AbstractGroundTheory* _grounding;
    StructureExtender* _extender;
};

#endif	/* MODELITERATOR_HPP */

