#include <cstdlib>
#include <memory>
#include <vector>
#include "MinimizeMarkers.hpp"
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "utils/ListUtils.hpp"

bool minimizeSubArray(const AbstractTheory *newtheory, const Structure *s, std::vector <DomainAtom> &curArr,
                      MXAssumptions &core, bool &stop);

 MXAssumptions minimizeAssumps(AbstractTheory *newtheory, Structure *s, MXAssumptions markers) {
    auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, markers);
    if (not mxresult.unsat) {
        throw AlreadySatisfiableException();
    }

    std::cout <<
    ">>> Unsatisfiable subset found, trying to reduce its size (might take some time, can be interrupted with ctrl-c.\n";

    // TODO should set remaining markers on true to allow ealier pruning
    auto core = mxresult.unsat_explanation;
    auto erased = true;
    auto stop = false;
    while (erased && not stop) {
        if (getGlobal()->terminateRequested()) {
            getGlobal()->reset();
            stop = true;
            break;
        }
        erased = false;

        while(
                !stop && (
                minimizeSubArray(newtheory, s, core.assumeTrue, core, stop,core.size()) ||
                minimizeSubArray(newtheory, s, core.assumeFalse, core, stop,core.size()))
        );
    }
    auto output = mxresult.unsat_explanation;
    return output;
}

bool minimizeSubArray(AbstractTheory *newtheory, Structure *s, std::vector <DomainAtom> &curArr,
                      MXAssumptions &core, bool &stop, uint goal) {
    uint maxsize = curArr.size();
    for (uint i = 0; i < maxsize;) {
        if (getGlobal()->terminateRequested()) {
            getGlobal()->reset();
            stop = true;
            break;
        }
        auto elem = curArr[i];

//This serves to prevent self-swapping (Cf. Issue 739)
        if (not (curArr[i].symbol == curArr[maxsize - 1].symbol && curArr[i].args == curArr[maxsize - 1].args)) {
            std::swap(curArr[i], curArr[maxsize - 1]);
        }
        curArr.pop_back();
        maxsize--;
        auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, core);
        if (mxresult._interrupted) {
            stop = true;
            return false;
        }
        if (not mxresult.unsat) {
            curArr.push_back(elem);
        } else {
            if (mxresult.unsat_explanation.size() < goal) {
                core = mxresult.unsat_explanation;
                return true;
            }
        }
    }
    return false;
}
