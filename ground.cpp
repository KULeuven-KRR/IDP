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

/** The two built-in literals 'true' and 'false' **/
int _true = numeric_limits<int>::max();
int _false = 0;

bool EcnfGrounder::run() const {
	// TODO TODO TODO
	return true;
}

bool TheoryGrounder::run() const {
	for(unsigned int n = 0; n < _grounders.size(); ++n) {
		bool b = _grounders[n]->run();
		if(!b) return b;
	}
	return true;
}

bool SentenceGrounder::run() const {
	vector<int> cl;
	_subgrounder->run(cl);
	if(cl.empty()) return _conj ? true : false;
	else if(cl.size() == 1) {
		if(cl[0] == _false) {
			_grounding->addEmptyClause();
			return false;
		}
		else if(cl[0] != _true) {
			_grounding->addClause(cl);
			return true;
		}
		else return true;
	}
	else {
		if(_conj) {
			_grounding->addClause(cl);
		}
		else {
			for(unsigned int n = 0; n < cl.size(); ++n)
				_grounding->addUnitClause(cl[n]);
		}
		return true;
	}
}

bool UnivSentGrounder::run() const {
	if(_generator->first()) {
		bool b = _subgrounder->run();
		if(!b) return b;
		while(_generator->next()) {
			b = _subgrounder->run();
			if(!b) return b;
		}
	}
	return true;
}

AtomGrounder::AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
							const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, const GroundingContext& ct) :
	FormulaGrounder(gt,ct), _sign(sign), _symbol(gt->addSymbol(s)),
	_args(sg.size()), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic),
	_tables(vst)
	{ _certainvalue = ct._truegen ? _true : _false; }

int AtomGrounder::run() const {
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) 
		_args[n] = _subtermgrounders[n]->run();
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		if(!ElementUtil::exists(_args[n])) {
			//TODO: produce a warning!
			if(_context._positive == PC_BOTH) {
				// TODO: produce an error
			}
			return _context._positive != PC_NEGATIVE  ? _true : _false;
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
		int atom = _translator->translate(_symbol,_args);
		if(!_sign) atom = -atom;
		return atom;
	}
}

void AtomGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
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
		/*if(_sentence) {
			_grounding->addUnitClause(_sign ? cl[0] : -cl[0]);
			return _true;
		}*/
		//else
			return _sign ? cl[0] : -cl[0];
	}
/*	else if(_sentence) {
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
	}	*/
	else {
		TsType tp = _context._tseitin;
		if(!_sign) {
			if(tp == TS_IMPL) tp = TS_RIMPL;
			else if(tp == TS_RIMPL) tp = TS_IMPL;
		}
		int ts = _translator->translate(cl,_conj,tp);
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

void BoolGrounder::run(vector<int>& clause) const {
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
			return;
		}
		else if(!check2(l)) clause.push_back(_sign ? l : -l);
	}
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

void QuantGrounder::run(vector<int>& clause) const {
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
			return;
		}
		else if(! check2(l)) clause.push_back(_sign ? l : -l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
				clause.clear();
				clause.push_back(result1());
				return;
			}
			else if(! check2(l)) clause.push_back(_sign ? l : -l);
		}
	}
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
	else {
		EcnfClause cl1(2);
		EcnfClause cl2(2);
		cl1[0] = left;	cl1[1] = _sign ? -right : right;
		cl2[0] = -left;	cl2[1] = _sign ? right : -right;
		EcnfClause tcl(2);
		TsType tp = _context._tseitin;
		int ts1 = _translator->translate(cl1,false,tp);
		int ts2 = _translator->translate(cl2,false,tp);
		tcl[0] = ts1; tcl[1] = ts2;
		int ts = _translator->translate(tcl,true,tp);
		return ts;
	}
}

void EquivGrounder::run(vector<int>& clause) const {
	// Run subgrounders
	int left = _leftgrounder->run();
	int right = _rightgrounder->run();

	if(left == right) { clause.push_back(_sign ? _true : _false); return;	}
	else if(left == _true) {
		if(right == _false) { clause.push_back(_sign ? _false : _true); return; }
		else { clause.push_back(_sign ? right : -right); return; }
	}
	else if(left == _false) {
		if(right == _true) { clause.push_back(_sign ? _false : _true); return; }
		else { clause.push_back(_sign ? -right : right); return; }
	}
	else if(right == _true) { clause.push_back(_sign ? left : -left); return; }
	else if(right == _false) { clause.push_back(_sign ? -left : left); return; }
	else {
		EcnfClause cl1(2);
		EcnfClause cl2(2);
		cl1[0] = left;	cl1[1] = _sign ? -right : right;
		cl2[0] = -left;	cl2[1] = _sign ? right : -right;
		TsType tp = _context._tseitin;
		int ts1 = _translator->translate(cl1,false,tp);
		int ts2 = _translator->translate(cl2,false,tp);
		clause.push_back(ts1); clause.push_back(ts2);
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
	const GroundSet& grs = _translator->groundset(setnr);
	assert(grs._setlits.empty());
	double value = AggUtils::compute(_type,grs._trueweights);
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
	int s = _translator->translateSet(literals,weights,trueweights);
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
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

HeadGrounder::HeadGrounder(GroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
			const vector<TermGrounder*>& sg, const vector<SortTable*>& vst) :
	_grounding(gt), _subtermgrounders(sg), _truechecker(pc), _falsechecker(cc), _symbol(gt->translator()->addSymbol(s)),
	_args(sg.size()), _tables(vst) { }

int HeadGrounder::run() const {
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) 
		_args[n] = _subtermgrounders[n]->run();
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...!
		//TODO: produce a warning!
		if(!ElementUtil::exists(_args[n])) return _false;
		if(!_tables[n]->contains(_args[n])) return _false;
	}

	// Run instance checkers and return grounding
	int atom = _grounding->translator()->translate(_symbol,_args);
	if(_truechecker->run(_args)) {
		_grounding->addUnitClause(atom);
	}
	else if(_falsechecker->run(_args)) {
		_grounding->addUnitClause(-atom);
	}
	return atom;
}

bool RuleGrounder::run() const {
	if(_bodygenerator->first()) {	
		vector<int>	body;
		_bodygrounder->run(body);
		bool falsebody = (body.empty() && !_conj) || (body.size() == 1 && body[0] == _false);
		if(!falsebody) {
			if(_headgenerator->first()) {
				int h = _headgrounder->run();
				assert(h != _true);
				if(h != _false) {
					_definition->addRule(h,body,_conj,_recursive);
				}
				while(_headgenerator->next()) {
					h = _headgrounder->run();
					assert(h != _true);
					if(h != _false) {
						_definition->addRule(h,body,_conj,_recursive);
					}
				}
			}
		}
		while(_headgenerator->next()) {
			body.clear();
			_bodygrounder->run(body);
			bool falsebody = (body.empty() && !_conj) || (body.size() == 1 && body[0] == _false);
			if(!falsebody) {
				if(_headgenerator->first()) {
					int h = _headgrounder->run();
					assert(h != _true);
					if(h != _false) {
						_definition->addRule(h,body,_conj,_recursive);
					}
					while(_headgenerator->next()) {
						h = _headgrounder->run();
						assert(h != _true);
						if(h != _false) {
							_definition->addRule(h,body,_conj,_recursive);
						}
					}
				}
			}
		}
	}
	return true;
}

bool DefinitionGrounder::run() const {
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		bool b = _subgrounders[n]->run();
		if(!b) return false;
	}
	_grounding->addDefinition(*_definition);
	return true;
}


/******************************
	GrounderFactory methods
******************************/

/*
 * void GrounderFactory::InitContext() 
 * DESCRIPTION
 *		Initializes the context of the GrounderFactory before visiting a sentence
 */
void GrounderFactory::InitContext() {
	_context._truegen		= false;
	_context._positive		= PC_POSITIVE;
	_context._component		= CC_SENTENCE;
	_context._tseitin		= TS_IMPL;
}

/*
 *	void GrounderFactory::SaveContext() 
 *	DESCRIPTION
 *		Pushes the current context on a stack 
 */
void GrounderFactory::SaveContext() {
	_contextstack.push(_context);
}

/*
 * void GrounderFactory::RestoreContext()
 * DESCRIPTION
 *		Restores the context to the top of the stack and pops the stack
 */
void GrounderFactory::RestoreContext() {
	_context = _contextstack.top();
	_contextstack.pop();
}

/*
 * void GrounderFactory::DeeperContext(bool sign)
 * DESCRIPTION
 *		Adapts the context to go one level deeper, and inverting some values if sign is negative
 * PARAMETERS
 *		sign	- the sign
 */
void GrounderFactory::DeeperContext(bool sign) {
	// One level deeper
	if(_context._component == CC_SENTENCE) _context._component = CC_FORMULA;
	// Swap positive, truegen and tseitin according to sign
	if(!sign) {

		_context._truegen = !_context._truegen;

		if(_context._positive == PC_POSITIVE) _context._positive = PC_NEGATIVE;
		else if(_context._positive == PC_NEGATIVE) _context._positive = PC_POSITIVE;

		if(_context._tseitin == TS_IMPL) _context._tseitin = TS_RIMPL;
		else if(_context._tseitin == TS_RIMPL) _context._tseitin = TS_IMPL;

	}
}

/*
 * void GrounderFactory::descend(Term* t)
 * DESCRIPTION
 *		Visits term a term and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		t	- the visited term
 */
void GrounderFactory::descend(Term* t) {
	SaveContext();
	t->accept(this);
	RestoreContext();
}

/*
 * void GrounderFactory::descend(Formula* f)
 * DESCRIPTION
 *		Visits term a formula and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		f	- the visited formula
 */
void GrounderFactory::descend(Formula* f) {
	SaveContext();
	f->accept(this);
	RestoreContext();
}

/*
 * TopLevelGrounder* GrounderFactory::create(AbstractTheory* theory)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is not passed to a solver, but stored internally as a EcnfTheory.
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 */
TopLevelGrounder* GrounderFactory::create(AbstractTheory* theory) {

	// Allocate an ecnf theory to be returned by the grounder
	_grounding = new EcnfTheory(theory->vocabulary(),_structure->clone());

	// Create the grounder
	theory->accept(this);
	return _toplevelgrounder;
}

/*
 * TopLevelGrounder* GrounderFactory::create(AbstractTheory* theory, SATSolver* solver)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is directly passed to the given solver. 
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created.
 *		solver	- the solver to which the grounding will be passed.
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 *		One or more models of the ground theory can be obtained by calling solve() on
 *		the solver.
 */
TopLevelGrounder* GrounderFactory::create(AbstractTheory* theory, SATSolver* solver) {

	// Allocate a solver theory
	_grounding = new SolverTheory(theory->vocabulary(),solver,_structure->clone());

	// Create the grounder
	theory->accept(this);
	return _toplevelgrounder;
}

/*
 * void GrounderFactory::visit(EcnfTheory* ecnf)
 * DESCRIPTION
 *		Creates a grounder for a ground ecnf theory. This grounder returns a (reduced) copy of the ecnf theory.
 * PARAMETERS
 *		ecnf	- the given ground ecnf theory
 * POSTCONDITIONS
 *		_toplevelgrounder is equal to the created grounder.
 */
void GrounderFactory::visit(EcnfTheory* ecnf) {
	_toplevelgrounder = new EcnfGrounder(_grounding,ecnf);	
}

/*
 * void GrounderFactory::visit(Theory* theory)
 * DESCRIPTION
 *		Creates a grounder for a non-ground theory.
 * PARAMETERS
 *		theory	- the non-ground theory
 * POSTCONDITIONS
 *		_toplevelgrounder is equal to the created grounder
 */
void GrounderFactory::visit(Theory* theory) {

	// Collect all components (sentences, definitions, and fixpoint definitions) of the theory
	vector<TheoryComponent*> components(theory->nrComponents());
	for(unsigned int n = 0; n < theory->nrComponents(); ++n) {
		components[n] = theory->component(n);
	}

	// Order components the components to optimize the grounding process
	// TODO

	// Create grounders for all components
	vector<TopLevelGrounder*> children(components.size());
	for(unsigned int n = 0; n < components.size(); ++n) {
		InitContext();
		components[n]->accept(this);
		children[n] = _toplevelgrounder; 
	}

	// Create the grounder
	_toplevelgrounder = new TheoryGrounder(_grounding,children);
}

/*
 * void GrounderFactory::visit(PredForm* pf) 
 * DESCRIPTION
 *		Creates a grounder for an atomic formula
 * PARAMETERS
 *		pf	- the atomic formula
 * PRECONDITIONS
 *		Each free variable that occurs in pf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_HEAD:		_headgrounder
 *			CC_BODY:		_formgrounder
 *			CC_FORMULA:		_formgrounder
 */
void GrounderFactory::visit(PredForm* pf) {

	// Move all functions and aggregates that are three-valued according 
	// to _structure outside the atom. To avoid changing the original atom, 
	// we first clone it.
	PredForm* newpf = pf->clone();
	Formula* transpf = FormulaUtils::moveThreeValTerms(newpf,_structure,_context._positive != PC_NEGATIVE);

	if(newpf != transpf) {	// The rewriting changed the atom
		delete(newpf);
		transpf->accept(this);
	}
	else {	// The rewriting did not change the atom

		// Create grounders for the subterms
		vector<TermGrounder*> subtermgrounders;
		vector<SortTable*>	  argsorttables;
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
			descend(pf->subterm(n));
			subtermgrounders.push_back(_termgrounder);
			argsorttables.push_back(_structure->inter(pf->symb()->sort(n)));
		}

		// Create checkers and grounder
		PredInter* inter = _structure->inter(pf->symb());
		CheckerFactory checkfactory;
		if(_context._component == CC_HEAD) {
			InstanceChecker* truech = checkfactory.create(inter,true,true);
			InstanceChecker* falsech = checkfactory.create(inter,false,true);
			_headgrounder = new HeadGrounder(_grounding,truech,falsech,pf->symb(),subtermgrounders,argsorttables);
		}
		else {
			InstanceChecker* possch;
			InstanceChecker* certainch;
			if(_context._truegen == pf->sign()) {	
				possch = checkfactory.create(inter,false,false);
				certainch = checkfactory.create(inter,true,true);
			}
			else {	
				possch = checkfactory.create(inter,true,false);
				certainch = checkfactory.create(inter,false,true);
			}
	
			// Create the grounder
			_formgrounder = new AtomGrounder(_grounding->translator(),pf->sign(),pf->symb(),
								 subtermgrounders,possch,certainch,argsorttables,_context);
			if(_context._component == CC_SENTENCE) 
				_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);
		}
	}
	transpf->recursiveDelete();
}

/*
 * void GrounderFactory::visit(BoolForm* bf)
 * DESCRIPTION
 *		Creates a grounding for a conjunction or disjunction of formulas
 * PARAMETERS
 *		bf	- the conjunction or disjunction
 * PRECONDITIONS
 *		Each free variable that occurs in bf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_BODY:		_formgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(BoolForm* bf) {

	// Handle a top-level conjunction without creating tseitin atoms
	if(_context._component == CC_SENTENCE && (bf->conj() == bf->sign())) {
		// If bf is a negated disjunction, push the negation one level deeper.
		// Take a clone to avoid changing bf;
		BoolForm* newbf = bf->clone();
		if(!(newbf->conj())) {
			newbf->conj(true);
			newbf->swapsign();
			for(unsigned int n = 0; n < newbf->nrSubforms(); ++n) newbf->subform(n)->swapsign();
		}

		// Visit the subformulas
		vector<TopLevelGrounder*> sub(newbf->nrSubforms());
		for(unsigned int n = 0; n < newbf->nrSubforms(); ++n) {
			descend(newbf->subform(n));
			sub[n] = _toplevelgrounder;
		}
		newbf->recursiveDelete();
		_toplevelgrounder = new TheoryGrounder(_grounding,sub);
	}
	else {	// Formula bf is not a top-level conjunction

		// Create grounders for subformulas
		SaveContext();
		DeeperContext(bf->sign());
		vector<FormulaGrounder*> sub(bf->nrSubforms());
		for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
			descend(bf->subform(n));
			sub[n] = _formgrounder;
		}
		RestoreContext();

		// Create grounder
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);

	}
}

/*
 * void GrounderFactory::visit(QuantForm* qf)
 * DESCRIPTION
 *		Creates a grounding for a quantified formula
 * PARAMETERS
 *		qf	- the quantified formula
 * PRECONDITIONS
 *		Each free variable that occurs in qf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_BODY:		_formgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(QuantForm* qf) {

	// Create instance generator
	vector<domelement*> vars;
	vector<SortTable*>	tables;
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		domelement* d = new domelement();
		_varmapping[qf->qvar(n)] = d;
		vars.push_back(d);
		SortTable* st = _structure->inter(qf->qvar(n)->sort());
		assert(st->finite());	// TODO: produce an error message
		tables.push_back(st);
	}
	GeneratorFactory gf;
	InstGenerator* gen = gf.create(vars,tables);

	// Handle top-level universal quantifiers efficiently
	if(_context._component == CC_SENTENCE && (qf->sign() == qf->univ())) {
		QuantForm* newqf = qf->clone();
		if(!(newqf->univ())) {
			newqf->univ(true);
			newqf->swapsign();
			newqf->subf()->swapsign();
		}
		descend(newqf->subf());
		newqf->recursiveDelete();
		_toplevelgrounder = new UnivSentGrounder(_grounding,_toplevelgrounder,gen);
	}
	else {

		// Create grounder for subformula
		SaveContext();
		DeeperContext(qf->sign());
		_context._truegen = !(qf->univ()); 
		descend(qf->subf());
		RestoreContext();

		// Create the grounder
		_formgrounder = new QuantGrounder(_grounding->translator(),_formgrounder,qf->sign(),qf->univ(),gen,_context);
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);

	}
}

/*
 * void GrounderFactory::visit(EquivForm* ef)
 * DESCRIPTION
 *		Creates a grounding for an equivalence.
 * PARAMETERS
 *		ef	- the equivalence
 * PRECONDITIONS
 *		Each free variable that occurs in qf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_BODY:		_formgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(EquivForm* ef) {

	// Create grounders for the subformulas
	SaveContext();
	DeeperContext(ef->sign());
	_context._positive = PC_BOTH;
	_context._tseitin = TS_EQ;
	descend(ef->left());
	FormulaGrounder* leftg = _formgrounder;
	descend(ef->right());
	FormulaGrounder* rightg = _formgrounder;
	RestoreContext();

	// Create the grounder
	_formgrounder = new EquivGrounder(_grounding->translator(),leftg,rightg,ef->sign(),_context);
	if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true);
}

void GrounderFactory::visit(EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::remove_eqchains(f,_grounding->vocabulary());
	f->accept(this);
	f->recursiveDelete();
}

void GrounderFactory::visit(VarTerm* t) {
	_termgrounder = new VarTermGrounder(_varmapping.find(t->var())->second);
}

void GrounderFactory::visit(DomainTerm* t) {
	_termgrounder = new DomTermGrounder(CPPointer(t->value(),t->type()));
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
	_termgrounder = new AggTermGrounder(_grounding->translator(),t->type(),_setgrounder);
}

void GrounderFactory::visit(EnumSetExpr* s) {
	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subgr;
	vector<TermGrounder*> subtgr;
	SaveContext();
	InitContext();
	_context._component = CC_FORMULA;
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		descend(s->subform(n));
		subgr.push_back(_formgrounder);
		descend(s->subterm(n));
		subtgr.push_back(_termgrounder);
	}
	RestoreContext();

	// Create set grounder
	_setgrounder = new EnumSetGrounder(_grounding->translator(),subgr,subtgr);
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
	SaveContext();
	InitContext();
	_context._component = CC_FORMULA;
	descend(s->subf());
	FormulaGrounder* sub = _formgrounder;

	// Create grounder	
	_setgrounder = new QuantSetGrounder(_grounding->translator(),sub,gen,_varmapping[s->qvar(0)]);
}

void GrounderFactory::visit(Definition* def) {
	// Create new ground definition
	_definition = new GroundDefinition();

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
	// Split the quantified variables in two categories: 
	//		1. the variables that only occur in the head
	//		2. the variables that occur in the body (and possibly in the head)
	vector<Variable*>	headvars;
	vector<Variable*>	bodyvars;
	for(unsigned int n = 0; n < rule->nrQvars(); ++n) {
		if(rule->body()->contains(rule->qvar(n))) bodyvars.push_back(rule->qvar(n));
		else headvars.push_back(rule->qvar(n));
	}
	// Create head instance generator
	InstGenerator* hig = 0;
	GeneratorNode* hnode = 0;
	for(unsigned int n = 0; n < headvars.size(); ++n) {
		domelement* d = new domelement();
		_varmapping[headvars[n]] = d;
		SortTable* st = _structure->inter(headvars[n]->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(headvars.size() == 1) {
			hig = tig;
			break;
		}
		else if(n == 0)
			hnode = new LeafGeneratorNode(tig);
		else 
			hnode = new OneChildGeneratorNode(tig,hnode);
	}
	if(!hig) hig = new TreeInstGenerator(hnode);
	
	// Create body instance generator
	InstGenerator* big = 0;
	GeneratorNode* bnode = 0;
	for(unsigned int n = 0; n < bodyvars.size(); ++n) {
		domelement* d = new domelement();
		_varmapping[bodyvars[n]] = d;
		SortTable* st = _structure->inter(bodyvars[n]->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(bodyvars.size() == 1) {
			big = tig;
			break;
		}
		else if(n == 0)
			bnode = new LeafGeneratorNode(tig);
		else 
			bnode = new OneChildGeneratorNode(tig,bnode);
	}
	if(!big) big = new TreeInstGenerator(bnode);
	
	// Create head grounder
	_context._component = CC_HEAD;
	rule->head()->accept(this);
	HeadGrounder* hgr = _headgrounder;

	// Create body grounder
	_context._positive = PC_NEGATIVE;	// minimize truth value of rule bodies
	_context._truegen = true;				// body instance generator corresponds to an existential quantifier
	_context._component = CC_BODY;
	rule->body()->accept(this);
	FormulaGrounder* bgr = _formgrounder;

	// TODO: conjunction? recursive?
	bool conj;
	bool rec;

	// Create rule grounder
	_rulegrounder = new RuleGrounder(_definition,hgr,bgr,hig,big,conj,rec);
}
