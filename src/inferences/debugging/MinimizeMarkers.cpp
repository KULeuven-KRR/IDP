#include <cstdlib>
#include <memory>
#include <vector>
#include "MinimizeMarkers.hpp"
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "utils/ListUtils.hpp"


namespace MinimizeMarkers {
MXAssumptions minimizeAssumps(AbstractTheory *newtheory, Structure *s, MXAssumptions markers) {
    auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, markers);
    if (not mxresult.unsat) {
        throw AlreadySatisfiableException();
    }

    std::cout <<
    ">>> Unsatisfiable subset found, trying to reduce its size (might take some time, can be interrupted with ctrl-c.\n";

    // TODO should set remaining markers on true to allow earlier pruning
    auto core = mxresult.unsat_explanation;

    while(minimizeSubArray(newtheory, s, core.assumeTrue, core, core.size()));
    while(minimizeSubArray(newtheory, s, core.assumeFalse, core, core.size()));
        
    return core;
}

bool minimizeSubArray(AbstractTheory *newtheory, Structure *s, std::vector <DomainAtom> &curArr,
                      MXAssumptions &core, uint goal) {
    //-1 == uint.maxsize -> catch this by checking curElem < arraysize
    for(uint curElem = curArr.size()-1 ; curElem < curArr.size() ; curElem--){
        if (getGlobal()->terminateRequested()) {
            getGlobal()->reset();
            return false;
        }
        auto elem = curArr[curElem];
        curArr.erase(curArr.begin()+curElem);

        auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, core);
        if (mxresult._interrupted) {
            return false;
        }
        if (not mxresult.unsat) {
            curArr.push_back(elem);
        } else if (mxresult.unsat_explanation.size() < goal) {
            core = mxresult.unsat_explanation;
            return true;
        }
    }
    return false;
}

}