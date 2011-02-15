/************************************
	ground.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ground.hpp"
#include "ecnf.hpp"
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <limits> // numeric_limits

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

int GroundTranslator::translate(unsigned int n, const vector<domelement>& args) {
	map<vector<domelement>,int>::iterator jt = _table[n].lower_bound(args);
	if(jt != _table[n].end() && jt->first == args) {
		return jt->second;
	}
	else {
		int nr = nextNumber();
		_table[n].insert(jt,pair<vector<domelement>,int>(args,nr));
		_backsymbtable[nr] = _symboffsets[n];
		_backargstable[nr] = args;
		return nr;
	}
}

int GroundTranslator::translate(PFSymbol* s, const vector<TypedElement>& args) {
	unsigned int offset = addSymbol(s);
	vector<domelement> newargs(args.size());
	for(unsigned int n = 0; n < args.size(); ++n) {
		newargs[n] = CPPointer(args[n]);
	}
	return translate(offset,newargs);
}

int GroundTranslator::translate(const vector<int>& cl, bool conj, TsType tp) {
	int nr = nextNumber();
	TsBody& tb = _tsbodies[nr];
	tb._body = cl;
	tb._type = tp;
	tb._conj = conj;
	return nr;
}

int GroundTranslator::translateSet(const vector<int>& lits, const vector<double>& weights, const vector<double>& trueweights) {
	int setnr;
	if(_freesetnumbers.empty()) {
		GroundSet newset;
		setnr = _sets.size();
		_sets.push_back(newset);
		GroundSet& grset = _sets.back();

		grset._setlits = lits;
		grset._litweights = weights;
		grset._trueweights = trueweights;
	}
	else {
		setnr = _freesetnumbers.front();
		_freesetnumbers.pop();
		GroundSet& grset = _sets[setnr];

		grset._setlits = lits;
		grset._litweights = weights;
		grset._trueweights = trueweights;
	}
	return setnr;
}

int GroundTranslator::nextNumber() {
	if(_freenumbers.empty()) {
		int nr = _backsymbtable.size(); 
		_backsymbtable.push_back(0);
		_backargstable.push_back(vector<domelement>(0));
		return nr;
	}
	else {
		int nr = _freenumbers.front();
		_freenumbers.pop();
		return nr;
	}
}

unsigned int GroundTranslator::addSymbol(PFSymbol* pfs) {
	for(unsigned int n = 0; n < _symboffsets.size(); ++n)
		if(_symboffsets[n] == pfs) return n;
	_symboffsets.push_back(pfs);
	_table.push_back(map<vector<domelement>,int>());
	return _symboffsets.size()-1;
}


/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

void NaiveGrounder::visit(VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	TypedElement te = _varmapping[vt->var()];
	Element e = te._element;
	_returnTerm = new DomainTerm(vt->sort(),te._type,e,ParseInfo());
}

void NaiveGrounder::visit(DomainTerm* dt) {
	_returnTerm = dt->clone();
}

void NaiveGrounder::visit(FuncTerm* ft) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		_returnTerm = 0;
		ft->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnTerm = new FuncTerm(ft->func(),vt,ParseInfo());
}

void NaiveGrounder::visit(AggTerm* at) {
	_returnSet = 0;
	at->set()->accept(this);
	assert(_returnSet);
	_returnTerm = new AggTerm(_returnSet,at->type(),ParseInfo());
}

void NaiveGrounder::visit(EnumSetExpr* s) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		_returnFormula = 0;
		s->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	vector<Term*> vt;
	for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
		_returnTerm = 0;
		s->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
}

void NaiveGrounder::visit(QuantSetExpr* s) {
	SortTableTupleIterator stti(s->qvars(),_structure);
	vector<SortTable*> tables;
	vector<Formula*> vf(0);
	vector<Term*> vt(0);
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[s->qvar(n)] = e;
	}
	if(stti.empty()) {
		_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
	}
	else {
		ElementType weighttype = tables[0]->type();
		do {
			Element weight = stti.value(0);
			for(unsigned int n = 0; n < s->nrQvars(); ++n) {
				_varmapping[s->qvar(n)]._element = stti.value(n);
			}
			_returnFormula = 0;
			s->subf()->accept(this);
			assert(_returnFormula);
			vf.push_back(_returnFormula);
			vt.push_back(new DomainTerm(s->qvar(0)->sort(),weighttype,weight,ParseInfo()));
		} while(stti.nextvalue());
		_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
	}
}

void NaiveGrounder::visit(PredForm* pf) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		_returnTerm = 0;
		pf->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new PredForm(pf->sign(),pf->symb(),vt,FormParseInfo());
}

void NaiveGrounder::visit(EquivForm* ef) {
	_returnFormula = 0;
	ef->left()->accept(this);
	assert(_returnFormula);
	Formula* nl = _returnFormula;
	_returnFormula = 0;
	ef->right()->accept(this);
	assert(_returnFormula);
	Formula* nr = _returnFormula;
	_returnFormula = new EquivForm(ef->sign(),nl,nr,FormParseInfo());
}

void NaiveGrounder::visit(EqChainForm* ef) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_returnTerm = 0;
		ef->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new EqChainForm(ef->sign(),ef->conj(),vt,ef->comps(),ef->compsigns(),FormParseInfo());
}

void NaiveGrounder::visit(BoolForm* bf) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		_returnFormula = 0;
		bf->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	_returnFormula = new BoolForm(bf->sign(),bf->conj(),vf,FormParseInfo());
}

void NaiveGrounder::visit(QuantForm* qf) {
	SortTableTupleIterator stti(qf->qvars(),_structure);
	vector<Formula*> vf(0);
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[qf->qvar(n)] = e;
	}
	if(stti.empty()) {
		_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,FormParseInfo());
	}
	else {
		do {
			for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
				_varmapping[qf->qvar(n)]._element = stti.value(n);
			}
			_returnFormula = 0;
			qf->subf()->accept(this);
			assert(_returnFormula);
			vf.push_back(_returnFormula);
		} while(stti.nextvalue());
		_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,FormParseInfo());
	}
}

void NaiveGrounder::visit(Rule* r) {
	SortTableTupleIterator stti(r->qvars(),_structure);
	Definition* d = new Definition();
	for(unsigned int n = 0; n < r->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[r->qvar(n)] = e;
	}
	if(!stti.empty()) {
		do {
			for(unsigned int n = 0; n < r->nrQvars(); ++n) {
				_varmapping[r->qvar(n)]._element = stti.value(n);
			}
			Formula* nb;
			PredForm* nh;

			_returnFormula = 0;
			r->head()->accept(this);
			assert(_returnFormula);
			assert(typeid(*_returnFormula) == typeid(PredForm));
			nh = dynamic_cast<PredForm*>(_returnFormula);

			_returnFormula = 0;
			r->body()->accept(this);
			assert(_returnFormula);
			nb = _returnFormula;

			d->add(new Rule(vector<Variable*>(0),nh,nb,ParseInfo()));

		} while(stti.nextvalue());
	}
	_returnDef = d;
}

void NaiveGrounder::visit(Definition* d) {
	Definition* grounddef = new Definition();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrRules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnDef = grounddef;
}

void NaiveGrounder::visit(FixpDef* d) {
	FixpDef* grounddef = new FixpDef(d->lfp());
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		_returnFixpDef = 0;
		visit(d->def(n));
		assert(_returnFixpDef);
		grounddef->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrRules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnFixpDef = grounddef;
}

void NaiveGrounder::visit(Theory* t) {
	Theory* grounding = new Theory("",t->vocabulary(),ParseInfo());

	// Ground the theory
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_returnDef = 0;
		t->definition(n)->accept(this);
		assert(_returnDef);
		grounding->add(_returnDef);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_returnFixpDef = 0;
		t->fixpdef(n)->accept(this);
		assert(_returnFixpDef);
		grounding->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_returnFormula = 0;
		t->sentence(n)->accept(this);
		assert(_returnFormula);
		grounding->add(_returnFormula);
	}
	
	// Add the structure
	AbstractTheory* structtheo = StructUtils::convert_to_theory(_structure);
	grounding->add(structtheo);
	delete(structtheo);

	_returnTheory = grounding;
}

/*************************************
	Optimized grounding algorithm
*************************************/

int Grounder::_true = numeric_limits<int>::max();
int Grounder::_false = 0;

int TheoryGrounder::run() const {
	// Run formula grounders
	for(unsigned int n = 0; n < _formulagrounders.size(); ++n) {
		int result = _formulagrounders[n]->run();
		if(result == _false) {
			_grounding->addEmptyClause();
			return _false;
		}
	}
	//TODO: Run definition grounders
	return _true;
}

AtomGrounder::AtomGrounder(GroundTheory* gt, bool sign, bool sent, PFSymbol* s,
							const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, bool pc, bool c) :
	FormulaGrounder(gt), _sign(sign), _sentence(sent), _symbol(g->translator()->addSymbol(s)),
	_args(sg.size()), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic),
	_tables(vst), _poscontext(pc)
	{ _certainvalue = c ? _true : _false; }

int AtomGrounder::run() const {
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) 
		_args[n] = _subtermgrounders[n]->run();
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		//TODO: produce a warning!
		if(!ElementUtil::exists(_args[n])) {
			return _poscontext ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(!_tables[n]->contains(_args[n])) 
			return _sign ? _false : _true;
	}

	// Run instance checkers and return grounding
	if(!(_pchecker->run(_args))) return _certainvalue ? _false : _true;	// TODO: dit is lelijk
	if(_cchecker->run(_args)) return _certainvalue;
	else {
		int atom = _grounding->translator()->translate(_symbol,_args);
		if(!_sign) atom = -atom;
		if(_sentence) _grounding->addUnitClause(atom);
		return atom;
	}
}

inline bool ClauseGrounder::check1(int l) const {
	return _conj ? l == _false : l == _true;
}

inline bool ClauseGrounder::check2(int l) const {
	return _conj ? l == _true : l == _false;
}

inline int ClauseGrounder::result1() const {
	return (_conj == _sign) ? _false : _true;
}

inline int ClauseGrounder::result2() const {
	return (_conj == _sign) ? _true : _false;
}

int ClauseGrounder::finish(vector<int>& cl) const {
	if(cl.empty())
		return result2();
	else if(cl.size() == 1) {
		if(_sentence) {
			_grounding->addUnitClause(_sign ? cl[0] : -cl[0]);
			return _true;
		}
		else
			return _sign ? cl[0] : -cl[0];
	}
	else if(_sentence) {
		if(_conj == _sign) {
			for(unsigned int n = 0; n < cl.size(); ++n)
				_grounding->addUnitClause(_sign ? cl[n] : -cl[n]);
		}
		else {
			if(! _sign) {
				for(unsigned int n = 0; n < cl.size(); ++n)
					cl[n] = -cl[n];
			}
			_grounding->addClause(cl);
		}
		return _true;
	}	
	else {
		TsType tp = TS_IMPL;
		if(_poscontext != _sign) tp = TS_RIMPL;
		int ts = _grounding->translator()->translate(cl,_conj,tp);
		return _sign ? ts : -ts;
	}
}

int BoolGrounder::run() const {
	vector<int> cl;
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(check1(l)) return result1();
		else if(! check2(l)) cl.push_back(l);
	}
	return finish(cl);
}

int QuantGrounder::run() const {
	vector<int> cl;
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) return result1();
		else if(! check2(l)) cl.push_back(l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) return result1();
			else if(! check2(l)) cl.push_back(l);
		}
	}
	return finish(cl);
}

int EquivGrounder::run() const {
	// Run subgrounders
	int left = _leftgrounder->run();
	int right = _rightgrounder->run();

	if(left == right) return _sign ? _true : _false;
	else if(left == _true) {
		if(right == _false) return _sign ? _false : _true;
		else return _sign ? right : -right;
	}
	else if(left == _false) {
		if(right == _true) return _sign ? _false : _true;
		else return _sign ? -right : right;
	}
	else if(right == _true) return _sign ? left : -left;
	else if(right == _false) return _sign ? -left : left;
	else if(_sign) {
		EcnfClause cl1(2);
		EcnfClause cl2(2);
		cl1[0] = left;	cl1[1] = -right;
		cl2[0] = -left;	cl2[1] = right;
		if(_sentence) {
			_grounding->addClause(cl1);
			_grounding->addClause(cl2);
			return _true;
		}
		else {
			EcnfClause tcl(2);
			TsType tp = _poscontext ? TS_IMPL : TS_RIMPL; 
			int ts1 = _grounding->translator()->translate(cl1,false,tp);
			int ts2 = _grounding->translator()->translate(cl2,false,tp);
			tcl[0] = ts1; tcl[1] = ts2;
			int ts = _grounding->translator()->translate(tcl,true,tp);
			return ts;
		}
	}
	else { // _sign = false
		EcnfClause cl1(2);
		EcnfClause cl2(2);
		cl1[0] = left;	cl1[1] = -right;
		cl2[0] = -left;	cl2[1] = right;
		EcnfClause tcl(2);
		TsType tp = _poscontext ? TS_IMPL : TS_RIMPL; 
		int ts1 = _grounding->translator()->translate(cl1,true,tp);
		int ts2 = _grounding->translator()->translate(cl2,true,tp);
		tcl[0] = ts1; tcl[1] = ts2;
		if(_sentence) {
			_grounding->addClause(tcl);
			return _true;
		}
		else {
			int ts = _grounding->translator()->translate(tcl,false,tp);
			return ts;
		}
	}
}

domelement FuncTermGrounder::run() const {
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
	}
	//_result->_type = _function->type(_function->arity());
	//_result->_element = (*_function)[_args];
	return (*_function)[_args];
/*	if(!_table->contains(*_result)) {
		_result->_element = ElementUtil::nonexist(_result->_type);
	}*/
}

domelement AggTermGrounder::run() const {
	int setnr = _setgrounder->run();
	const GroundSet& grs = _grounding->translator()->groundset(setnr);
	assert(grs._setlits.empty());
	double value;
	switch(_type) {
		case AGGCARD:
		{
			Element e; e._int = grs._trueweights.size();
			return CPPointer(e,ELINT);
		}
		case AGGSUM:
			value = 0;
			for(unsigned int n = 0; n < grs._trueweights.size(); ++n) 
				value += grs._trueweights[n];
			break;
		case AGGPROD:
			value = 1;
			for(unsigned int n = 0; n < grs._trueweights.size(); ++n) 
				value *= grs._trueweights[n];
			break;
		case AGGMIN:
			value = numeric_limits<double>::max();
			for(unsigned int n = 0; n < grs._trueweights.size(); ++n) 
				value = grs._trueweights[n] < value ? grs._trueweights[n] : value;
			break;
		case AGGMAX:
			value = numeric_limits<double>::min();
			for(unsigned int n = 0; n < grs._trueweights.size(); ++n) 
				value = grs._trueweights[n] > value ? grs._trueweights[n] : value;
			break;
	}

	Element e;
	if(isInt(value)) {
		e._int = int(value);
		return CPPointer(e,ELINT);
	}
	else {
		e._double = value;
		return CPPointer(e,ELDOUBLE);
	}
}

int EnumSetGrounder::run() const {
	vector<int>	literals;
	vector<double> weights;
	vector<double> trueweights;
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(l != _false) {
			domelement d =  _subtermgrounders[n]->run();
			Element e; e._compound = d;
			Element w = ElementUtil::convert(e,ELCOMPOUND,ELDOUBLE);
			if(l == _true) trueweights.push_back(w._double);
			else {
				weights.push_back(w._double);
				literals.push_back(l);
			}
		}
	}
	int s = _grounding->translator()->translateSet(literals,weights,trueweights);
	return s;
}

int QuantSetGrounder::run() const {
	vector<int> literals;
	vector<double> weights;
	vector<double> trueweights;
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(l != _false) {
			Element e; e._compound = *_weight;
			Element w = ElementUtil::convert(e,ELCOMPOUND,ELDOUBLE);
			if(l == _true) trueweights.push_back(w._double);
			else {
				weights.push_back(w._double);
				literals.push_back(l);
			}
		}
		while(_generator->next()) {
			l = _subgrounder->run();
			if(l != _false) {
				Element e; e._compound = *_weight;
				Element w = ElementUtil::convert(e,ELCOMPOUND,ELDOUBLE);
				if(l == _true) trueweights.push_back(w._double);
				else {
					weights.push_back(w._double);
					literals.push_back(l);
				}
			}
		}
	}
	int s = _grounding->translator()->translateSet(literals,weights,trueweights);
	return s;
}

bool RuleGrounder::run() const {
	//TODO
	if(_generator->first()) {	
		int headlit = _headgrounder->run();
		int bodylit = _bodygrounder->run();
		while(_generator->next()) {
			int headlit = _headgrounder->run();
			int bodylit = _bodygrounder->run();
		}
	}
	//TODO return succes?
}

int DefinitionGrounder::run() const {
	for(unsigned int n = 0; n < subgrounders.size(); ++n)
		subgrounder[n]->run();
	//TODO: return _grounding->translator()->translateDefinition(_definition) ??
}


/******************************
	GrounderFactory methods
******************************/

AbstractTheoryGrounder* GrounderFactory::create(AbstractTheory* theory) {
	// Allocate an ecnf theory to be returned by the grounder
	_grounding = new EcnfTheory(theory->vocabulary());
	// Create grounder
	theory->accept(this);
	return _theogrounder;
}

AbstractTheoryGrounder* GrounderFactory::create(AbstractTheory* theory, SATSolver* solver) {
	// Allocate a solver theory
	_grounding = new SolverTheory(theory->vocabulary(), solver);
	// Create grounder
	theory->accept(this);
	return _theogrounder;
}

void GrounderFactory::visit(EcnfTheory* ecnf) {
	_theogrounder = new EcnfGrounder(ecnf);		// TODO: add the structure?
}

void GrounderFactory::visit(Theory* theory) {
	// Collect components of the theory
	vector<TheoryComponent*> components(theory->nrComponents());
	for(unsigned int n = 0; n < theory->nrComponents(); ++n) {
		components[n] = theory->component(n);
	}

	// TODO (OPTIMIZATION) order components

	// Create grounders for the components
	vector<FormulaGrounder*> children(components.size());
	for(unsigned int n = 0; n < components.size(); ++n) {
		_poscontext = true;
		_truegencontext = false;
		_sentence = true;
		components[n]->accept(this);
		children[n] = _grounder; 
	}

	// Create grounder
	_theogrounder = new TheoryGrounder(_grounding,children);
}

void GrounderFactory::visit(PredForm* pf) {
	// Check if each of the arguments of pf evaluates to a single value 
	PredForm* newpf = pf->clone();
	Formula* transpf = newpf; // TODO: replace this by the appropriate rewriting

	if(newpf == transpf) {	// all arguments indeed evaluate to a single value
		// Save context
		bool posc = _poscontext;
		bool truegc = _truegencontext;
		bool sent = _sentence;

		// Create grounders for the subterms
		vector<TermGrounder*> vtg;
		vector<SortTable*>	  vst;
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
			pf->subterm(n)->accept(this);
			if(_termgrounder) vtg.push_back(_termgrounder);
			vst.push_back(_structure->inter(pf->symb()->sort(n)));
		}

		// Restore context
		_poscontext = posc;
		_truegencontext = truegc;
		_sentence = sent;

		// create checker
		InstanceChecker* pch;
		InstanceChecker* cch;
		PredInter* inter = _structure->inter(pf->symb());
		if(_truegencontext == pf->sign()) {		// check according to ct-table
			if(inter->cf()) {
				if(inter->cfpt()->empty()) pch = new TrueInstanceChecker();
				else pch = new InvTableInstanceChecker(inter->cfpt());
			}
			else {
				if(inter->cfpt()->empty()) pch = new FalseInstanceChecker();
				else pch = new TableInstanceChecker(inter->cfpt());
			}

			if(inter->ct()) {
				if(inter->ctpf()->empty()) cch = new FalseInstanceChecker();
				else cch = new TableInstanceChecker(inter->ctpf());
			}
			else {
				if(inter->ctpf()->empty()) cch = new TrueInstanceChecker();
				else cch = new InvTableInstanceChecker(inter->ctpf());
			}
		}
		else {	// check according to cf-table
			if(inter->ct()) {
				if(inter->ctpf()->empty()) pch = new TrueInstanceChecker();
				else pch = new InvTableInstanceChecker(inter->ctpf());
			}
			else {
				if(inter->ctpf()->empty()) pch = new FalseInstanceChecker();
				else pch = new TableInstanceChecker(inter->ctpf());
			}

			if(inter->cf()) {
				if(inter->cfpt()->empty()) cch = new FalseInstanceChecker();
				else cch = new TableInstanceChecker(inter->cfpt());
			}
			else {
				if(inter->cfpt()->empty()) cch = new TrueInstanceChecker();
				else cch = new InvTableInstanceChecker(inter->cfpt());
			}
		}

		// Create the grounder
		_grounder = new AtomGrounder(_grounding,pf->sign(),_sentence,pf->symb(),vtg,pch,cch,vst,_poscontext,_truegencontext);
	}
	else {
		transpf->accept(this);
	}

	// TODO: delete temporary values

}

void GrounderFactory::visit(BoolForm* bf) {
	// Save context
	bool pos = _poscontext;
	bool gen = _truegencontext;
	bool sent = _sentence;

	// Create grounders for subformulas
	vector<FormulaGrounder*> sub(bf->nrSubforms());
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		_sentence = false;
		_poscontext = bf->sign() ? pos : !pos;
		_truegencontext = bf->sign() ? gen : !gen;
		bf->subform(n)->accept(this);
		sub[n] = _grounder;
	}

	// Restore context
	_sentence = sent;
	_poscontext = pos;
	_truegencontext = gen;

	// Create grounder
	_grounder = new BoolGrounder(_grounding,sub,bf->sign(),sent,bf->conj(),_poscontext);
}

void GrounderFactory::visit(QuantForm* qf) {
	// Save context
	bool pos = _poscontext;
	bool tgen = _truegencontext;
	bool sent = _sentence;

	// Create instance generator
	InstGenerator* gen = 0;
	GeneratorNode* node = 0;
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		domelement* d = new domelement();
		_varmapping[qf->qvar(n)] = d;
		SortTable* st = _structure->inter(qf->qvar(n)->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(qf->nrQvars() == 1) {
			gen = tig;
			break;
		}
		else if(n == 0)
			node = new LeafGeneratorNode(tig);
		else 
			node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	
	// Create grounder for subformula
	if(!(qf->univ())) _sentence = false;
	_poscontext = qf->sign() ? pos : !pos;
	_truegencontext = !(qf->univ()); 
	qf->subf()->accept(this);
	FormulaGrounder* sub = _grounder;

	// Restore context
	_sentence = sent;
	_poscontext = pos;
	_truegencontext = tgen;

	// Create grounder
	_grounder = new QuantGrounder(_grounding,sub,qf->sign(),_sentence,qf->univ(),_poscontext,gen);
}

void GrounderFactory::visit(EquivForm* ef) {
	//TODO: check for partial functions

	// Save context
	bool pos = _poscontext;
	bool tgen = _truegencontext;
	bool sent = _sentence;

	// Create grounder for left subformula
	_sentence = false;
	ef->left()->accept(this);
	FormulaGrounder* leftg = _grounder;

	// Restore context before going right
	_poscontext = pos;
	_truegencontext = tgen;
	_sentence = sent;

	// Create grounder for right subformula
	_sentence = false;
	ef->right()->accept(this);
	FormulaGrounder* rightg = _grounder;
	
	// Restore context
	_poscontext = pos;
	_truegencontext = tgen;
	_sentence = sent;

	// Create grounder
	_grounder = new EquivGrounder(_grounding,leftg,rightg,ef->sign(),_sentence,_poscontext);
}

void GrounderFactory::visit(EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::remove_eqchains(f,_grounding->vocabulary());
	f->accept(this);
	f->recursiveDelete();
}

void GrounderFactory::visit(VarTerm* t) {
	// Create termgrounder
	_termgrounder = new VarTermGrounder(_varmapping.find(t->var())->second);
}

void GrounderFactory::visit(DomainTerm* t) {
	// Create termgrounder
	_termgrounder = new DomTermGrounder(CPPointer(t->value(),t->type()));

	// Check whether value is within bounds of the current position.
//	if(_structure->inter(_currsort)->contains(t->value(),t->type()))
//		_value = new TypedElement(t->value(),t->type());
//	else {
//		_value = new TypedElement();
//		_value->_type = t->type();
//		_value->_element = ElementUtil::nonexist(t->type());
//	}
}

void GrounderFactory::visit(FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> sub;
	for(unsigned int n = 0; n < t->nrSubterms(); ++n) {
		t->subterm(n)->accept(this);
		if(_termgrounder) sub.push_back(_termgrounder);
	}

	// Create term grounder
	FuncTable* ft = _structure->inter(t->func())->functable();
	_termgrounder = new FuncTermGrounder(sub,ft);
}

void GrounderFactory::visit(AggTerm* t) {
	// Create set grounder
	t->set()->accept(this);

	// Create term grounder
	_termgrounder = new AggTermGrounder(_grounding,t->type(),_setgrounder);
}

void GrounderFactory::visit(EnumSetExpr* s) {
	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subgr;
	vector<TermGrounder*> subtgr;
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		_sentence = false;
		_poscontext = true;
		_truegencontext = true;
		s->subform(n)->accept(this);
		subgr.push_back(_grounder);
		s->subterm(n)->accept(this);
		subtgr.push_back(_termgrounder);
	}

	// Create set grounder
	_setgrounder = new EnumSetGrounder(_grounding,subgr,subtgr);
}

void GrounderFactory::visit(QuantSetExpr* s) {
	// Create instance generator
	InstGenerator* gen = 0;
	GeneratorNode* node = 0;
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		domelement* d = new domelement();
		_varmapping[s->qvar(n)] = d;
		SortTable* st = _structure->inter(s->qvar(n)->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(s->nrQvars() == 1) {
			gen = tig;
			break;
		}
		else if(n == 0)
			node = new LeafGeneratorNode(tig);
		else 
			node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	
	// Create grounder for subformula
	_sentence = false;
	_poscontext = true;
	_truegencontext = true; 
	s->subf()->accept(this);
	FormulaGrounder* sub = _grounder;

	// Create grounder	
	_setgrounder = new QuantSetGrounder(_grounding,sub,gen,_varmapping[s->qvar(0)]);
}

void GrounderFactory::visit(Definition* def) {
	// Create new ground definition
	_definition = new EcnfDefinition();

	// Create rule grounders
	vector<RuleGrounder*> subgr;
	for(unsigned int n = 0; n < def->nrRules(); ++n) {
		def->rule(n)->accept(this);
		subgr.push_back(_rulegrounder);
	}
	
	// Create definition grounder
	_defgrounder = new DefinitionGrounder(_grounding,_definition,subgr);
}

void GrounderFactory::visit(Rule* rule) {
	// Create instance generator
	InstGenerator* gen = 0;
	GeneratorNode* node = 0;
	for(unsigned int n = 0; n < rule->nrQvars(); ++n) {
		domelement* d = new domelement();
		_varmapping[rule->qvar(n)] = d;
		SortTable* st = _structure->inter(rule->qvar(n)->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(rule->nrQvars() == 1) {
			gen = tig;
			break;
		}
		else if(n == 0)
			node = new LeafGeneratorNode(tig);
		else 
			node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	
	// Create head grounder
	_sentence = false;
	_poscontext = true;
	_truegencontext = true;
	_rulecontext = true;
	rule->head()->visit(this);
	FormulaGrounder* hgr = _grounder;

	// Create body grounder
	_sentence = false;
	_poscontext = true;
	_truegencontext = true;
	_rulecontext = true;
	rule->body()->visit(this);
	FormulaGrounder* bgr = _grounder;

	// Create rule grounder
	_rulegrounder = new RuleGrounder(_grounding,_definition,hgr,bgr,gen);
}
