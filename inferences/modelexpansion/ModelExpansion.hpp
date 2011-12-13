#ifndef MODELEXPANSION_HPP_
#define MODELEXPANSION_HPP_

#include <vector>

class AbstractStructure;
class AbstractTheory;
class TraceMonitor;


class ModelExpansion {
public:
        static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor){
                ModelExpansion m(theory, structure, tracemonitor);
                return m.expand();
        }

private:
        AbstractTheory* theory;
        AbstractStructure* structure;
        TraceMonitor* tracemonitor;
        ModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor):
                        theory(theory), structure(structure), tracemonitor(tracemonitor){

        }
        std::vector<AbstractStructure*> expand() const;

};
#endif //MODELEXPANSION_HPP_
