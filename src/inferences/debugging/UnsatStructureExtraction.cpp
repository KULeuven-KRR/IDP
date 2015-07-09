#include <cstdlib>
#include <vector>

#include <inferences/modelexpansion/ModelExpansion.hpp>
#include "UnsatStructureExtraction.hpp"
#include "IncludeComponents.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/TheoryUtils.hpp"
#include "creation/cppinterface.hpp"

#include "MinimizeMarkers.hpp"
UnsatStructureResult UnsatStructureExtraction::extractStructure(AbstractTheory* intheory, Structure* structure, Vocabulary* vAssume) {
    //Clone theory/structure/vocabulary so we can modify them
    std::stringstream ss;
    ss << "unsatstruct_voc" << getGlobal()->getNewID();
    auto voc = new Vocabulary(ss.str());
    voc->add(intheory->vocabulary());
    auto newtheory = dynamic_cast<Theory*>(intheory->clone());
    newtheory->vocabulary(voc);

    //The clone of the structure is completely empty except for the sorts
    Structure* emptyStruc = new Structure("empty", newtheory->vocabulary(), ParseInfo());
    auto sorts = structure->getSortInters();
    for (const auto& kv : structure->getSortInters()) {
        emptyStruc->changeInter(kv.first,kv.second);
    }

    //All literals in the structure need to be assumed
    std::vector<DomainAtom> assumeNeg;
    std::vector<DomainAtom> assumePos;
    for(const auto& kv : structure->getPredInters()) {
        auto symbol = kv.first;
        auto pi = kv.second;

        //If the symbol is in the assumption vocabulary, it is assumed
        if(vAssume->contains(symbol)){
            PredTable* cfTab = pi->cf();
            for(auto it = cfTab->begin(); not it.isAtEnd() ; ++it){
                DomainAtom da;
                da.symbol = symbol;
                da.args = *it;
                assumeNeg.push_back(da);
            }

            PredTable* ctTab = pi->ct();
            for(auto it = ctTab->begin(); not it.isAtEnd() ; ++it){
                DomainAtom da;
                da.symbol = symbol;
                da.args = *it;
                assumePos.push_back(da);
            }
        }else{
            //Else it is added to the empty structure
            emptyStruc->changeInter(symbol,pi->clone());
        }

    }

    for(const auto& kv : structure->getFuncInters()) {
        auto symbol = kv.first;
        auto pi = kv.second->graphInter();

        PredTable* cfTab = pi->cf();
        for(auto it = cfTab->begin(); not it.isAtEnd() ; ++it){
            DomainAtom da;
            da.symbol = symbol;
            da.args = *it;
            assumeNeg.push_back(da);
        }

        PredTable* ctTab = pi->ct();
        for(auto it = ctTab->begin(); not it.isAtEnd() ; ++it){
            DomainAtom da;
            da.symbol = symbol;
            da.args = *it;
            assumePos.push_back(da);
        }
    }



    MXAssumptions assume;
    assume.assumeFalse = assumeNeg;
    assume.assumeTrue = assumePos;


    //Copy the literals from the result to the output structure
    auto coreresult = minimizeAssumps(newtheory, emptyStruc, assume);
    for(DomainAtom da : coreresult){
        PredInter* orig = structure->inter(da.symbol);
        PredInter* target = emptyStruc->inter(da.symbol);
        if(orig->isTrue(da.args,true)){
            target->makeTrueExactly(da.args,true);
        }else{
            target->makeFalseAtLeast(da.args,true);
        }
    }
    emptyStruc->changeVocabulary(intheory->vocabulary());
    delete (voc);
    newtheory->recursiveDelete();
    UnsatStructureResult r;
    r.succes = true;
    r.core = emptyStruc;
    return r;
}
