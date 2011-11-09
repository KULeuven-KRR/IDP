/************************************
	Distributivity.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DISTRIBUTIVITY_HPP_
#define DISTRIBUTIVITY_HPP_

/**
 * Class to exhaustively distribute addition with respect to multiplication in a FOBDDFuncTerm
 */

class Distributivity : public FOBDDVisitor {
	public:
		Distributivity(FOBDDManager* m) : FOBDDVisitor(m) { }

		const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
			if(functerm->func()->name() == "*/2") {
				const FOBDDArgument* leftterm = functerm->args(0);
				const FOBDDArgument* rightterm = functerm->args(1);
				if(typeid(*leftterm) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* leftfuncterm = dynamic_cast<const FOBDDFuncTerm*>(leftterm);
					if(leftfuncterm->func()->name() == "+/2") {
						vector<const FOBDDArgument*> newleftargs;
						newleftargs.push_back(leftfuncterm->args(0));
						newleftargs.push_back(rightterm);
						vector<const FOBDDArgument*> newrightargs;
						newrightargs.push_back(leftfuncterm->args(1));
						newrightargs.push_back(rightterm);
						const FOBDDArgument* newleft = _manager->getFuncTerm(functerm->func(),newleftargs);
						const FOBDDArgument* newright = _manager->getFuncTerm(functerm->func(),newrightargs);
						vector<const FOBDDArgument*> newargs;
						newargs.push_back(newleft);
						newargs.push_back(newright);
						const FOBDDArgument* newterm = _manager->getFuncTerm(leftfuncterm->func(),newargs);
						return newterm->acceptchange(this);
					}
				}
				if(typeid(*rightterm) == typeid(FOBDDFuncTerm)) {
					const FOBDDFuncTerm* rightfuncterm = dynamic_cast<const FOBDDFuncTerm*>(rightterm);
					if(rightfuncterm->func()->name() == "+/2") {
						vector<const FOBDDArgument*> newleftargs;
						newleftargs.push_back(rightfuncterm->args(0));
						newleftargs.push_back(leftterm);
						vector<const FOBDDArgument*> newrightargs;
						newrightargs.push_back(rightfuncterm->args(1));
						newrightargs.push_back(leftterm);
						const FOBDDArgument* newleft = _manager->getFuncTerm(functerm->func(),newleftargs);
						const FOBDDArgument* newright = _manager->getFuncTerm(functerm->func(),newrightargs);
						vector<const FOBDDArgument*> newargs;
						newargs.push_back(newleft);
						newargs.push_back(newright);
						const FOBDDArgument* newterm = _manager->getFuncTerm(rightfuncterm->func(),newargs);
						return newterm->acceptchange(this);
					}
				}
			}
			return FOBDDVisitor::change(functerm);
		}
};

#endif /* DISTRIBUTIVITY_HPP_ */
