#include <cstdlib>
#include <vector>

#include <inferences/modelexpansion/ModelExpansion.hpp>
#include "UnsatStructureExtraction.hpp"
#include "IncludeComponents.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/TheoryUtils.hpp"
#include "creation/cppinterface.hpp"

#include "MinimizeMarkers.hpp"
#include "AddMarkers.hpp"

Structure* UnsatStructureExtraction::extractStructure(bool assumeStruc, bool assumeTheo, AbstractTheory* intheory, Structure* structure, Vocabulary* vAssume) {
    //Clone theory/structure/vocabulary so we can modify them
    std::stringstream ss;
    ss << "unsatstruct_voc" << getGlobal()->getNewID();
    auto voc = new Vocabulary(ss.str());
    voc->add(intheory->vocabulary());
    auto newtheory = dynamic_cast<Theory*>(intheory)->clone();
    if (newtheory == NULL) {
        throw notyetimplemented("Unsatcore extraction for non first-order theories");
    }
    newtheory->vocabulary(voc);
    Structure *emptyStruc = structure;

    MXAssumptions assume;
    AddMarkers *am;

    if(assumeStruc){
        assumifyStructure(structure, vAssume, newtheory, emptyStruc, assume.assumeFalse, assume.assumeTrue);
    }
    if(assumeTheo){
        assumifyTheory(newtheory, assume.assumeAllFalse, am);
    }

    //Copy the literals from the result to the output structure
    vector<DomainAtom> coreresult = minimizeAssumps(newtheory, emptyStruc, assume);
    vector<DomainAtom> theoryMarkers;

    if(assumeStruc){
        theoryMarkers = outputStructure(intheory, structure, emptyStruc, coreresult, theoryMarkers);
    }
    if(assumeTheo){
        outputTheory(assumeTheo, am, theoryMarkers);
    }

    emptyStruc->changeVocabulary(intheory->vocabulary());
    delete (voc);
    newtheory->recursiveDelete();
    return emptyStruc;
}

void UnsatStructureExtraction::outputTheory(bool assumeTheo, const AddMarkers *am,
                                            const vector<DomainAtom> &theoryMarkers) const {
    Theory* outputTheo;
    if(assumeTheo){
            auto core = am->getComponentsFromMarkers(theoryMarkers);
            outputTheo = new Theory("unsat_core", theory->vocabulary(), {});
            for(auto c: core){
                outputTheo->add(c);
            }
            delete (am);
        }
}

vector<DomainAtom> &UnsatStructureExtraction::outputStructure(const AbstractTheory *intheory,
                                                              const Structure *structure, const Structure *emptyStruc,
                                                              vector<DomainAtom> &coreresult,
                                                              vector<DomainAtom> &theoryMarkers) const {
    for(DomainAtom da : coreresult){
            if(intheory->vocabulary()->contains(da.symbol)){
                cout << da.symbol->nameNoArity();
                PredInter* orig = structure->inter(da.symbol);
                PredInter* target = emptyStruc->inter(da.symbol);
                if(orig->isTrue(da.args,true)){
                    target->makeTrueExactly(da.args,true);
                }else{
                    target->makeFalseExactly(da.args,true);
                }
            }else{
                theoryMarkers.push_back(da);
            }
        }
    coreresult = theoryMarkers;
    return theoryMarkers;
}

void UnsatStructureExtraction::assumifyTheory(Theory *&newtheory, vector<Predicate*> &assumeAllFalse, AddMarkers *&am) {
    am = new AddMarkers();
    newtheory = am->execute(newtheory);
    assumeAllFalse = am->getMarkers();
}

void UnsatStructureExtraction::assumifyStructure(const Structure *structure, const Vocabulary *vAssume,
                                                 const Theory *newtheory, Structure *&emptyStruc,
                                                 vector<DomainAtom> &assumeNeg, vector<DomainAtom> &assumePos) {
    emptyStruc= new Structure("empty", newtheory->vocabulary(), ParseInfo());
    //The new structure is completely empty except for the sortsauto sorts = structure->getSortInters();
    for (const auto& kv : structure->getSortInters()) {
        emptyStruc->changeInter(kv.first,kv.second);
    }//TODO: Need generalized changeInter to ommit duplicate code for preds/funcs
    for(const auto& kv : structure->getPredInters()) {
        auto symbol = kv.first;
        auto pi = kv.second;
        //If the symbol is in the assumption vocabulary, it is assumed
        if(vAssume->contains(symbol)){
            tableToVector(assumeNeg, symbol, pi->cf());
            tableToVector(assumePos,symbol,pi->ct());
        }else{
            //Else it is added to the empty structure
            emptyStruc->changeInter(symbol,pi->clone());
        }
    }
    for(const auto& kv : structure->getFuncInters()) {
        auto symbol = kv.first;
        auto pi = kv.second->graphInter();
        //If the symbol is in the assumption vocabulary, it is assumed
        if(vAssume->contains(symbol)){
            tableToVector(assumeNeg, symbol, pi->cf());
            tableToVector(assumePos,symbol,pi->ct());
        }else{
            //Else it is added to the empty structure
            emptyStruc->changeInter(symbol,kv.second->clone());
        }
    }
}

void UnsatStructureExtraction::tableToVector(std::vector<DomainAtom> &assumeNeg, PFSymbol *const symbol,
                                             const PredTable *cfTab) {
    for(auto it = cfTab->begin(); not it.isAtEnd() ; ++it){
        DomainAtom da;
        da.symbol = symbol;
        da.args = *it;
        assumeNeg.push_back(da);
    }
}
