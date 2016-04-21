#include <cstdlib>
#include <vector>

#include <inferences/modelexpansion/ModelExpansion.hpp>
#include "UnsatExtraction.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/TheoryUtils.hpp"
#include "creation/cppinterface.hpp"

#include "MinimizeMarkers.hpp"
#include "AddMarkers.hpp"

pair<Structure*,Theory*> UnsatExtraction::extractCore(bool assumeStruc, bool assumeTheo, AbstractTheory* intheory, Structure* structure, Vocabulary* vAssume) {
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
    MXAssumptions coreresult = MinimizeMarkers::minimizeAssumps(newtheory, emptyStruc, assume);

    if(assumeStruc){
        outputStructure(intheory, emptyStruc, coreresult);
    }
    Theory* outTheo = 0;
    if(assumeTheo){
        outTheo = outputTheory(am, coreresult, intheory->vocabulary());
        newtheory->recursiveDelete();
    }

    emptyStruc->changeVocabulary(intheory->vocabulary());
    delete (voc);
    return pair<Structure*,Theory*>(emptyStruc,outTheo);
}

Theory*UnsatExtraction::outputTheory(const AddMarkers *am,
                                     MXAssumptions &theoryMarkers,
                                        Vocabulary* voc) {
    Theory* outputTheo;
    auto core = am->getComponentsFromMarkers(theoryMarkers.assumeFalse);
    outputTheo = new Theory("unsat_core", voc, ParseInfo());
    for(auto c: core){
        outputTheo->add(c);
    }
    delete (am);
    return outputTheo;
}

void UnsatExtraction::outputStructure(const AbstractTheory *intheory, const Structure *emptyStruc,
                                      MXAssumptions &coreresult) {
    for(DomainAtom da : coreresult.assumeTrue){
        if(intheory->vocabulary()->contains(da.symbol)){
            PredInter* target = emptyStruc->inter(da.symbol);
            target->makeTrueExactly(da.args,true);
        }
    }
    for(DomainAtom da : coreresult.assumeFalse){
        if(intheory->vocabulary()->contains(da.symbol)){
            PredInter* target = emptyStruc->inter(da.symbol);
            target->makeFalseExactly(da.args,true);
        }
    }
}

void UnsatExtraction::assumifyTheory(Theory *&newtheory, vector<Predicate*> &assumeAllFalse, AddMarkers *&am) {
    am = new AddMarkers();
    newtheory = am->execute(newtheory);
    assumeAllFalse = am->getMarkers();
}

void UnsatExtraction::assumifyStructure(const Structure *structure, const Vocabulary *vAssume,
                                                 const Theory *newtheory, Structure *&emptyStruc,
                                                 vector<DomainAtom> &assumeNeg, vector<DomainAtom> &assumePos) {
    emptyStruc= new Structure(emptyStruc->name() + "_reduced", newtheory->vocabulary(), ParseInfo());
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
            tableToVector(assumePos, symbol, pi->ct());
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

void UnsatExtraction::tableToVector(std::vector<DomainAtom> &assumeNeg, PFSymbol *const symbol,
                                             const PredTable *cfTab) {
    for(auto it = cfTab->begin(); not it.isAtEnd() ; ++it){
        DomainAtom da;
        da.symbol = symbol;
        da.args = *it;
        assumeNeg.push_back(da);
    }
}
