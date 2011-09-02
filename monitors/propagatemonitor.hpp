/************************************
	luatracemonitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PROPAGATEMONITOR_HPP_
#define PROPAGATEMONITOR_HPP_

#include "monitors/tracemonitor.hpp"
#include "external/SearchMonitor.hpp"

class PropagateMonitor : public TraceMonitor {
	private:
		std::vector<MinisatID::Literal>	_partialmodel;
		MinisatID::SearchMonitor*		_solvermonitor;
	public:
		PropagateMonitor() {
			cb::Callback2<void, MinisatID::Literal, int> callbackprop(this, &PropagateMonitor::propagate);
			MinisatID::SearchMonitor* solvermonitor_ = new MinisatID::SearchMonitor();
			solvermonitor_->setPropagateCB(callbackprop);
		}
		~PropagateMonitor() { }

		void backtrack(int )								{ assert(false);							}
		void propagate(MinisatID::Literal lit, int )		{ _partialmodel.push_back(lit);				}
		std::string* index() const							{ assert(false); return 0;					}
		void setTranslator(GroundTranslator* )				{ assert(false);							}
		void setSolver(SATSolver* solver)					{ solver->addMonitor(_solvermonitor);		}
		const std::vector<MinisatID::Literal>& model()		{ return _partialmodel;						}
};

#endif /* PROPAGATEMONITOR_HPP_ */