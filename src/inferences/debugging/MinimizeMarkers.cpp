#include <cstdlib>
#include <memory>
#include <vector>

#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "utils/ListUtils.hpp"

std::vector<DomainAtom> minimizeAssumps(Theory *newtheory, Structure *s, MXAssumptions markers) {
    auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL,  markers);
    if (not mxresult.unsat) {
        throw IdpException("Can not get unsatcore of satisfiable theory");
    }

    std::cout << ">>> Unsatisfiable subset found, trying to reduce its size (might take some time, can be interrupted with ctrl-c.\n";

    // TODO should set remaining markers on true to allow ealier pruning
    auto core = mxresult.unsat_in_function_of_ct_lits;
    auto erased = true;
    auto stop = false;
    while (erased && not stop) {
        if (getGlobal()->terminateRequested()) {
            getGlobal()->reset();
            stop = true;
            break;
        }
        erased = false;
        auto maxsize = core.size();
        for (uint i = 0; i < maxsize;) {
            if (getGlobal()->terminateRequested()) {
                getGlobal()->reset();
                stop = true;
                break;
            }
            auto elem = core[i];

            //This serves to prevent self-swapping (Cf. Issue 739)
            if (not(core[i].symbol==core[maxsize - 1].symbol && core[i].args==core[maxsize - 1].args)) {
                std::swap(core[i], core[maxsize - 1]);
            }
            core.pop_back();
            maxsize--;
            auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, { core, { } });
            if (mxresult._interrupted) {
                stop = true;
                break;
            }
            if (not mxresult.unsat) {
                core.push_back(elem);
            } else {
                erased = true;
                if (mxresult.unsat_in_function_of_ct_lits.size() < core.size()) {
                    core = mxresult.unsat_in_function_of_ct_lits;
                    break;
                }
            }
        }
    }
    auto output = mxresult.unsat_in_function_of_ct_lits;
    return output;
}
