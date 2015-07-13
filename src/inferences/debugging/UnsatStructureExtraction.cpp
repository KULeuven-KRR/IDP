#include <cstdlib>
#include <vector>

#include <inferences/modelexpansion/ModelExpansion.hpp>
#include "UnsatStructureExtraction.hpp"
#include "IncludeComponents.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/TheoryUtils.hpp"
#include "creation/cppinterface.hpp"

#include "MinimizeMarkers.hpp"
Structure* UnsatStructureExtraction::extractStructure(AbstractTheory* intheory, Structure* structure, Vocabulary* vAssume) {
    //Clone theory/structure/vocabulary so we can modify them
    std::stringstream ss;
    ss << "unsatstruct_voc" << getGlobal()->getNewID();
    auto voc = new Vocabulary(ss.str());
    voc->add(intheory->vocabulary());
    auto newtheory =intheory->clone();
    newtheory->vocabulary(voc);

    //The new structure is completely empty except for the sorts
    Structure* emptyStruc = new Structure("empty", newtheory->vocabulary(), ParseInfo());
    auto sorts = structure->getSortInters();
    for (const auto& kv : structure->getSortInters()) {
        emptyStruc->changeInter(kv.first,kv.second);
    }

    std::vector<DomainAtom> assumeNeg;
    std::vector<DomainAtom> assumePos;
    //TODO: Need generalized changeInter to ommit duplicate code for preds/funcs
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
    return emptyStruc;
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
