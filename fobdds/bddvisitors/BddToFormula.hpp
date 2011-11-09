/************************************
	BddToFormula.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BDDTOFORMULA_HPP_
#define BDDTOFORMULA_HPP_

class BDDToFormula : public FOBDDVisitor {
	private:
		Formula* _currformula;
		Term* _currterm;
		map<const FOBDDDeBruijnIndex*,Variable*> _dbrmapping;

		void visit(const FOBDDDeBruijnIndex* index) {
			auto it = _dbrmapping.find(index);
			Variable* v;
			if(it == _dbrmapping.cend()) {
				Variable* v = new Variable(index->sort());
				_dbrmapping[index] = v;
			}
			else { v = it->second; }
			_currterm = new VarTerm(v,TermParseInfo());
		}

		void visit(const FOBDDVariable* var) {
			_currterm = new VarTerm(var->variable(),TermParseInfo());
		}

		void visit(const FOBDDDomainTerm* dt) {
			_currterm = new DomainTerm(dt->sort(),dt->value(),TermParseInfo());
		}

		void visit(const FOBDDFuncTerm* ft) {
			vector<Term*> args;
			for(auto it = ft->args().cbegin(); it != ft->args().cend(); ++it) {
				(*it)->accept(this);
				args.push_back(_currterm);
			}
			_currterm = new FuncTerm(ft->func(),args,TermParseInfo());
		}

		void visit(const FOBDDAtomKernel* atom) {
			vector<Term*> args;
			for(auto it = atom->args().cbegin(); it != atom->args().cend(); ++it) {
				(*it)->accept(this);
				args.push_back(_currterm);
			}
			switch(atom->type()) {
				case AKT_TWOVAL:
					_currformula = new PredForm(SIGN::POS,atom->symbol(),args,FormulaParseInfo());
					break;
				case AKT_CT:
					_currformula = new PredForm(SIGN::POS,atom->symbol()->derivedSymbol(ST_CT),args,FormulaParseInfo());
					break;
				case AKT_CF:
					_currformula = new PredForm(SIGN::POS,atom->symbol()->derivedSymbol(ST_CF),args,FormulaParseInfo());
					break;
			}
		}

		void visit(const FOBDDQuantKernel* quantkernel) {
			map<const FOBDDDeBruijnIndex*,Variable*> savemapping = _dbrmapping;
			_dbrmapping.clear();
			for(auto it = savemapping.cbegin(); it != savemapping.cend(); ++it)
				_dbrmapping[_manager->getDeBruijnIndex(it->first->sort(),it->first->index()+1)] = it->second;
			FOBDDVisitor::visit(quantkernel->bdd());
			set<Variable*> quantvars;
			quantvars.insert(_dbrmapping[_manager->getDeBruijnIndex(quantkernel->sort(),0)]);
			_dbrmapping = savemapping;
			_currformula = new QuantForm(SIGN::POS,QUANT::EXIST,quantvars,_currformula,FormulaParseInfo());
		}

		void visit(const FOBDD* bdd) {
			if(_manager->isTruebdd(bdd)) { _currformula =  FormulaUtils::trueFormula(); }
			else if(_manager->isFalsebdd(bdd)) { _currformula = FormulaUtils::falseFormula(); }
			else {
				bdd->kernel()->accept(this);
				if(_manager->isFalsebdd(bdd->falsebranch())) {
					if(not _manager->isTruebdd(bdd->truebranch())) {
						Formula* kernelform = _currformula;
						FOBDDVisitor::visit(bdd->truebranch());
						_currformula = new BoolForm(SIGN::POS,true,kernelform,_currformula,FormulaParseInfo());
					}
				}
				else if(_manager->isFalsebdd(bdd->truebranch())) {
					_currformula->negate();
					if(not _manager->isTruebdd(bdd->falsebranch())) {
						Formula* kernelform = _currformula;
						FOBDDVisitor::visit(bdd->falsebranch());
						_currformula = new BoolForm(SIGN::POS,true,kernelform,_currformula,FormulaParseInfo());
					}
				}
				else {
					Formula* kernelform = _currformula;
					Formula* negkernelform = kernelform->clone(); negkernelform->negate();
					if(_manager->isTruebdd(bdd->falsebranch())) {
						FOBDDVisitor::visit(bdd->truebranch());
						BoolForm* bf = new BoolForm(SIGN::POS,true,kernelform,_currformula,FormulaParseInfo());
						_currformula = new BoolForm(SIGN::POS,false,negkernelform,bf,FormulaParseInfo());
					}
					else if(_manager->isTruebdd(bdd->truebranch())) {
						FOBDDVisitor::visit(bdd->falsebranch());
						BoolForm* bf = new BoolForm(SIGN::POS,true,negkernelform,_currformula,FormulaParseInfo());
						_currformula = new BoolForm(SIGN::POS,false,kernelform,bf,FormulaParseInfo());
					}
					else {
						FOBDDVisitor::visit(bdd->truebranch());
						Formula* trueform = _currformula;
						FOBDDVisitor::visit(bdd->falsebranch());
						Formula* falseform = _currformula;
						BoolForm* bf1 = new BoolForm(SIGN::POS,true,kernelform,trueform,FormulaParseInfo());
						BoolForm* bf2 = new BoolForm(SIGN::POS,true,negkernelform,falseform,FormulaParseInfo());
						_currformula = new BoolForm(SIGN::POS,false,bf1,bf2,FormulaParseInfo());
					}
				}
			}
		}

	public:
		BDDToFormula(FOBDDManager* m) : FOBDDVisitor(m) { }
		Formula* run(const FOBDDKernel* kernel) {
			kernel->accept(this);
			return _currformula;
		}
		Formula* run(const FOBDD* bdd) {
			FOBDDVisitor::visit(bdd);
			return _currformula;
		}
		Term* run(const FOBDDArgument* arg) {
			arg->accept(this);
			return _currterm;
		}
};


#endif /* BDDTOFORMULA_HPP_ */
