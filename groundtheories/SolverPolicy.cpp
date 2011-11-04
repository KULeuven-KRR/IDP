#include "grounders/LazyQuantGrounder.hpp"
#include "groundtheories/SolverPolicy.hpp"

using namespace std;

void SolverPolicy::notifyLazyResidual(ResidualAndFreeInst* inst, LazyQuantGrounder const* const grounder){
	LazyClauseMon* mon = new LazyClauseMon(inst);
	MinisatID::LazyGroundLit lc(false, createLiteral(inst->residual), mon);
	callbackgrounding cbmore(const_cast<LazyQuantGrounder*>(grounder), &LazyQuantGrounder::requestGroundMore);
	mon->setRequestMoreGrounding(cbmore);
	getSolver().add(lc);
}
