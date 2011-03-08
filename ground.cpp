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
#include "options.hpp"

/** The two built-in literals 'true' and 'false' **/
int _true = numeric_limits<int>::max();
int _false = 0;

extern CLOptions _cloptions;

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
	PCTsBody* tb = new PCTsBody();
	tb->_body = cl;
	tb->_type = tp;
	tb->_conj = conj;
	_tsbodies[nr] = tb;
	return nr;
}

int GroundTranslator::translate(int setnr, AggType atp, char comp, double bound, TsType ttp) {
	if(comp == '=') {
		vector<int> cl(2);
		cl[0] = -(translate(setnr,atp,'<',bound,ttp));
		cl[1] = -(translate(setnr,atp,'>',bound,ttp));
		return translate(cl,true,ttp);
	}
	else {
		int nr = nextNumber();
		AggTsBody* tb = new AggTsBody();
		if(ttp == TS_IMPL) tb->_type = TS_RIMPL;
		else if(ttp == TS_RIMPL) tb->_type = TS_IMPL;
		else tb->_type = ttp;
		tb->_setnr = setnr;
		tb->_aggtype = atp;
		tb->_lower = (comp == '>');
		tb->_bound = bound;
		_tsbodies[nr] = tb;
		return -nr;
	}
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

string GroundTranslator::printatom(int nr) const {
	stringstream s;
	nr = abs(nr);
	if(nr == _true) return "true";
	else if(nr == _false) return "false";
	if(nr >= int(_backsymbtable.size())) {
		return "error";
	}
	PFSymbol* pfs = symbol(nr);
	if(pfs) {
		s << pfs->to_string();
		if(!(args(nr).empty())) {
			s << "(";
			for(unsigned int c = 0; c < args(nr).size(); ++c) {
				s << ElementUtil::ElementToString((args(nr))[c]);
				if(c !=  args(nr).size()-1) s << ",";
			}
			s << ")";
		}
	}
	else s << "tseitin_" << nr;
	return s.str();
}
/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

void NaiveGrounder::visit(const VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	TypedElement te = _varmapping[vt->var()];
	Element e = te._element;
	_returnTerm = new DomainTerm(vt->sort(),te._type,e,ParseInfo());
}

void NaiveGrounder::visit(const DomainTerm* dt) {
	_returnTerm = dt->clone();
}

void NaiveGrounder::visit(const FuncTerm* ft) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		_returnTerm = 0;
		ft->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnTerm = new FuncTerm(ft->func(),vt,ParseInfo());
}

void NaiveGrounder::visit(const AggTerm* at) {
	_returnSet = 0;
	at->set()->accept(this);
	assert(_returnSet);
	_returnTerm = new AggTerm(_returnSet,at->type(),ParseInfo());
}

void NaiveGrounder::visit(const EnumSetExpr* s) {
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

void NaiveGrounder::visit(const QuantSetExpr* s) {
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

void NaiveGrounder::visit(const PredForm* pf) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		_returnTerm = 0;
		pf->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new PredForm(pf->sign(),pf->symb(),vt,FormParseInfo());
}

void NaiveGrounder::visit(const EquivForm* ef) {
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

void NaiveGrounder::visit(const EqChainForm* ef) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_returnTerm = 0;
		ef->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new EqChainForm(ef->sign(),ef->conj(),vt,ef->comps(),ef->compsigns(),FormParseInfo());
}

void NaiveGrounder::visit(const BoolForm* bf) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		_returnFormula = 0;
		bf->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	_returnFormula = new BoolForm(bf->sign(),bf->conj(),vf,FormParseInfo());
}

void NaiveGrounder::visit(const QuantForm* qf) {
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

void NaiveGrounder::visit(const Rule* r) {
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

void NaiveGrounder::visit(const Definition* d) {
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

void NaiveGrounder::visit(const FixpDef* d) {
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

void NaiveGrounder::visit(const Theory* t) {
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
	if(cl.empty()) {
		return _conj ? true : false;
	}
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
			for(unsigned int n = 0; n < cl.size(); ++n)
				_grounding->addUnitClause(cl[n]);
		}
		else {
			_grounding->addClause(cl);
		}
		return true;
	}
}

bool UnivSentGrounder::run() const {
	if(_generator->first()) {
		bool b = _subgrounder->run();
		if(!b) {
			_grounding->addEmptyClause();
			return b;
		}
		while(_generator->next()) {
			b = _subgrounder->run();
			if(!b) {
				_grounding->addEmptyClause();
				return b;
			}
		}
	}
	return true;
}

#ifndef NDEBUG
void FormulaGrounder::setorig(const Formula* f, const map<Variable*,domelement*>& mvd) {
	map<Variable*,Variable*> mvv;
	for(unsigned int n = 0; n < f->nrFvars(); ++n) {
		Variable* v = new Variable(f->fvar(n)->name(),f->fvar(n)->sort(),ParseInfo());
		mvv[f->fvar(n)] = v;
		_varmap[v] = mvd.find(f->fvar(n))->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printorig() const {
	cerr << "Grounding formula " << _origform->to_string() << " with instance ";
	for(unsigned int n = 0; n < _origform->nrFvars(); ++n) {
		cerr << _origform->fvar(n)->to_string() << " = ";
		Element e; e._compound = *(_varmap.find(_origform->fvar(n))->second);
		cerr << ElementUtil::ElementToString(e,ELCOMPOUND) << ' ';
	}
	cerr << endl;

}
#endif

AtomGrounder::AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
							const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, const GroundingContext& ct) :
	FormulaGrounder(gt,ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic), _symbol(gt->addSymbol(s)), _args(sg.size()), _tables(vst), _sign(sign)
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
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Partial function went out of bounds\n";
				cerr << "Result is " << (_context._positive != PC_NEGATIVE  ? "true" : "false") << endl;
			}
#endif
			return _context._positive != PC_NEGATIVE  ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(!_tables[n]->contains(_args[n])) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Term value out of predicate type\n";
				cerr << "Result is " << (_sign  ? "false" : "true") << endl;
			}
#endif
			return _sign ? _false : _true;
		}
	}

	// Run instance checkers and return grounding
	if(!(_pchecker->run(_args))) {
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Possible checker failed\n";
			cerr << "Result is " << (_certainvalue ? "false" : "true") << endl;
		}
#endif
		return _certainvalue ? _false : _true;	// TODO: dit is lelijk
	}
	if(_cchecker->run(_args)) {
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Certain checker succeeded\n";
			cerr << "Result is " << _translator->printatom(_certainvalue) << endl;
		}
#endif
		return _certainvalue;
	}
	else {
		int atom = _translator->translate(_symbol,_args);
		if(!_sign) atom = -atom;
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Term value out of predicate type\n";
			cerr << "Result is " << _translator->printatom(atom) << endl;
		}
#endif
		return atom;
	}
}

void AtomGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
#ifndef NDEBUG
if(_cloptions._verbose) {
	printorig();
	cerr << "Result = " << _translator->printatom(clause.back()) << endl;
}
#endif
}

int AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const {
	int leftvalue = int(boundvalue - truevalue);
	const GroundSet& grs = _translator->groundset(setnr);
	int maxposscard = grs._setlits.size();
	TsType tp = _context._tseitin;	// TODO
	switch(_comp) {
		case '=':
			if(leftvalue < 0 || leftvalue > maxposscard) {
				return _sign ? _false : _true;
			}
			else if(leftvalue == 0) {
				int tseitin = _translator->translate(grs._setlits,false,tp);
				return _sign ? -tseitin : tseitin;
			}
			else if(leftvalue == maxposscard) {
				int tseitin = _translator->translate(grs._setlits,true,tp);
				return _sign ? tseitin : -tseitin;
			}
			break;
		case '<':
			if(leftvalue < 0) {
				return _sign ? _true : _false;
			}
			else if(leftvalue == 0) {
				int tseitin = _translator->translate(grs._setlits,false,tp);
				return _sign ? tseitin : -tseitin;
			}
			else if(leftvalue == maxposscard-1) {
				int tseitin = _translator->translate(grs._setlits,true,tp);
				return _sign ? tseitin : -tseitin;
			}
			else if(leftvalue >= maxposscard) {
				return _sign ? _false : _true;
			}
			break;
		case '>':
			if(leftvalue <= 0) {
				return _sign ? _false : _true;
			}
			else if(leftvalue == 1) {
				int tseitin = _translator->translate(grs._setlits,false,tp);
				return _sign ? -tseitin : tseitin;
			}
			else if(leftvalue == maxposscard) {
				int tseitin = _translator->translate(grs._setlits,true,tp);
				return _sign ? -tseitin : tseitin;
			}
			else if(leftvalue > maxposscard) {
				return _sign ? _true : _false;
			}
			break;
	}
	int tseitin = _translator->translate(setnr,AGGCARD,_comp,double(leftvalue),tp);
	return _sign ? tseitin : -tseitin;
}

int AggGrounder::finishSum(double truevalue, double boundvalue, int setnr) const {
	const GroundSet& grs = _translator->groundset(setnr);

	// Compute the minimal and maximal possible value of the sum
	double minposssum = truevalue;
	double maxposssum = truevalue;
	bool containszeros = false;
	for(unsigned int n = 0; n < grs._litweights.size(); ++n) {
		if(grs._litweights[n] > 0) maxposssum += grs._litweights[n];
		else if(grs._litweights[n] < 0) minposssum += grs._litweights[n];
		else containszeros = true;
	}

	TsType tp = _context._tseitin;	// TODO
	switch(_comp) {
		case '=':
			if(minposssum > boundvalue || maxposssum < boundvalue) {
				return _sign ? _false : _true;
			}
			// TODO: more complicated propagation is possible!
			break;
		case '<':
			if(boundvalue < minposssum) {
				return _sign ? _true : _false;
			}
			else if(boundvalue >= maxposssum) {
				return _sign ? _false : _true;
			}
			// TODO: more complicated propagation is possible!
			break;
		case '>':
			if(boundvalue > maxposssum) {
				return _sign ? _true : _false;
			}
			else if(boundvalue <= minposssum) {
				return _sign ? _false : _true;
			}
			// TODO: more complicated propagation is possible!
			break;
	}
	int tseitin = _translator->translate(setnr,AGGSUM,_comp,boundvalue+truevalue,tp);
	return _sign ? tseitin : -tseitin;

}

int AggGrounder::run() const {
	int setnr = _setgrounder->run();
	domelement bound = _boundgrounder->run();
	const GroundSet& grs = _translator->groundset(setnr);

	double truevalue = AggUtils::compute(_type,grs._trueweights);
	double boundvalue = ElementUtil::convert(bound->_args[0],ELDOUBLE)._double;
	int ts;
	switch(_type) {
		case AGGCARD: 
			ts = finishCard(truevalue,boundvalue,setnr);
			break;
		case AGGSUM:
			ts = finishSum(truevalue,boundvalue,setnr);
			break;
		case AGGPROD:
			assert(false);
			// TODO
			break;
		case AGGMIN:
			assert(false);
			// TODO
			break;
		case AGGMAX:
			assert(false);
			// TODO
			break;
		default:
			assert(false);
	}
	return ts;
}

void AggGrounder::run(vector<int>& clause) const {
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
	if(cl.empty()) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << _translator->printatom(result2()) << endl;
			}
#endif
		return result2();
	}
	else if(cl.size() == 1) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << (_sign ? _translator->printatom(cl[0]) : _translator->printatom(-cl[0])) << endl;
			}
#endif
		return _sign ? cl[0] : -cl[0];
	}
	else {
		TsType tp = _context._tseitin;
		if(!_sign) {
			if(tp == TS_IMPL) tp = TS_RIMPL;
			else if(tp == TS_RIMPL) tp = TS_IMPL;
		}
		int ts = _translator->translate(cl,_conj,tp);
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << (_sign ? "" : "~");
				cerr << _translator->printatom(cl[0]) << ' ';
				for(unsigned int n = 1; n < cl.size(); ++n) cerr << (_conj ? "& " : "| ") << _translator->printatom(cl[n]) << ' ';
			}
#endif
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
		if(check1(l)) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << _translator->printatom(result1()) << endl;
			}
#endif
			return result1();
		}
		else if(! check2(l)) cl.push_back(l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
#ifndef NDEBUG
				if(_cloptions._verbose) {
					printorig();
					cerr << "Result = " << _translator->printatom(result1()) << endl;
				}
#endif
				return result1();
			}
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
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << _translator->printatom(result1()) << endl;
			}
#endif
			return;
		}
		else if(! check2(l)) clause.push_back(_sign ? l : -l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
				clause.clear();
				clause.push_back(result1());
#ifndef NDEBUG
				if(_cloptions._verbose) {
					printorig();
					cerr << "Result = " << _translator->printatom(result1()) << endl;
				}
#endif
				return;
			}
			else if(! check2(l)) clause.push_back(_sign ? l : -l);
		}
	}
#ifndef NDEBUG
	if(_cloptions._verbose) {
		printorig();
		cerr << "Result = " << (_sign ? "" : "~");
		if(clause.empty()) cerr << (_conj ? "true" : "false") << endl;
		else {
			cerr << _translator->printatom(clause[0]) << ' ';
			for(unsigned int n = 1; n < clause.size(); ++n) cerr << (_conj ? "& " : "| ") << _translator->printatom(clause[n]) << ' ';
		}
	}
#endif
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
		tcl[0] = _translator->translate(cl1,false,tp);
		tcl[1] = _translator->translate(cl2,false,tp);
		return _translator->translate(tcl,true,tp);
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

#ifndef NDEBUG
void TermGrounder::setorig(const Term* t, const map<Variable*,domelement*>& mvd) {
	map<Variable*,Variable*> mvv;
	for(unsigned int n = 0; n < t->nrFvars(); ++n) {
		Variable* v = new Variable(t->fvar(n)->name(),t->fvar(n)->sort(),ParseInfo());
		mvv[t->fvar(n)] = v;
		_varmap[v] = mvd.find(t->fvar(n))->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printorig() const {
	cerr << "Grounding term " << _origterm->to_string() << " with instance ";
	for(unsigned int n = 0; n < _origterm->nrFvars(); ++n) {
		cerr << _origterm->fvar(n)->to_string() << " = ";
		Element e; e._compound = *(_varmap.find(_origterm->fvar(n))->second);
		cerr << ElementUtil::ElementToString(e,ELCOMPOUND) << ' ';
	}
	cerr << endl;
}
#endif 

domelement VarTermGrounder::run() const {
	return *_value;
}

domelement FuncTermGrounder::run() const {
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
	}
#ifndef NDEBUG
if(_cloptions._verbose) {
	printorig();
	Element e; e._compound = (*_function)[_args];
	cerr << "Result is " << ElementUtil::ElementToString(e,ELCOMPOUND) << endl;
}
#endif
	return (*_function)[_args];
}

domelement AggTermGrounder::run() const {
	int setnr = _setgrounder->run();
	const GroundSet& grs = _translator->groundset(setnr);
	assert(grs._setlits.empty());
	double value = AggUtils::compute(_type,grs._trueweights);
	Element e;
	if(isInt(value)) {
		e._int = int(value);
#ifndef NDEBUG
if(_cloptions._verbose) {
	printorig();
	cerr << "Result is " << ElementUtil::ElementToString(e,ELINT) << endl;
}
#endif
		return CPPointer(e,ELINT);
	}
	else {
		e._double = value;
#ifndef NDEBUG
if(_cloptions._verbose) {
	printorig();
	cerr << "Result is " << ElementUtil::ElementToString(e,ELDOUBLE) << endl;
}
#endif
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
	bool conj = _bodygrounder->conjunctive();
	if(_bodygenerator->first()) {	
		vector<int>	body;
		_bodygrounder->run(body);
		bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
		if(!falsebody) {
			bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
			if(_headgenerator->first()) {
				int head = _headgrounder->run();
				assert(head != _true);
				if(head != _false) {
					if(truebody) _definition->addTrueRule(head);
					else _definition->addRule(head,body,conj,_context._tseitin == TS_RULE);
				}
				while(_headgenerator->next()) {
					head = _headgrounder->run();
					assert(head != _true);
					if(head != _false) {
						if(truebody) _definition->addTrueRule(head);
						else _definition->addRule(head,body,conj,_context._tseitin == TS_RULE);
					}
				}
			}
		}
		while(_bodygenerator->next()) {
			body.clear();
			_bodygrounder->run(body);
			bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
			if(!falsebody) {
				bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
				if(_headgenerator->first()) {
					int head = _headgrounder->run();
					assert(head != _true);
					if(head != _false) {
						if(truebody) _definition->addTrueRule(head);
						else _definition->addRule(head,body,conj,_context._tseitin == TS_RULE);
					}
					while(_headgenerator->next()) {
						head = _headgrounder->run();
						assert(head != _true);
						if(head != _false) {
							if(truebody) _definition->addTrueRule(head);
							else _definition->addRule(head,body,conj,_context._tseitin == TS_RULE);
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

bool GrounderFactory::recursive(const Formula* f) {
	for(set<PFSymbol*>::const_iterator it = _context._defined.begin(); it != _context._defined.end(); ++it) {
		if(f->contains(*it)) return true;
	}
	return false;
}

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
	_context._defined.clear();
}

void GrounderFactory::AggContext() {
	_context._truegen = false;
	_context._positive = PC_POSITIVE;
	_context._tseitin = TS_IMPL;
	_context._component = CC_FORMULA;
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
 *		Visits a term and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		t	- the visited term
 */
void GrounderFactory::descend(Term* t) {
	SaveContext();
	t->accept(this);
	RestoreContext();
}

/*
 * void GrounderFactory::descend(SetExpr* s)
 * DESCRIPTION
 *		Visits a set and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		s	- the visited set
 */
void GrounderFactory::descend(SetExpr* s) {
	SaveContext();
	s->accept(this);
	RestoreContext();
}

/*
 * void GrounderFactory::descend(Formula* f)
 * DESCRIPTION
 *		Visits a formula and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		f	- the visited formula
 */
void GrounderFactory::descend(Formula* f) {
	SaveContext();
	f->accept(this);
	RestoreContext();
}

/*
 * void GrounderFactory::descend(Rule* r)
 * DESCRIPTION
 *		Visits a rule and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		r	- the visited rule
 */
void GrounderFactory::descend(Rule* r) {
	SaveContext();
	r->accept(this);
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
TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory) {

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
TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory, SATSolver* solver) {

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
void GrounderFactory::visit(const EcnfTheory* ecnf) {
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
void GrounderFactory::visit(const Theory* theory) {

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
 *			CC_FORMULA:		_formgrounder
 */
void GrounderFactory::visit(const PredForm* pf) {
	// Move all functions and aggregates that are three-valued according 
	// to _structure outside the atom. To avoid changing the original atom, 
	// we first clone it.
	PredForm* newpf = pf->clone();
	Formula* transpf = FormulaUtils::moveThreeValTerms(newpf,_structure,_context._positive != PC_NEGATIVE);

	if(newpf != transpf) {	// The rewriting changed the atom
		//delete(newpf); TODO: produces a segfault??
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
#ifndef NDEBUG
			_formgrounder->setorig(pf,_varmapping);
#endif
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
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const BoolForm* bf) {

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
		SaveContext();
		if(recursive(bf)) _context._tseitin = TS_RULE;
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		RestoreContext();
#ifndef NDEBUG
		_formgrounder->setorig(bf,_varmapping);
#endif	
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
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const QuantForm* qf) {
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
		Formula* newsub = qf->subf()->clone();
		if(!(qf->univ())) newsub->swapsign();
		descend(newsub);
		newsub->recursiveDelete();
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
		SaveContext();
		if(recursive(qf)) _context._tseitin = TS_RULE;
		_formgrounder = new QuantGrounder(_grounding->translator(),_formgrounder,qf->sign(),qf->univ(),gen,_context);
		RestoreContext();
#ifndef NDEBUG
		_formgrounder->setorig(qf,_varmapping);
#endif	
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
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const EquivForm* ef) {
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
	SaveContext();
	if(recursive(ef)) _context._tseitin = TS_RULE;
	_formgrounder = new EquivGrounder(_grounding->translator(),leftg,rightg,ef->sign(),_context);
	RestoreContext();
	if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true);
}

void GrounderFactory::visit(const AggForm* af) {
	descend(af->left());
	TermGrounder* boundgr = _termgrounder;
	descend(af->right()->set());
	SetGrounder* setgr = _setgrounder;

	SaveContext();
	if(recursive(af)) _context._tseitin = TS_RULE;
	_formgrounder = new AggGrounder(_grounding->translator(),_context,af->right()->type(),setgr,boundgr,af->comp(),af->sign());
	RestoreContext();
	if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true);
}

void GrounderFactory::visit(const EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::remove_eqchains(f,_grounding->vocabulary());
	f->accept(this);
	f->recursiveDelete();
}

void GrounderFactory::visit(const VarTerm* t) {
	assert(_varmapping.find(t->var()) != _varmapping.end());
	_termgrounder = new VarTermGrounder(_varmapping.find(t->var())->second);
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
}

void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(CPPointer(t->value(),t->type()));
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
}

void GrounderFactory::visit(const FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> sub;
	for(unsigned int n = 0; n < t->nrSubterms(); ++n) {
		t->subterm(n)->accept(this);
		if(_termgrounder) sub.push_back(_termgrounder);
	}

	// Create term grounder
	FuncTable* ft = _structure->inter(t->func())->functable();
	_termgrounder = new FuncTermGrounder(sub,ft);
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
}

void GrounderFactory::visit(const AggTerm* t) {
	// Create set grounder
	t->set()->accept(this);

	// Create term grounder
	_termgrounder = new AggTermGrounder(_grounding->translator(),t->type(),_setgrounder);
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
}

void GrounderFactory::visit(const EnumSetExpr* s) {
	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subgr;
	vector<TermGrounder*> subtgr;
	SaveContext();
	AggContext();
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

void GrounderFactory::visit(const QuantSetExpr* s) {
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
		else if(n == 0)	node = new LeafGeneratorNode(tig);
		else node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	
	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(s->subf());
	FormulaGrounder* sub = _formgrounder;
	RestoreContext();

	// Create grounder	
	_setgrounder = new QuantSetGrounder(_grounding->translator(),sub,gen,_varmapping[s->qvar(0)]);
}

void GrounderFactory::visit(const Definition* def) {
	// Create new ground definition
	_definition = new GroundDefinition(_grounding->translator());

	// Store defined predicates
	for(unsigned int m = 0; m < def->nrDefsyms(); ++m) {
		_context._defined.insert(def->defsym(m));
	}
	
	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for(unsigned int n = 0; n < def->nrRules(); ++n) {
		descend(def->rule(n));
		subgrounders.push_back(_rulegrounder);
	}
	
	// Create definition grounder
	_toplevelgrounder = new DefinitionGrounder(_grounding,_definition,subgrounders);

	_context._defined.clear();
}

void GrounderFactory::visit(const Rule* rule) {
	// Split the quantified variables in two categories: 
	//		1. the variables that only occur in the head
	//		2. the variables that occur in the body (and possibly in the head)
	vector<Variable*>	headvars;
	vector<Variable*>	bodyvars;
	for(unsigned int n = 0; n < rule->nrQvars(); ++n) {
		if(rule->body()->contains(rule->qvar(n))) {
			bodyvars.push_back(rule->qvar(n));
		}
		else {
			headvars.push_back(rule->qvar(n));
		}
	}

	// Create head instance generator
	InstGenerator* headgen = 0;
	GeneratorNode* hnode = 0;
	for(unsigned int n = 0; n < headvars.size(); ++n) {
		domelement* d = new domelement();
		_varmapping[headvars[n]] = d;
		SortTable* st = _structure->inter(headvars[n]->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* sig = new SortInstGenerator(st,d);
		if(headvars.size() == 1) {
			headgen = sig;
			break;
		}
		else if(n == 0) hnode = new LeafGeneratorNode(sig);
		else hnode = new OneChildGeneratorNode(sig,hnode);
	}
	if(!headgen) headgen = new TreeInstGenerator(hnode);
	
	// Create body instance generator
	InstGenerator* bodygen = 0;
	GeneratorNode* bnode = 0;
	for(unsigned int n = 0; n < bodyvars.size(); ++n) {
		domelement* d = new domelement();
		_varmapping[bodyvars[n]] = d;
		SortTable* st = _structure->inter(bodyvars[n]->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* sig = new SortInstGenerator(st,d);
		if(bodyvars.size() == 1) {
			bodygen = sig;
			break;
		}
		else if(n == 0) bnode = new LeafGeneratorNode(sig);
		else bnode = new OneChildGeneratorNode(sig,bnode);
	}
	if(!bodygen) bodygen = new TreeInstGenerator(bnode);
	
	// Create head grounder
	SaveContext();
	_context._component = CC_HEAD;
	descend(rule->head());
	HeadGrounder* headgr = _headgrounder;
	RestoreContext();

	// Create body grounder
	SaveContext();
	_context._positive = PC_NEGATIVE;		// minimize truth value of rule bodies
	_context._truegen = true;				// body instance generator corresponds to an existential quantifier
	_context._component = CC_FORMULA;
	_context._tseitin = TS_EQ;
	descend(rule->body());
	FormulaGrounder* bodygr = _formgrounder;
	RestoreContext();

	// Create rule grounder
	SaveContext();
	if(recursive(rule->body())) _context._tseitin = TS_RULE;
	_rulegrounder = new RuleGrounder(_definition,headgr,bodygr,headgen,bodygen,_context);
	RestoreContext();

}
