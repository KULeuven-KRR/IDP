/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IncludeComponents.hpp"
#include "insert.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

#include "theory/information/CheckSorts.hpp"
#include "theory/transformations/DeriveSorts.hpp"

#include "parser/yyltype.hpp"
#include "parser.hh"
#include "errorhandling/error.hpp"
#include "options.hpp"
#include "internalargument.hpp"
#include "lua/luaconnection.hpp"
#include "theory/Query.hpp"
#include "structure/StructureComponents.hpp"

#include "GlobalData.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;
using namespace LuaConnection;
using namespace Error;
//TODO add abstraction to remove lua dependence here

/**
 * Rewrite a vector of strings s1,s2,...,sn to the single string s1::s2::...::sn
 */
string toString(const longname& vs) {
	stringstream sstr;
	if (!vs.empty()) {
		sstr << vs[0];
		for (unsigned int n = 1; n < vs.size(); ++n)
			sstr << "::" << vs[n];
	}
	return sstr.str();
}

string predName(const longname& name, const vector<Sort*>& vs) {
	stringstream sstr;
	sstr << toString(name);
	if (!vs.empty()) {
		sstr << '[' << vs[0]->name();
		for (unsigned int n = 1; n < vs.size(); ++n)
			sstr << ',' << vs[n]->name();
		sstr << ']';
	}
	return sstr.str();
}

string funcName(const longname& name, const vector<Sort*>& vs) {
	Assert(!vs.empty());
	stringstream sstr;
	sstr << toString(name) << '[';
	if (vs.size() > 1) {
		sstr << vs[0]->name();
		for (unsigned int n = 1; n < vs.size() - 1; ++n)
			sstr << ',' << vs[n]->name();
	}
	sstr << ':' << vs.back()->name() << ']';
	return sstr.str();
}

/*************
 NSPair
 *************/

void NSPair::includePredArity() {
	Assert(_sortsincluded && !_arityincluded);
	_name.back() = _name.back() + '/' + convertToString(_sorts.size());
	_arityincluded = true;
}

void NSPair::includeFuncArity() {
	Assert(_sortsincluded && !_arityincluded);
	_name.back() = _name.back() + '/' + convertToString(_sorts.size() - 1);
	_arityincluded = true;
}

void NSPair::includeArity(unsigned int n) {
	Assert(!_arityincluded);
	_name.back() = _name.back() + '/' + convertToString(n);
	_arityincluded = true;
}

ostream& NSPair::put(ostream& output) const {
	Assert(!_name.empty());
	string str = _name[0];
	for (unsigned int n = 1; n < _name.size(); ++n)
		str = str + "::" + _name[n];
	if (_sortsincluded) {
		if (_arityincluded)
			str = str.substr(0, str.rfind('/'));
		str = str + '[';
		if (!_sorts.empty()) {
			if (_func && _sorts.size() == 1)
				str = str + ':';
			if (_sorts[0])
				str = str + _sorts[0]->name();
			for (unsigned int n = 1; n < _sorts.size() - 1; ++n) {
				if (_sorts[n])
					str = str + ',' + _sorts[n]->name();
			}
			if (_sorts.size() > 1) {
				if (_func)
					str = str + ':';
				else
					str = str + ',';
				if (_sorts[_sorts.size() - 1])
					str = str + _sorts[_sorts.size() - 1]->name();
			}
		}
		str = str + ']';
	}
	output << str;
	return output;
}

/**********
 * Insert
 **********/

bool Insert::belongsToVoc(Sort* s) const {
	if (_currvocabulary->contains(s)) {
		return true;
	}
	return false;
}

bool Insert::belongsToVoc(Predicate* p) const {
	if (_currvocabulary->contains(p)) {
		return true;
	}
	return false;
}

bool Insert::belongsToVoc(Function* f) const {
	if (_currvocabulary->contains(f)) {
		return true;
	}
	return false;
}

std::set<Predicate*> Insert::noArPredInScope(const string& name) const {
	std::set<Predicate*> vp;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		std::set<Predicate*> nvp = _usingvocab[n]->pred_no_arity(name);
		vp.insert(nvp.cbegin(), nvp.cend());
	}
	return vp;
}

std::set<Predicate*> Insert::noArPredInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return noArPredInScope(vs[0]);
	} else {
		vector<string> vv(vs.size() - 1);
		for (size_t n = 0; n < vv.size(); ++n) {
			vv[n] = vs[n];
		}
		Vocabulary* v = vocabularyInScope(vv, pi);
		if (v) {
			return v->pred_no_arity(vs.back());
		} else {
			return std::set<Predicate*>();
		}
	}
}

std::set<Function*> Insert::noArFuncInScope(const string& name) const {
	std::set<Function*> vf;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		std::set<Function*> nvf = _usingvocab[n]->func_no_arity(name);
		vf.insert(nvf.cbegin(), nvf.cend());
	}
	return vf;
}

std::set<Function*> Insert::noArFuncInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return noArFuncInScope(vs[0]);
	} else {
		vector<string> vv(vs.size() - 1);
		for (size_t n = 0; n < vv.size(); ++n) {
			vv[n] = vs[n];
		}
		Vocabulary* v = vocabularyInScope(vv, pi);
		if (v) {
			return v->func_no_arity(vs.back());
		} else {
			return std::set<Function*>();
		}
	}
}

Function* Insert::funcInScope(const string& name) const {
	std::set<Function*> vf;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		Function* f = _usingvocab[n]->func(name);
		if (f) {
			vf.insert(f);
		}
	}
	if (vf.empty()) {
		return NULL;
	} else {
		return FuncUtils::overload(vf);
	}
}

Function* Insert::funcInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return funcInScope(vs[0]);
	} else {
		vector<string> vv(vs.size() - 1);
		for (size_t n = 0; n < vv.size(); ++n) {
			vv[n] = vs[n];
		}
		Vocabulary* v = vocabularyInScope(vv, pi);
		if (v) {
			return v->func(vs.back());
		} else {
			return NULL;
		}
	}
}

Predicate* Insert::predInScope(const string& name) const {
	std::set<Predicate*> vp;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		Predicate* p = _usingvocab[n]->pred(name);
		if (p) {
			vp.insert(p);
		}
	}
	if (vp.empty()) {
		return NULL;
	} else {
		return PredUtils::overload(vp);
	}
}

Predicate* Insert::predInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return predInScope(vs[0]);
	} else {
		vector<string> vv(vs.size() - 1);
		for (size_t n = 0; n < vv.size(); ++n) {
			vv[n] = vs[n];
		}
		Vocabulary* v = vocabularyInScope(vv, pi);
		if (v) {
			return v->pred(vs.back());
		} else {
			return NULL;
		}
	}
}

Sort* Insert::sortInScope(const string& name, const ParseInfo& pi) const {
	Sort* s = NULL;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		auto temp = _usingvocab[n]->sort(name);
		if (temp == NULL) {
			continue;
		}
		if (s != NULL) {
			overloaded(ComponentType::Sort, s->name(), s->pi(), temp->pi(), pi);
		} else {
			s = temp;
		}
	}
	return s;
}

Sort* Insert::sortInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return sortInScope(vs[0], pi);
	} else {
		vector<string> vv(vs.size() - 1);
		for (size_t n = 0; n < vv.size(); ++n) {
			vv[n] = vs[n];
		}
		Vocabulary* v = vocabularyInScope(vv, pi);
		if (v) {
			return v->sort(vs.back());
		} else {
			return NULL;
		}
	}
}

Namespace* Insert::namespaceInScope(const string& name, const ParseInfo& pi) const {
	Namespace* ns = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isSubspace(name)) {
			if (ns) {
				overloaded(ComponentType::Namespace, name, _usingspace[n]->pi(), ns->pi(), pi);
			} else {
				ns = _usingspace[n]->subspace(name);
			}
		}
	}
	return ns;
}

Namespace* Insert::namespaceInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return namespaceInScope(vs[0], pi);
	} else {
		Namespace* ns = namespaceInScope(vs[0], pi);
		for (size_t n = 1; n < vs.size(); ++n) {
			if (ns->isSubspace(vs[n])) {
				ns = ns->subspace(vs[n]);
			} else {
				return NULL;
			}
		}
		return ns;
	}
}

Vocabulary* Insert::vocabularyInScope(const string& name, const ParseInfo& pi) const {
	Vocabulary* v = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isVocab(name)) {
			if (v) {
				overloaded(ComponentType::Vocabulary, name, _usingspace[n]->vocabulary(name)->pi(), v->pi(), pi);
			} else {
				v = _usingspace[n]->vocabulary(name);
			}
		}
	}
	return v;
}

Vocabulary* Insert::vocabularyInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return vocabularyInScope(vs[0], pi);
	} else {
		Namespace* ns = namespaceInScope(vs[0], pi);
		if (ns) {
			for (size_t n = 1; n < (vs.size() - 1); ++n) {
				if (ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				} else {
					return NULL;
				}
			}
			if (ns->isVocab(vs.back())) {
				return ns->vocabulary(vs.back());
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
	}
}

AbstractStructure* Insert::structureInScope(const string& name, const ParseInfo& pi) const {
	AbstractStructure* s = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isStructure(name)) {
			if (s) {
				overloaded(ComponentType::Structure, name, _usingspace[n]->structure(name)->pi(), s->pi(), pi);
			} else {
				s = _usingspace[n]->structure(name);
			}
		}
	}
	return s;
}

AbstractStructure* Insert::structureInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return structureInScope(vs[0], pi);
	} else {
		Namespace* ns = namespaceInScope(vs[0], pi);
		for (size_t n = 1; n < (vs.size() - 1); ++n) {
			if (ns->isSubspace(vs[n])) {
				ns = ns->subspace(vs[n]);
			} else {
				return NULL;
			}
		}
		if (ns->isStructure(vs.back())) {
			return ns->structure(vs.back());
		} else {
			return NULL;
		}
	}
}

Query* Insert::queryInScope(const string& name, const ParseInfo& pi) const {
	Query* q = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isQuery(name)) {
			if (q) {
				overloaded(ComponentType::Query, name, _usingspace[n]->query(name)->pi(), q->pi(), pi);
			} else {
				q = _usingspace[n]->query(name);
			}
		}
	}
	return q;
}

Term* Insert::termInScope(const string& name, const ParseInfo& pi) const {
	Term* t = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isTerm(name)) {
			if (t) {
				overloaded(ComponentType::Term, name, _usingspace[n]->term(name)->pi(), t->pi(), pi);
			} else {
				t = _usingspace[n]->term(name);
			}
		}
	}
	return t;
}

AbstractTheory* Insert::theoryInScope(const string& name, const ParseInfo& pi) const {
	AbstractTheory* th = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isTheory(name)) {
			if (th) {
				overloaded(ComponentType::Theory, name, _usingspace[n]->theory(name)->pi(), th->pi(), pi);
			} else {
				th = _usingspace[n]->theory(name);
			}
		}
	}
	return th;
}

UserProcedure* Insert::procedureInScope(const string& name, const ParseInfo& pi) const {
	UserProcedure* lp = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isProc(name)) {
			if (lp) {
				overloaded(ComponentType::Procedure, name, _usingspace[n]->procedure(name)->pi(), lp->pi(), pi);
			} else {
				lp = _usingspace[n]->procedure(name);
			}
		}
	}
	return lp;
}

UserProcedure* Insert::procedureInScope(const vector<string>& vs, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return procedureInScope(vs[0], pi);
	} else {
		Namespace* ns = namespaceInScope(vs[0], pi);
		for (size_t n = 1; n < (vs.size() - 1); ++n) {
			if (ns->isSubspace(vs[n])) {
				ns = ns->subspace(vs[n]);
			} else {
				return NULL;
			}
		}
		if (ns->isProc(vs.back())) {
			return ns->procedure(vs.back());
		} else {
			return NULL;
		}
	}
}

UTF getUTF(const string& utf, const ParseInfo& pi) {
	if (utf == "u")
		return UTF_UNKNOWN;
	else if (utf == "ct")
		return UTF_CT;
	else if (utf == "cf")
		return UTF_CF;
	else {
		expectedutf(utf, pi);
		return UTF_ERROR;
	}
}

Insert::Insert(Namespace * ns) {
	Assert(ns!=NULL);
	openblock();
	_currfile = 0;
	_currspace = ns;
	usenamespace(_currspace);
}

string* Insert::currfile() const {
	return _currfile;
}

void Insert::currfile(const string& s) {
	_currfile = StringPointer(s);
}

void Insert::currfile(string* s) {
	_currfile = s;
}

void Insert::partial(Function* f) const {
	f->partial(true);
}

void Insert::makeLFD(FixpDef* d, bool lfp) const {
	if (d)
		d->lfp(lfp);
}

void Insert::addRule(FixpDef* d, Rule* r) const {
	if (d && r)
		d->add(r);
}

void Insert::addDef(FixpDef* d, FixpDef* sd) const {
	if (d && sd)
		d->add(sd);
}

FixpDef* Insert::createFD() const {
	return new FixpDef;
}

ParseInfo Insert::parseinfo(YYLTYPE l) const {
	return ParseInfo(l.first_line, l.first_column, _currfile);
}

FormulaParseInfo Insert::formparseinfo(Formula* f, YYLTYPE l) const {
	return FormulaParseInfo(l.first_line, l.first_column, _currfile, *f);
}

TermParseInfo Insert::termparseinfo(Term* t, YYLTYPE l) const {
	return TermParseInfo(l.first_line, l.first_column, _currfile, *t);
}

TermParseInfo Insert::termparseinfo(Term* t, const ParseInfo& l) const {
	return TermParseInfo(l.linenumber(), l.columnnumber(), l.filename(), *t);
}

SetParseInfo Insert::setparseinfo(SetExpr* s, YYLTYPE l) const {
	return SetParseInfo(l.first_line, l.first_column, _currfile, *s);
}

set<Variable*> Insert::freevars(const ParseInfo& pi) {
	std::set<Variable*> vv;
	string vs;
	for (auto i = _curr_vars.cbegin(); i != _curr_vars.cend(); ++i) {
		vv.insert(i->_var);
		vs = vs + ' ' + i->_name;
	}
	if (not vv.empty() && getOption(BoolType::SHOWWARNINGS)) {
		Warning::freevars(vs, pi);
	}
	_curr_vars.clear();
	return vv;
}

void Insert::remove_vars(const std::vector<Variable*>& v) {
	for (auto it = v.cbegin(); it != v.cend(); ++it) {
		for (auto i = _curr_vars.begin(); i != _curr_vars.end(); ++i) {
			if (i->_name == (*it)->name()) {
				_curr_vars.erase(i);
				break;
			}
		}
	}
}

void Insert::remove_vars(const std::set<Variable*>& v) {
	for (auto it = v.cbegin(); it != v.cend(); ++it) {
		for (auto i = _curr_vars.begin(); i != _curr_vars.end(); ++i) {
			if (i->_name == (*it)->name()) {
				_curr_vars.erase(i);
				break;
			}
		}
	}
}

void Insert::usenamespace(Namespace* s) {
	++_nrspaces.back();
	_usingspace.push_back(s);
}

void Insert::usevocabulary(Vocabulary* v) {
	bool found = false;
	for (auto i = _usingvocab.cbegin(); i < _usingvocab.cend() && not found; ++i) {
		if (*i == v) {
			found = true;
		}
	}
	if (not found) {
		++_nrvocabs.back();
		_usingvocab.push_back(v);
	}
}

void Insert::usingvocab(const longname& vs, YYLTYPE l) {
	auto pi = parseinfo(l);
	auto v = vocabularyInScope(vs, pi);
	if (v) {
		usevocabulary(v);
	} else {
		notDeclared(ComponentType::Vocabulary, toString(vs), pi);
	}
}

void Insert::usingspace(const longname& vs, YYLTYPE l) {
	auto pi = parseinfo(l);
	auto s = namespaceInScope(vs, pi);
	if (s) {
		usenamespace(s);
	} else {
		notDeclared(ComponentType::Namespace, toString(vs), pi);
	}
}

void Insert::openblock() {
	_nrvocabs.push_back(0);
	_nrspaces.push_back(0);
}

void Insert::closeblock() {
	for (unsigned int n = 0; n < _nrvocabs.back(); ++n) {
		_usingvocab.pop_back();
	}
	for (unsigned int n = 0; n < _nrspaces.back(); ++n) {
		_usingspace.pop_back();
	}
	_nrvocabs.pop_back();
	_nrspaces.pop_back();
	_currvocabulary = 0;
	_currtheory = 0;
	_currstructure = 0;
	_currprocedure = 0;
	_currquery = "";
	_currterm = "";
}

Namespace* findNamespace(const std::string& name, Namespace* ns) {
	if (ns->name() == name) {
		return ns;
	}
	for (auto i = ns->subspaces().cbegin(); i != ns->subspaces().cend(); ++i) {
		auto result = findNamespace(name, i->second);
		if (result != NULL) {
			return result;
		}
	}
	return NULL;
}

void Insert::openNamespace(const string& sname, YYLTYPE l) {
	openblock();
	auto pi = parseinfo(l);
	auto globalns = getGlobal()->getGlobalNamespace();
	auto ns = findNamespace(sname, globalns);
	if (ns == NULL) {
		ns = new Namespace(sname, _currspace, pi);
	}
	_currspace = ns;
	usenamespace(ns);
}

void Insert::closeNamespace() {
	LuaConnection::checkedAddToGlobal(_currspace);
	_currspace = _currspace->super();
	Assert(_currspace);
	closeblock();
}

void Insert::openvocab(const string& vname, YYLTYPE l) {
	openblock();
	auto pi = parseinfo(l);
	auto v = vocabularyInScope(vname, pi);
	if (v) {
		declaredEarlier(ComponentType::Vocabulary, vname, pi, v->pi());
	}
	_currvocabulary = new Vocabulary(vname, pi);
	_currspace->add(_currvocabulary);
	usevocabulary(_currvocabulary);
}

void Insert::assignvocab(InternalArgument* arg, YYLTYPE l) {
	auto v = LuaConnection::vocabulary(arg);
	if (v) {
		_currvocabulary->add(v);
	} else {
		expected(ComponentType::Vocabulary, parseinfo(l));
	}
}

void Insert::closevocab() {
	Assert(_currvocabulary);
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(_currvocabulary);
	}
	closeblock();
}

void Insert::setvocab(const longname& vs, YYLTYPE l) {
	auto pi = parseinfo(l);
	auto v = vocabularyInScope(vs, pi);
	if (v != NULL) {
		usevocabulary(v);
		_currvocabulary = v;
		if (_currstructure) {
			_currstructure->changeVocabulary(v);
		} else if (_currtheory) {
			_currtheory->vocabulary(v);
		}
	} else {
		notDeclared(ComponentType::Vocabulary, toString(vs), pi);
		_currvocabulary = Vocabulary::std();
		if (_currstructure) {
			_currstructure->changeVocabulary(Vocabulary::std());
		} else if (_currtheory) {
			_currtheory->vocabulary(Vocabulary::std());
		}
	}
}

void Insert::externvocab(const vector<string>& vname, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vname, pi);
	if (v) {
		_currvocabulary->add(v);
	} else {
		notDeclared(ComponentType::Vocabulary, toString(vname), pi);
	}
}

void Insert::openquery(const string& qname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Query* q = queryInScope(qname, pi);
	_currquery = qname;
	if (q) {
		declaredEarlier(ComponentType::Query, qname, pi, q->pi());
	}
}

void Insert::openterm(const string& tname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Term* t = termInScope(tname, pi);
	_currterm = tname;
	if (t) {
		declaredEarlier(ComponentType::Term, tname, pi, t->pi());
	}
}

void Insert::opentheory(const string& tname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	AbstractTheory* t = theoryInScope(tname, pi);
	if (t) {
		declaredEarlier(ComponentType::Theory, tname, pi, t->pi());
	}
	_currtheory = new Theory(tname, pi);
	_currspace->add(_currtheory);
}

void Insert::assigntheory(InternalArgument* arg, YYLTYPE l) {
	AbstractTheory* t = LuaConnection::theory(arg);
	if (t) {
		_currtheory->addTheory(t);
	} else {
		expected(ComponentType::Theory, parseinfo(l));
	}
}

void Insert::closetheory() {
	Assert(_currtheory);
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(_currtheory);
	}
	closeblock();
}

void Insert::closequery(Query* q) {
	_curr_vars.clear();
	if (q != NULL) { // Allows for better error catching
		std::set<Variable*> sv(q->variables().cbegin(), q->variables().cend());
		QuantForm* qf = new QuantForm(SIGN::POS, QUANT::UNIV, sv, q->query(), FormulaParseInfo());
		FormulaUtils::deriveSorts(_currvocabulary, qf);
		FormulaUtils::checkSorts(_currvocabulary, qf);
		delete (qf); //No recursive delete, the rest of the query should still exist!
		_currspace->add(_currquery, q);
		if (_currspace->isGlobal()) {
			LuaConnection::addGlobal(_currquery, q);
		}
	}
	closeblock();
}

void Insert::closeterm(Term* t) {
	_curr_vars.clear();
	if (t != NULL) { // Allows for better error catching
		TermUtils::deriveSorts(_currvocabulary, t);
		TermUtils::checkSorts(_currvocabulary, t);
		_currspace->add(_currterm, t);
		if (_currspace->isGlobal()) {
			LuaConnection::addGlobal(_currterm, t);
		}
		t->name(_currterm);
		t->vocabulary(_currvocabulary);
	}
	closeblock();
}

void Insert::openstructure(const string& sname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	AbstractStructure* s = structureInScope(sname, pi);
	if (s) {
		declaredEarlier(ComponentType::Structure, sname, pi, s->pi());
	}
	_currstructure = new Structure(sname, pi);
	_currspace->add(_currstructure);
}

void Insert::assignstructure(InternalArgument* arg, YYLTYPE l) {
	AbstractStructure* s = LuaConnection::structure(arg);
	if (s)
		_currstructure->addStructure(s);
	else {
		expected(ComponentType::Structure, parseinfo(l));
	}
}

void Insert::closestructure() {
	Assert(_currstructure);
	assignunknowntables();
	if (getOption(BoolType::AUTOCOMPLETE)) {
		_currstructure->autocomplete();
	}
	_currstructure->sortCheck(); // TODO also add to commands?
	_currstructure->functionCheck();
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(_currstructure);
	}
	closeblock();
}

void Insert::openprocedure(const string& name, YYLTYPE l) {
	openblock();
	auto pi = parseinfo(l);
	auto p = procedureInScope(name, pi);
	if (p) {
		declaredEarlier(ComponentType::Procedure, name, pi, p->pi());
	}
	_currprocedure = new UserProcedure(name, pi, l.descr);
	_currspace->add(_currprocedure);

	// Include all internal blocks as local objects in the scope of the procedure
	for (auto it = _usingspace.cbegin(); it != _usingspace.cend(); ++it) {
		if ((*it)->isGlobal()) {
			continue;
		}
		stringstream sstr;
		auto componentnames = (*it)->getComponentNamesExceptSpaces();
		for (auto jt = componentnames.cbegin(); jt != componentnames.cend(); ++jt) {
			sstr << "local " << *jt << " = ";
			(*it)->putLuaName(sstr);
			sstr << '.' << *jt << '\n';
		}
		_currprocedure->add(sstr.str());
	}
}

void Insert::closeprocedure(stringstream* chunk) {
	_currprocedure->add(chunk->str());
	LuaConnection::compile(_currprocedure);
	if (_currspace->isGlobal() || _currspace == getGlobal()->getStdNamespace() || _currspace->hasParent(getGlobal()->getStdNamespace())) {
		LuaConnection::addGlobal(_currprocedure);
	}
	closeblock();
}

Sort* Insert::sort(Sort* s) const {
	if (s) {
		_currvocabulary->add(s);
	}
	return s;
}

Predicate* Insert::predicate(Predicate* p) const {
	if (p) {
		_currvocabulary->add(p);
	}
	return p;
}

Function* Insert::function(Function* f) const {
	if (f) {
		_currvocabulary->add(f);
	}
	return f;
}

Sort* Insert::sortpointer(const longname& vs, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Sort* s = sortInScope(vs, pi);
	if (!s) {
		notDeclared(ComponentType::Sort, toString(vs), pi);
	}
	return s;
}

Predicate* Insert::predpointer(longname& vs, int arity, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	vs.back() = vs.back() + '/' + convertToString(arity);
	Predicate* p = predInScope(vs, pi);
	if (!p) {
		notDeclared(ComponentType::Predicate, toString(vs), pi);
	}
	return p;
}

Predicate* Insert::predpointer(longname& vs, const vector<Sort*>& va, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	longname copyvs = vs;
	Predicate* p = predpointer(copyvs, va.size(), l);
	if (p) {
		p = p->resolve(va);
	}
	if (!p) {
		notDeclared(ComponentType::Predicate, predName(vs, va), pi);
	}
	return p;
}

Function* Insert::funcpointer(longname& vs, int arity, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	vs.back() = vs.back() + '/' + convertToString(arity);
	Function* f = funcInScope(vs, pi);
	if (!f) {
		notDeclared(ComponentType::Function, toString(vs), pi);
	}
	return f;
}

Function* Insert::funcpointer(longname& vs, const vector<Sort*>& va, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	longname copyvs = vs;
	Function* f = funcpointer(copyvs, va.size() - 1, l);
	if (f) {
		f = f->resolve(va);
	}
	if (!f) {
		notDeclared(ComponentType::Function, funcName(vs, va), pi);
	}
	return f;
}

NSPair* Insert::internpredpointer(const vector<string>& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return new NSPair(name, sorts, false, pi);
}

NSPair* Insert::internfuncpointer(const vector<string>& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	NSPair* nsp = new NSPair(name, insorts, false, pi);
	nsp->_sorts.push_back(outsort);
	nsp->_func = true;
	return nsp;
}

NSPair* Insert::internpointer(const vector<string>& name, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return new NSPair(name, false, pi);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name		the name of the new sort	
 * \param sups		the supersorts of the new sort
 * \param subs		the subsorts of the new sort
 */
Sort* Insert::sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);

	// Create the sort
	Sort* s = new Sort(name, pi);

	// Add the sort to the current vocabulary
	if (_currvocabulary->contains(s)) {
		error("An identical sort already exists " + name);
	} else {
		_currvocabulary->add(s);
	}

	// Collect the ancestors of all super- and subsorts
	vector<std::set<Sort*> > supsa(sups.size());
	vector<std::set<Sort*> > subsa(subs.size());
	for (unsigned int n = 0; n < sups.size(); ++n) {
		if (sups[n]) {
			supsa[n] = sups[n]->ancestors(0);
			supsa[n].insert(sups[n]);
		}
	}
	for (unsigned int n = 0; n < subs.size(); ++n) {
		if (subs[n]) {
			subsa[n] = subs[n]->ancestors(0);
			subsa[n].insert(subs[n]);
		}
	}

	// Add the supersorts
	for (unsigned int n = 0; n < sups.size(); ++n) {
		if (sups[n]) {
			for (unsigned int m = 0; m < subs.size(); ++m) {
				if (subs[m]) {
					if (subsa[m].find(sups[n]) == subsa[m].cend()) {
						notsubsort(subs[m]->name(), sups[n]->name(), parseinfo(l));
					}
				}
			}
			s->addParent(sups[n]);
		}
	}

	// Add the subsorts
	for (unsigned int n = 0; n < subs.size(); ++n) {
		if (subs[n]) {
			for (unsigned int m = 0; m < sups.size(); ++m) {
				if (sups[m]) {
					if (supsa[m].find(subs[n]) != supsa[m].cend()) {
						cyclichierarchy(subs[n]->name(), sups[m]->name(), parseinfo(l));
					}
				}
			}
			s->addChild(subs[n]);
		}
	}
	return s;
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name	the name of the sort
 */
Sort* Insert::sort(const string& name, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return sort(name, vs, vs, l);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name		the name of the sort
 * \param supbs		the super- or subsorts of the sort
 * \param super		true if supbs are the supersorts, false if supbs are the subsorts
 */
Sort* Insert::sort(const string& name, const vector<Sort*> supbs, bool super, YYLTYPE l) const {
	vector<Sort*> vs(0);
	if (super) {
		return sort(name, supbs, vs, l);
	} else {
		return sort(name, vs, supbs, l);
	}
}

Predicate* Insert::predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	string nar = string(name) + '/' + convertToString(sorts.size());
	for (size_t n = 0; n < sorts.size(); ++n) {
		if (sorts[n] == NULL) {
			return NULL;
		}
	}
	Predicate* p = new Predicate(nar, sorts, pi);
	if (_currvocabulary->containsOverloaded(p)) {
		auto oldp = _currvocabulary->pred(p->name());
		auto existsp = oldp->resolve(sorts);
		if (existsp != NULL) {
			error("An identical predicate already exists " + name);
		}
	}
	_currvocabulary->add(p);
	return p;
}

Predicate* Insert::predicate(const string& name, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return predicate(name, vs, l);
}

Function* Insert::function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	string nar = string(name) + '/' + convertToString(insorts.size());
	for (size_t n = 0; n < insorts.size(); ++n) {
		if (insorts[n] == NULL) {
			return NULL;
		}
	}
	if (outsort == NULL) {
		return NULL;
	}
	Function* f = new Function(nar, insorts, outsort, pi);
	if (_currvocabulary->containsOverloaded(f)) {
		auto oldf = _currvocabulary->func(f->name());
		auto v = insorts;
		v.push_back(outsort);
		auto existsf = oldf->resolve(v);
		if (existsf != NULL) {
			error("An identical function already exists " + name);
		}
	}
	_currvocabulary->add(f);
	return f;
}

Function* Insert::function(const string& name, Sort* outsort, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return function(name, vs, outsort, l);
}

Function* Insert::aritfunction(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	for (size_t n = 0; n < sorts.size(); ++n) {
		if (sorts[n] == NULL) {
			return NULL;
		}
	}
	Function* orig = _currvocabulary->func(name);
	unsigned int binding = orig ? orig->binding() : 0;
	Function* f = new Function(name, sorts, pi, binding);
	_currvocabulary->add(f);
	return f;
}

InternalArgument* Insert::call(const longname& proc, const vector<longname>& args, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return LuaConnection::call(proc, args, pi);
}

InternalArgument* Insert::call(const longname& proc, YYLTYPE l) const {
	vector<longname> vl(0);
	return call(proc, vl, l);
}

void Insert::definition(Definition* d) const {
	if (d != NULL) {
		_currtheory->add(d);
	}
}

void Insert::sentence(Formula* f) {
	if (f == NULL) {
		_curr_vars.clear();
		return;
	}

	// 1. Quantify the free variables universally
	auto vv = freevars(f->pi());
	if (not vv.empty()) {
		f = new QuantForm(SIGN::POS, QUANT::UNIV, vv, f, f->pi());
	}

	// 2. Sort derivation & checking
	FormulaUtils::deriveSorts(_currvocabulary, f);
	FormulaUtils::checkSorts(_currvocabulary, f);

	// 3. Add the formula to the current theory
	_currtheory->add(f);

}

void Insert::fixpdef(FixpDef* d) const {
	if (d) {
		_currtheory->add(d);
	}
}

Definition* Insert::definition(const vector<Rule*>& rules) const {
	Definition* d = new Definition();
	for (size_t n = 0; n < rules.size(); ++n) {
		if (rules[n]) {
			d->add(rules[n]);
		}
	}
	return d;
}

Rule* Insert::rule(const std::set<Variable*>& qv, Formula* head, Formula* body, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	remove_vars(qv);
	if (head && body) {
		// Quantify the free variables
		std::set<Variable*> vv = freevars(head->pi());
		remove_vars(vv);
		// Split quantified variables in head and body variables
		std::set<Variable*> hv, bv;
		for (auto it = qv.cbegin(); it != qv.cend(); ++it) {
			if (head->contains(*it)) {
				hv.insert(*it);
			} else {
				bv.insert(*it);
			}
		}
		for (auto it = vv.cbegin(); it != vv.cend(); ++it) {
			if (head->contains(*it)) {
				hv.insert(*it);
			} else {
				bv.insert(*it);
			}
		}
		// Create a new rule
		if (not bv.empty()) {
			body = new QuantForm(SIGN::POS, QUANT::EXIST, bv, body, FormulaParseInfo((body->pi())));
		}
		Assert(isa<PredForm>(*head));
		auto pfhead = dynamic_cast<PredForm*>(head);
		auto r = new Rule(hv, pfhead, body, pi);
		// Sort derivation
		DefinitionUtils::deriveSorts(_currvocabulary, r);
		DefinitionUtils::checkSorts(_currvocabulary, r);
		// Return the rule
		return r;
	} else {
		_curr_vars.clear();
		if (head) {
			head->recursiveDelete();
		}
		if (body) {
			body->recursiveDelete();
		}
		for (auto it = qv.cbegin(); it != qv.cend(); ++it) {
			delete (*it);
		}
		return NULL;
	}
}

Rule* Insert::rule(const std::set<Variable*>& qv, Formula* head, YYLTYPE l) {
	return rule(qv, head, FormulaUtils::trueFormula(), l);
}

Formula* Insert::trueform(YYLTYPE l) const {
	vector<Formula*> vf(0);
	// FIXME implement deep clone of formula to prevent having double calls here (the first one just saves an original copy to refer to later)
	auto temp = new BoolForm(SIGN::POS, true, vf, FormulaParseInfo());
	FormulaParseInfo pi = formparseinfo(temp, l);
	temp->recursiveDelete();
	return new BoolForm(SIGN::POS, true, vf, pi);
}

Formula* Insert::falseform(YYLTYPE l) const {
	vector<Formula*> vf(0);
	auto temp = new BoolForm(SIGN::POS, false, vf, FormulaParseInfo());
	FormulaParseInfo pi = formparseinfo(temp, l);
	temp->recursiveDelete();
	return new BoolForm(SIGN::POS, false, vf, pi);
}

Formula* Insert::predform(NSPair* nst, const vector<Term*>& vt, YYLTYPE l) const {
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vt.size()) {
			incompatiblearity(toString(nst), nst->_pi);
		}
		if (nst->_func) {
			prednameexpected(nst->_pi);
		}
	}
	nst->includeArity(vt.size());
	Predicate* p = predInScope(nst->_name, nst->_pi);
	if (p && nst->_sortsincluded && (nst->_sorts).size() == vt.size()) {
		p = p->resolve(nst->_sorts);
	}

	PredForm* pf = NULL;
	if (p) {
		if (belongsToVoc(p)) {
			size_t n = 0;
			for (; n < vt.size(); ++n) {
				if (vt[n] == NULL) {
					break;
				}
			}
			if (n == vt.size()) {
				vector<Term*> vtpi;
				for (auto it = vt.cbegin(); it != vt.cend(); ++it) {
					/*if ((*it)->pi().originalobject())
					 vtpi.push_back((*it)->pi().originalobject()->clone());
					 else*/
					vtpi.push_back((*it)->clone());
				}
				PredForm* pipf = new PredForm(SIGN::POS, p, vtpi, FormulaParseInfo());
				FormulaParseInfo pi = formparseinfo(pipf, l);
				pipf->recursiveDelete(); //the needed things for the pi are cloned
				pf = new PredForm(SIGN::POS, p, vt, pi);
			}
		} else {
			notInVocabularyOf(ComponentType::Predicate, ComponentType::Theory, p->name(), _currtheory->name(), nst->_pi);
		}
	} else {
		notDeclared(ComponentType::Predicate, toString(nst), nst->_pi);
	}

	// Cleanup
	if (pf == NULL) {
		for (size_t n = 0; n < vt.size(); ++n) {
			if (vt[n]) {
				vt[n]->recursiveDelete();
			}
		}
	}
	delete (nst);

	return pf;
}

Formula* Insert::predform(NSPair* t, YYLTYPE l) const {
	vector<Term*> vt(0);
	return predform(t, vt, l);
}

// NOTE: The lefthand functon is considered defined!
Formula* Insert::equalityhead(Term* left, Term* right, YYLTYPE l) const {
	if (left == NULL or right == NULL) {
		return NULL;
	}
	if (not isa<FuncTerm>(*left)) {
		funcnameexpected(left->pi());
		return NULL;
	}
	auto functerm = dynamic_cast<FuncTerm*>(left);
	vector<Term*> vt2(left->subterms());
	vt2.push_back(right);
	vector<Term*> vtpi;
	for (auto it = vt2.cbegin(); it != vt2.cend(); ++it) {
		/*if ((*it)->pi().originalobject()) {
		 vtpi.push_back((*it)->pi().originalobject()->clone());
		 } else {*/
		vtpi.push_back((*it)->clone());
		//}
	}
	auto temp = new PredForm(SIGN::POS, functerm->function(), vtpi, FormulaParseInfo());
	FormulaParseInfo pi = formparseinfo(temp, l);
	temp->recursiveDelete();
	return new PredForm(SIGN::POS, functerm->function(), vt2, pi);
}

Formula* Insert::funcgraphform(NSPair* nst, const vector<Term*>& vt, Term* t, YYLTYPE l) const {
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vt.size() + 1) {
			incompatiblearity(toString(nst), nst->_pi);
		}
		if (not nst->_func) {
			funcnameexpected(nst->_pi);
		}
	}
	nst->includeArity(vt.size());
	Function* f = funcInScope(nst->_name, nst->_pi);
	if (f && nst->_sortsincluded && (nst->_sorts).size() == vt.size() + 1) {
		f = f->resolve(nst->_sorts);
	}

	PredForm* pf = NULL;
	if (f) {
		if (belongsToVoc(f)) {
			size_t n = 0;
			for (; n < vt.size(); ++n) {
				if (vt[n] == NULL) {
					break;
				}
			}
			if (n == vt.size() && t) {
				vector<Term*> vt2(vt);
				vt2.push_back(t);
				vector<Term*> vtpi;
				for (auto it = vt2.cbegin(); it != vt2.cend(); ++it) {
					/*if ((*it)->pi().originalobject())
					 vtpi.push_back((*it)->pi().originalobject()->clone());
					 else*/
					vtpi.push_back((*it)->clone());
				}
				auto temp = new PredForm(SIGN::POS, f, vtpi, FormulaParseInfo());
				FormulaParseInfo pi = formparseinfo(temp, l);
				temp->recursiveDelete();
				pf = new PredForm(SIGN::POS, f, vt2, pi);
			}
		} else {
			notInVocabularyOf(ComponentType::Function, ComponentType::Theory, f->name(), _currtheory->name(), nst->_pi);
		}
	} else {
		notDeclared(ComponentType::Function, toString(nst), nst->_pi);
	}

	// Cleanup
	if (pf == NULL) {
		for (size_t n = 0; n < vt.size(); ++n) {
			if (vt[n]) {
				delete (vt[n]);
			}
		}
		if (t) {
			delete (t);
		}
	}
	delete (nst);

	return pf;
}

Formula* Insert::funcgraphform(NSPair* nst, Term* t, YYLTYPE l) const {
	vector<Term*> vt;
	return funcgraphform(nst, vt, t, l);
}

Formula* Insert::equivform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if (lf && rf) {
		Formula* lfpi = lf->clone();
		Formula* rfpi = rf->clone();
		auto temp = new EquivForm(SIGN::POS, lfpi, rfpi, FormulaParseInfo());
		FormulaParseInfo pi = formparseinfo(temp, l);
		temp->recursiveDelete();
		return new EquivForm(SIGN::POS, lf, rf, pi);
	} else {
		if (lf) {
			lf->recursiveDelete();
		}
		if (rf) {
			rf->recursiveDelete();
		}
		return 0;
	}
}

Formula* Insert::boolform(bool conj, Formula* lf, Formula* rf, YYLTYPE l) const {
	if (lf && rf) {
		vector<Formula*> vf(2);
		vector<Formula*> pivf(2);
		vf[0] = lf;
		vf[1] = rf;
		pivf[0] = lf->clone();
		pivf[1] = rf->clone();
		auto tempbf = new BoolForm(SIGN::POS, conj, pivf, FormulaParseInfo());
		FormulaParseInfo pi = formparseinfo(tempbf, l);
		//All necessary things from tempbf are cloned
		tempbf->recursiveDelete();
		return new BoolForm(SIGN::POS, conj, vf, pi);
	} else {
		if (lf) {
			lf->recursiveDelete();
		}
		if (rf) {
			rf->recursiveDelete();
		}
		return 0;
	}
}

Formula* Insert::disjform(Formula* lf, Formula* rf, YYLTYPE l) const {
	return boolform(false, lf, rf, l);
}

Formula* Insert::conjform(Formula* lf, Formula* rf, YYLTYPE l) const {
	return boolform(true, lf, rf, l);
}

Formula* Insert::implform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if (lf)
		lf->negate();
	return boolform(false, lf, rf, l);
}

Formula* Insert::revimplform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if (rf)
		rf->negate();
	return boolform(false, rf, lf, l);
}

Formula* Insert::quantform(bool univ, const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	remove_vars(vv);
	if (f) {
		std::set<Variable*> pivv;
		map<Variable*, Variable*> mvv;
		for (auto it = vv.cbegin(); it != vv.cend(); ++it) {
			Variable* v = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
			pivv.insert(v);
			mvv[*it] = v;
		}
		Formula* pif = f->clone(mvv);
		QUANT quant = univ ? QUANT::UNIV : QUANT::EXIST;
		auto tempqf = new QuantForm(SIGN::POS, quant, pivv, pif, FormulaParseInfo());
		FormulaParseInfo pi = formparseinfo(tempqf, l);
		//All necessary things from tempqf are cloned
		tempqf->recursiveDelete();
		return new QuantForm(SIGN::POS, quant, vv, f, pi);
	} else {
		for (auto it = vv.cbegin(); it != vv.cend(); ++it)
			delete (*it);
		return 0;
	}
}

Formula* Insert::univform(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	return quantform(true, vv, f, l);
}

Formula* Insert::existform(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	return quantform(false, vv, f, l);
}

Formula* Insert::bexform(CompType c, int bound, const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	if (f == NULL) {
		return f;
	}
	auto aggterm = dynamic_cast<AggTerm*>(aggregate(AggFunction::CARD, set(vv, f, l), l));
	auto boundterm = domterm(bound, l);

	// Create parseinfo (TODO UGLY!)
	auto temp = new AggForm(SIGN::POS, boundterm->clone(), invertComp(c), aggterm->clone(), FormulaParseInfo());
	auto pi = formparseinfo(temp, l);
	temp->recursiveDelete();

	return new AggForm(SIGN::POS, boundterm, invertComp(c), aggterm, pi);
}

void Insert::negate(Formula* f) const {
	if (f != NULL) {
		f->negate();
	}
}

Formula* Insert::eqchain(CompType c, Formula* f, Term* t, YYLTYPE) const {
	if (f && t) {
		Assert(isa<EqChainForm>(*f));
		auto ecf = dynamic_cast<EqChainForm*>(f);
		ecf->add(c, t);
	}
	return f;
}

Formula* Insert::eqchain(CompType c, Term* left, Term* right, YYLTYPE l) const {
	if (left && right) {
		auto leftpi = left->clone();
		auto rightpi = right->clone();
		auto ecfpi = new EqChainForm(SIGN::POS, true, leftpi, FormulaParseInfo());
		ecfpi->add(c, rightpi);
		auto fpi = formparseinfo(ecfpi, l);
		auto ecf = new EqChainForm(SIGN::POS, true, left, fpi);
		ecf->add(c, right);
		ecfpi->recursiveDelete();
		return ecf;
	} else {
		return NULL;
	}
}

Variable* Insert::quantifiedvar(const string& name, YYLTYPE l) {
	auto othervar = getVar(name);
	if(getVar(name) != NULL){
		declaredEarlier(ComponentType::Variable,name,parseinfo(l),othervar->pi());
	}
	auto pi = parseinfo(l);
	auto v = new Variable(name, 0, pi);
	_curr_vars.push_front(VarName(name, v));
	return v;
}

Variable* Insert::quantifiedvar(const string& name, Sort* sort, YYLTYPE l) {
	auto v = quantifiedvar(name, l);
	if (sort) {
		v->sort(sort);
	}
	return v;
}

Sort* Insert::theosortpointer(const vector<string>& vs, YYLTYPE l) const {
	auto s = sortpointer(vs, l);
	if (s) {
		if (belongsToVoc(s)) {
			return s;
		} else {
			ParseInfo pi = parseinfo(l);
			string uname = toString(vs);
			if (_currtheory) {
				notInVocabularyOf(ComponentType::Sort, ComponentType::Theory, uname, _currtheory->name(), pi);
			} else if (_currstructure) {
				notInVocabularyOf(ComponentType::Sort, ComponentType::Structure, uname, _currstructure->name(), pi);
			}
			return NULL;
		}
	} else {
		return NULL;
	}
}

FuncTerm* Insert::functerm(NSPair* nst, const vector<Term*>& vt) {
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vt.size() + 1) {
			incompatiblearity(toString(nst), nst->_pi);
		}
		if (not nst->_func) {
			funcnameexpected(nst->_pi);
		}
	}
	nst->includeArity(vt.size());
	Function* f = funcInScope(nst->_name, nst->_pi);
	if (f != NULL && nst->_sortsincluded && (nst->_sorts).size() == vt.size() + 1) {
		f = f->resolve(nst->_sorts);
	}

	FuncTerm* t = NULL;
	if (f != NULL) {
		if (belongsToVoc(f)) {
			size_t n = 0;
			for (; n < vt.size(); ++n) {
				if (vt[n] == NULL)
					break;
			}
			if (n == vt.size()) {
				vector<Term*> vtpi;
				for (auto it = vt.cbegin(); it != vt.cend(); ++it) {
					/*if ((*it)->pi().originalobject()) {
					 vtpi.push_back((*it)->pi().originalobject()->clone());
					 } else {*/
					vtpi.push_back((*it)->clone());
					//}
				}
				auto temp = new FuncTerm(f, vtpi, TermParseInfo());
				TermParseInfo pi = termparseinfo(temp, nst->_pi);
				temp->recursiveDelete();
				t = new FuncTerm(f, vt, pi);
			}
		} else {
			notInVocabularyOf(ComponentType::Function, ComponentType::Theory, f->name(), _currtheory->name(), nst->_pi);
		}
	} else {
		notDeclared(ComponentType::Function, toString(nst), nst->_pi);
	}

	// Cleanup
	if (t == NULL) {
		for (size_t n = 0; n < vt.size(); ++n) {
			if (vt[n] != NULL) {
				delete (vt[n]);
			}
		}
	}
	delete (nst);

	return t;
}

Variable* Insert::getVar(const string& name) const {
	for (auto i = _curr_vars.cbegin(); i != _curr_vars.cend(); ++i) {
		if (name == i->_name) {
			return i->_var;
		}
	}
	return NULL;
}

Term* Insert::term(NSPair* nst) {
	if (nst->_sortsincluded || (nst->_name).size() != 1) {
		vector<Term*> vt = vector<Term*>(0);
		return functerm(nst, vt);
	} else {
		Term* t = NULL;
		string name = (nst->_name)[0];
		Variable* v = getVar(name);
		nst->includeArity(0);
		Function* f = funcInScope(nst->_name, nst->_pi);
		if (v != NULL) {
			if (f != NULL) {
				Warning::varcouldbeconst((nst->_name)[0], nst->_pi);
			}
			auto temp = new VarTerm(v, TermParseInfo());
			t = new VarTerm(v, termparseinfo(temp, nst->_pi));
			temp->recursiveDelete();
			delete (nst);
		} else if (f != NULL) {
			vector<Term*> vt(0);
			nst->_name = vector<string>(1, name);
			nst->_arityincluded = false;
			t = functerm(nst, vt);
		} else {
			YYLTYPE l;
			l.first_line = (nst->_pi).linenumber();
			l.first_column = (nst->_pi).columnnumber();
			v = quantifiedvar(name, l);
			auto temp = new VarTerm(v, TermParseInfo());
			t = new VarTerm(v, termparseinfo(temp, nst->_pi));
			temp->recursiveDelete();
			delete (nst);
		}
		return t;
	}
}

FuncTerm* Insert::arterm(char c, Term* lt, Term* rt, YYLTYPE l) const {
	if (lt && rt) {
		Function* f = _currvocabulary->func(string(1, c) + "/2");
		Assert(f);
		vector<Term*> vt = { lt, rt };
		vector<Term*> pivt(2);
		pivt[0] = lt->clone();
		pivt[1] = rt->clone();
		auto temp = new FuncTerm(f, pivt, TermParseInfo());
		auto pi = termparseinfo(temp, l);
		temp->recursiveDelete();
		return new FuncTerm(f, vt, pi);
	} else {
		if (lt) {
			lt->recursiveDelete();
		}
		if (rt) {
			rt->recursiveDelete();
		}
		return NULL;
	}
}

FuncTerm* Insert::arterm(const string& s, Term* t, YYLTYPE l) const {
	if (t == NULL) {
		t->recursiveDelete();
		return NULL;
	}
	Function* f = _currvocabulary->func(s + "/1");
	Assert(f);
	vector<Term*> vt(1, t);
	vector<Term*> pivt(1, t->clone());
	auto temp = new FuncTerm(f, pivt, TermParseInfo());
	auto res = new FuncTerm(f, vt, termparseinfo(temp, l));
	temp->recursiveDelete();
	return res;
}

DomainTerm* Insert::domterm(int i, YYLTYPE l) const {
	const DomainElement* d = createDomElem(i);
	Sort* s = (i >= 0 ? get(STDSORT::NATSORT) : get(STDSORT::INTSORT));
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

DomainTerm* Insert::domterm(double f, YYLTYPE l) const {
	const DomainElement* d = createDomElem(f);
	Sort* s = get(STDSORT::FLOATSORT);
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

DomainTerm* Insert::domterm(std::string* e, YYLTYPE l) const {
	const DomainElement* d = createDomElem(e);
	Sort* s = get(STDSORT::STRINGSORT);
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

DomainTerm* Insert::domterm(char c, YYLTYPE l) const {
	const DomainElement* d = createDomElem(StringPointer(string(1, c)));
	Sort* s = get(STDSORT::CHARSORT);
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

DomainTerm* Insert::domterm(std::string* e, Sort* s, YYLTYPE l) const {
	const DomainElement* d = createDomElem(e);
	Assert(s != NULL);
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

AggTerm* Insert::aggregate(AggFunction f, EnumSetExpr* s, YYLTYPE l) const {
	if (s == NULL) {
		return NULL;
	}
	auto pis = s->clone();
	auto temp = new AggTerm(pis, f, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new AggTerm(s, f, pi);
}

Query* Insert::query(const std::vector<Variable*>& vv, Formula* f, YYLTYPE l) {
	remove_vars(vv);
	if (f) {
		ParseInfo pi = parseinfo(l);
		auto res = new Query(_currquery, vv, f, pi);
		res->vocabulary(_currvocabulary);
		return res;
	} else {
		for (auto it = vv.cbegin(); it != vv.cend(); ++it) {
			delete (*it);
		}
		return NULL;
	}
}

EnumSetExpr* Insert::set(const std::set<Variable*>& vv, Formula* f, Term* counter, YYLTYPE l) {
	remove_vars(vv);
	if (f && counter) {
		std::set<Variable*> pivv;
		map<Variable*, Variable*> mvv;
		for (auto it = vv.cbegin(); it != vv.cend(); ++it) {
			auto v = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
			pivv.insert(v);
			mvv[*it] = v;
		}
		auto picounter = counter->clone();
		auto pif = f->clone(mvv);
		auto temp = new QuantSetExpr(pivv, pif, picounter, SetParseInfo());
		auto pi = setparseinfo(temp, l);
		temp->recursiveDelete();
		return new EnumSetExpr( { new QuantSetExpr(vv, f, counter, pi) }, pi);
	} else {
		if (f) {
			f->recursiveDelete();
		}
		if (counter) {
			counter->recursiveDelete();
		}
		for (auto it = vv.cbegin(); it != vv.cend(); ++it) {
			delete (*it);
		}
		return NULL;
	}
}

EnumSetExpr* Insert::set(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	auto d = createDomElem(1);
	auto counter = new DomainTerm(get(STDSORT::NATSORT), d, TermParseInfo());
	return set(vv, f, counter, l);
}

EnumSetExpr* Insert::createEnum(YYLTYPE l) const {
	EnumSetExpr* pis = new EnumSetExpr(SetParseInfo());
	SetParseInfo pi = setparseinfo(pis, l);
	pis->recursiveDelete();
	return new EnumSetExpr(pi);
}

EnumSetExpr* Insert::addFT(EnumSetExpr* s, Formula* f, Term* t) const {
	if (f && s && t) {
		//SetExpr* orig = s->pi().originalobject();
		/*if (orig && typeid(*orig) == typeid(EnumSetExpr)) {
		 EnumSetExpr* origset = dynamic_cast<EnumSetExpr*>(orig);
		 Formula* pif = f->clone();
		 Term* tif = t->clone();
		 origset->addTerm(tif);
		 origset->addFormula(pif);
		 }*/
		auto set = new QuantSetExpr( { }, f, t, s->pi()); // TODO incorrect pi
		s->addSet(set);
	} else {
		if (f) {
			f->recursiveDelete();
		}
		if (s) {
			s->recursiveDelete();
			s = NULL;
		}
		if (t) {
			t->recursiveDelete();
		}
	}
	return s;
}

EnumSetExpr* Insert::addFormula(EnumSetExpr* s, Formula* f) const {
	auto d = createDomElem(1);
	auto t = new DomainTerm(get(STDSORT::NATSORT), d, TermParseInfo());
	return addFT(s, f, t);
}

void Insert::emptyinter(NSPair* nst) const {
	if (nst->_sortsincluded) {
		if (nst->_func) {
			auto ift = new EnumeratedInternalFuncTable();
			auto ft = new FuncTable(ift, TableUtils::fullUniverse(nst->_sorts.size()));
			funcinter(nst, ft);
		} else {
			auto ipt = new EnumeratedInternalPredTable();
			auto pt = new PredTable(ipt, TableUtils::fullUniverse(nst->_sorts.size()));
			predinter(nst, pt);
		}
	} else {
		ParseInfo pi = nst->_pi;
		auto vp = noArPredInScope(nst->_name, pi);
		if (vp.empty())
			notDeclared(ComponentType::Predicate, toString(nst), pi);
		else if (vp.size() > 1) {
			auto it = vp.cbegin();
			auto p1 = *it;
			++it;
			auto p2 = *it;
			overloaded(ComponentType::Predicate, toString(nst), p1->pi(), p2->pi(), pi);
		} else {
			auto ipt = new EnumeratedInternalPredTable();
			auto pt = new PredTable(ipt, TableUtils::fullUniverse((*(vp.cbegin()))->arity()));
			predinter(nst, pt);
		}
	}
}

void Insert::predinter(NSPair* nst, PredTable* t) const {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != t->arity()) {
			incompatiblearity(toString(nst), pi);
		}
		if (nst->_func) {
			prednameexpected(pi);
		}
	}
	nst->includeArity(t->arity());
	Predicate* p = predInScope(nst->_name, pi);
	if (p != NULL && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) {
		p = p->resolve(nst->_sorts);
	}
	if (p != NULL) {
		if (belongsToVoc(p)) {
			PredTable* nt = new PredTable(t->internTable(), _currstructure->universe(p));
			delete (t);
			PredInter* inter = _currstructure->inter(p);
			inter->ctpt(nt);
		} else {
			notInVocabularyOf(ComponentType::Predicate, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else {
		notDeclared(ComponentType::Predicate, toString(nst), pi);
	}
	delete (nst);
}

void Insert::funcinter(NSPair* nst, FuncTable* t) const {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != t->arity() + 1) {
			incompatiblearity(toString(nst), pi);
		}
		if (not nst->_func) {
			funcnameexpected(pi);
		}
	}
	nst->includeArity(t->arity());
	Function* f = funcInScope(nst->_name, pi);
	if (f && nst->_sortsincluded && (nst->_sorts).size() == t->arity() + 1) {
		f = f->resolve(nst->_sorts);
	}
	if (f) {
		if (belongsToVoc(f)) {
			FuncTable* nt = new FuncTable(t->internTable(), _currstructure->universe(f));
			delete (t);
			FuncInter* inter = _currstructure->inter(f);
			inter->funcTable(nt);
		} else {
			notInVocabularyOf(ComponentType::Function, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else {
		notDeclared(ComponentType::Function, toString(nst), pi);
	}
	delete (nst);
}

void Insert::constructor(NSPair* nst) const {
	ParseInfo pi = nst->_pi;
	Function* f = 0;
	if (nst->_sortsincluded) {
		if (not nst->_func) {
			funcnameexpected(pi);
		}
		nst->includeFuncArity();
		f = funcInScope(nst->_name, pi);
		if (f) {
			f = f->resolve(nst->_sorts);
		} else {
			notDeclared(ComponentType::Function, toString(nst), pi);
		}
	} else {
		std::set<Function*> vf = noArFuncInScope(nst->_name, pi);
		if (vf.empty()) {
			notDeclared(ComponentType::Function, toString(nst), pi);
		} else if (vf.size() > 1) {
			std::set<Function*>::const_iterator it = vf.cbegin();
			Function* f1 = *it;
			++it;
			Function* f2 = *it;
			overloaded(ComponentType::Function, toString(nst), f1->pi(), f2->pi(), pi);
		} else {
			f = *(vf.cbegin());
		}
	}
	if (f) {
		if (belongsToVoc(f)) {
			UNAInternalFuncTable* uift = new UNAInternalFuncTable(f);
			FuncTable* ft = new FuncTable(uift, _currstructure->universe(f));
			FuncInter* inter = _currstructure->inter(f);
			inter->funcTable(ft);
		} else {
			notInVocabularyOf(ComponentType::Function, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	}
}

void Insert::sortinter(NSPair* nst, SortTable* t) {
	ParseInfo pi = nst->_pi;
	longname name = nst->_name;
	auto s = sortInScope(name, pi);
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != 1) {
			incompatiblearity(toString(nst), pi);
		}
		if (nst->_func) {
			prednameexpected(pi);
		}
	}
	nst->includeArity(1);
	Predicate* p = predInScope(nst->_name, pi);
	if (p && nst->_sortsincluded && (nst->_sorts).size() == 1)
		p = p->resolve(nst->_sorts);
	if (s) {
		if (belongsToVoc(s)) {
			SortTable* st = _currstructure->inter(s);
			st->internTable(t->internTable());
			sortsOccurringInUserDefinedStructure.insert(s);
			delete (t);
		} else {
			notInVocabularyOf(ComponentType::Sort, ComponentType::Structure, toString(name), _currstructure->name(), pi);
		}
	} else if (p) {
		if (belongsToVoc(p)) {
			PredTable* pt = new PredTable(t->internTable(), _currstructure->universe(p));
			PredInter* i = _currstructure->inter(p);
			i->ctpt(pt);
			delete (t);
		} else {
			notInVocabularyOf(ComponentType::Predicate, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else {
		notDeclared(ComponentType::Predicate, toString(nst), pi);
	}
	delete (nst);
}

void Insert::sortinter(NSPair* nst, const longname& sortidentifier) {
	ParseInfo pi = nst->_pi;
	longname name = nst->_name;
	auto assignee = sortInScope(name, pi);
	auto s = sortInScope(sortidentifier, pi);
	if(s==NULL){
		notDeclared(ComponentType::Predicate, toString(sortidentifier), pi);
		return;
	}
	if(assignee==NULL){
		notDeclared(ComponentType::Predicate, toString(name), pi);
		return;
	}
	if(not _currstructure->hasInter(s)){
		stringstream ss;
		ss <<"The assigned sort " <<s->name() <<"does not have an interpretation. \n";
		Error::error(ss.str());
		return;
	}
	for(auto parent: assignee->parents()){
		for(auto voc = assignee->firstVocabulary(); voc!=assignee->lastVocabulary(); ++voc){ // FIXME what does it mean to have multiple vocabularies?
			if(not SortUtils::isSubsort(s, parent, *voc)){
				stringstream ss;
				ss <<"The assigned sort " <<s->name() <<" is not a subsort of the parent " <<parent->name() <<" of assignee " <<assignee->name() <<". \n";
				Error::error(ss.str());
				return;
			}
		}
	}
	auto inter = _currstructure->inter(s);
	_currstructure->changeInter(assignee, new SortTable(inter->internTable()));
}

bool Insert::interpretationSpecifiedByUser(Sort *s) const {
	return sortsOccurringInUserDefinedStructure.find(s) != sortsOccurringInUserDefinedStructure.cend();
}

void Insert::addElement(SortTable* s, int i) const {
	const DomainElement* d = createDomElem(i);
	s->add(d);
}

void Insert::addElement(SortTable* s, double f) const {
	const DomainElement* d = createDomElem(f);
	s->add(d);
}

void Insert::addElement(SortTable* s, std::string* e) const {
	const DomainElement* d = createDomElem(e);
	s->add(d);
}

void Insert::addElement(SortTable* s, const Compound* c) const {
	const DomainElement* d = createDomElem(c);
	s->add(d);
}

void Insert::addElement(SortTable* s, int i1, int i2) const {
	s->add(i1, i2);
}

void Insert::addElement(SortTable* s, char c1, char c2) const {
	for (char c = c1; c <= c2; ++c)
		addElement(s, StringPointer(string(1, c)));
}

SortTable* Insert::createSortTable() const {
	return TableUtils::createSortTable();
}

void Insert::truepredinter(NSPair* nst) const {
	auto pt = TableUtils::createPredTable(Universe(vector<SortTable*>(0)));
	ElementTuple et;
	pt->add(et);
	predinter(nst, pt);
}

void Insert::falsepredinter(NSPair* nst) const {
	auto pt = TableUtils::createPredTable(Universe(vector<SortTable*>(0)));
	predinter(nst, pt);
}

PredTable* Insert::createPredTable(unsigned int arity) const {
	return TableUtils::createPredTable(TableUtils::fullUniverse(arity));
}

void Insert::addTuple(PredTable* pt, ElementTuple& tuple, YYLTYPE l) const {
	if (tuple.size() == pt->arity() || pt->empty()) {
		pt->add(tuple);
	} else {
		ParseInfo pi = parseinfo(l);
		wrongarity(pi);
	}
}

void Insert::addTuple(PredTable* pt, YYLTYPE l) const {
	ElementTuple tuple;
	addTuple(pt, tuple, l);
}

const DomainElement* Insert::element(int i) const {
	return createDomElem(i);
}

const DomainElement* Insert::element(double d) const {
	return createDomElem(d);
}

const DomainElement* Insert::element(char c) const {
	return createDomElem(StringPointer(string(1, c)));
}

const DomainElement* Insert::element(std::string* s) const {
	return createDomElem(s);
}

const DomainElement* Insert::element(const Compound* c) const {
	return createDomElem(c);
}

FuncTable* Insert::createFuncTable(unsigned int arity) const {
	EnumeratedInternalFuncTable* eift = new EnumeratedInternalFuncTable();
	return new FuncTable(eift, TableUtils::fullUniverse(arity));
}

void Insert::addTupleVal(FuncTable* ft, ElementTuple& tuple, YYLTYPE l) const {
	if (ft->arity() == tuple.size() - 1) {
		ft->add(tuple);
	} else if (ft->empty()) {
		ft->add(tuple);
	} else {
		ParseInfo pi = parseinfo(l);
		wrongarity(pi);
	}
}

void Insert::addTupleVal(FuncTable* ft, const DomainElement* d, YYLTYPE l) const {
	ElementTuple et(1, d);
	addTupleVal(ft, et, l);
}

void Insert::inter(NSPair* nsp, const longname& procedure, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	UserProcedure* up = procedureInScope(procedure, pi);
	string* proc = NULL;
	if (up) {
		proc = StringPointer(up->registryindex());
	} else {
		proc = LuaConnection::getProcedure(procedure, pi);
	}
	if (proc) {
		vector<SortTable*> univ;
		if (nsp->_sortsincluded) {
			for (auto it = nsp->_sorts.cbegin(); it != nsp->_sorts.cend(); ++it) {
				if (*it) {
					univ.push_back(_currstructure->inter(*it));
				}
			}
			if (nsp->_func) {
				ProcInternalFuncTable* pift = new ProcInternalFuncTable(proc);
				FuncTable* ft = new FuncTable(pift, Universe(univ));
				funcinter(nsp, ft);
			} else {
				ProcInternalPredTable* pipt = new ProcInternalPredTable(proc);
				PredTable* pt = new PredTable(pipt, Universe(univ));
				predinter(nsp, pt);
			}
		} else {
			ParseInfo pi = nsp->_pi;
			std::set<Predicate*> vp = noArPredInScope(nsp->_name, pi);
			if (vp.empty()) {
				notDeclared(ComponentType::Predicate, toString(nsp), pi);
			} else if (vp.size() > 1) {
				std::set<Predicate*>::const_iterator it = vp.cbegin();
				Predicate* p1 = *it;
				++it;
				Predicate* p2 = *it;
				overloaded(ComponentType::Predicate, toString(nsp), p1->pi(), p2->pi(), pi);
			} else {
				for (auto it = (*(vp.cbegin()))->sorts().cbegin(); it != (*(vp.cbegin()))->sorts().cend(); ++it) {
					if (*it) {
						univ.push_back(_currstructure->inter(*it));
					}
				}
				ProcInternalPredTable* pipt = new ProcInternalPredTable(proc);
				PredTable* pt = new PredTable(pipt, Universe(univ));
				predinter(nsp, pt);
			}
		}
	}
}

void Insert::emptythreeinter(NSPair* nst, const string& utf) {
	if (nst->_sortsincluded) {
		EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
		PredTable* pt = new PredTable(ipt, TableUtils::fullUniverse(nst->_sorts.size()));
		if (nst->_func)
			threefuncinter(nst, utf, pt);
		else
			threepredinter(nst, utf, pt);
	} else {
		ParseInfo pi = nst->_pi;
		std::set<Predicate*> vp = noArPredInScope(nst->_name, pi);
		if (vp.empty())
			notDeclared(ComponentType::Predicate, toString(nst), pi);
		else if (vp.size() > 1) {
			std::set<Predicate*>::const_iterator it = vp.cbegin();
			Predicate* p1 = *it;
			++it;
			Predicate* p2 = *it;
			overloaded(ComponentType::Predicate, toString(nst), p1->pi(), p2->pi(), pi);
		} else {
			EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
			PredTable* pt = new PredTable(ipt, TableUtils::fullUniverse((*(vp.cbegin()))->arity()));
			threepredinter(nst, utf, pt);
		}
	}
}

void Insert::threepredinter(NSPair* nst, const string& utf, PredTable* t) {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != t->arity())
			incompatiblearity(toString(nst), pi);
		if (nst->_func)
			prednameexpected(pi);
	}
	nst->includeArity(t->arity());
	Predicate* p = predInScope(nst->_name, pi);
	if (p && nst->_sortsincluded && (nst->_sorts).size() == t->arity())
		p = p->resolve(nst->_sorts);
	if (p) {
		if (p->arity() == 1 && p->sort(0)->pred() == p) {
			threevalsort(p->name(), pi);
		} else {
			if (belongsToVoc(p)) {
				PredTable* nt = new PredTable(t->internTable(), _currstructure->universe(p));
				delete (t);
				switch (getUTF(utf, pi)) {
				case UTF_UNKNOWN:
					_unknownpredtables[p] = nt;
					break;
				case UTF_CT: {
					PredInter* pt = _currstructure->inter(p);
					pt->ct(nt);
					_cpreds[p] = UTF_CT;
					break;
				}
				case UTF_CF: {
					PredInter* pt = _currstructure->inter(p);
					pt->cf(nt);
					_cpreds[p] = UTF_CF;
					break;
				}
				case UTF_ERROR:
					break;
				}
			} else
				notInVocabularyOf(ComponentType::Predicate, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else
		notDeclared(ComponentType::Predicate, toString(nst), pi);
	delete (nst);
}

void Insert::threefuncinter(NSPair* nst, const string& utf, PredTable* t) {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != t->arity())
			incompatiblearity(toString(nst), pi);
		if (!(nst->_func))
			funcnameexpected(pi);
	}
	nst->includeArity(t->arity() - 1);
	Function* f = funcInScope(nst->_name, pi);
	if (f && nst->_sortsincluded && (nst->_sorts).size() == t->arity())
		f = f->resolve(nst->_sorts);
	if (f) {
		if (belongsToVoc(f)) {
			PredTable* nt = new PredTable(t->internTable(), _currstructure->universe(f));
			delete (t);
			switch (getUTF(utf, pi)) {
			case UTF_UNKNOWN:
				_unknownfunctables[f] = nt;
				break;
			case UTF_CT: {
				PredInter* ft = _currstructure->inter(f)->graphInter();
				ft->ct(nt);
				_cfuncs[f] = UTF_CT;
				break;
			}
			case UTF_CF: {
				PredInter* ft = _currstructure->inter(f)->graphInter();
				ft->cf(nt);
				_cfuncs[f] = UTF_CF;
				break;
			}
			case UTF_ERROR:
				break;
			}
		} else
			notInVocabularyOf(ComponentType::Function, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
	} else
		notDeclared(ComponentType::Function, toString(nst), pi);
}

void Insert::threepredinter(NSPair* nst, const string& utf, SortTable* t) {
	PredTable* pt = new PredTable(t->internTable(), TableUtils::fullUniverse(1));
	delete (t);
	threepredinter(nst, utf, pt);
}

void Insert::truethreepredinter(NSPair* nst, const string& utf) {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt, Universe(vector<SortTable*>(0)));
	ElementTuple et;
	pt->add(et);
	threepredinter(nst, utf, pt);
}

void Insert::falsethreepredinter(NSPair* nst, const string& utf) {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt, Universe(vector<SortTable*>(0)));
	threepredinter(nst, utf, pt);
}

pair<int, int>* Insert::range(int i1, int i2, YYLTYPE l) const {
	if (i1 > i2) {
		i2 = i1;
		invalidrange(i1, i2, parseinfo(l));
	}
	return new pair<int, int>(i1, i2);
}

pair<char, char>* Insert::range(char c1, char c2, YYLTYPE l) const {
	if (c1 > c2) {
		c2 = c1;
		invalidrange(c1, c2, parseinfo(l));
	}
	return new pair<char, char>(c1, c2);
}

const Compound* Insert::compound(NSPair* nst, const vector<const DomainElement*>& vte) const {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vte.size() + 1) {
			incompatiblearity(toString(nst), pi);
		}
		if (not nst->_func) {
			funcnameexpected(pi);
		}
	}
	nst->includeArity(vte.size());
	Function* f = funcInScope(nst->_name, pi);
	const Compound* c = NULL;
	if (f && nst->_sortsincluded && (nst->_sorts).size() == vte.size() + 1) {
		f = f->resolve(nst->_sorts);
	}
	if (f) {
		if (belongsToVoc(f)) {
			return createCompound(f, vte);
		} else {
			notInVocabularyOf(ComponentType::Function, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else {
		notDeclared(ComponentType::Function, toString(nst), pi);
	}
	return c;
}

const Compound* Insert::compound(NSPair* nst) const {
	ElementTuple t;
	return compound(nst, t);
}

void Insert::predatom(NSPair* nst, const vector<ElRange>& args, bool t) const {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != args.size()) {
			incompatiblearity(toString(nst), pi);
		}
		if (nst->_func) {
			prednameexpected(pi);
		}
	}
	nst->includeArity(args.size());
	Predicate* p = predInScope(nst->_name, pi);
	if (p && nst->_sortsincluded && (nst->_sorts).size() == args.size()) {
		p = p->resolve(nst->_sorts);
	}
	if (p) {
		if (belongsToVoc(p)) {
			if (p->arity() == 1 && p == (*(p->sorts().cbegin()))->pred()) {
				Sort* s = *(p->sorts().cbegin());
				SortTable* st = _currstructure->inter(s);
				switch (args[0]._type) {
				case ERE_EL:
					st->add(args[0]._value._element);
					break;
				case ERE_INT:
					st->add(args[0]._value._intrange->first, args[0]._value._intrange->second);
					break;
				case ERE_CHAR:
					for (char c = args[0]._value._charrange->first; c != args[0]._value._charrange->second; ++c) {
						st->add(createDomElem(StringPointer(string(1, c))));
					}
					break;
				}
			} else {
				ElementTuple tuple(p->arity());
				for (size_t n = 0; n < args.size(); ++n) {
					switch (args[n]._type) {
					case ERE_EL:
						tuple[n] = args[n]._value._element;
						break;
					case ERE_INT:
						tuple[n] = createDomElem(args[n]._value._intrange->first);
						break;
					case ERE_CHAR:
						tuple[n] = createDomElem(StringPointer(string(1, args[n]._value._charrange->first)));
						break;
					}
				}
				PredInter* inter = _currstructure->inter(p);
				if (t) {
					inter->makeTrue(tuple);
				} else {
					inter->makeFalse(tuple);
				}
				while (true) {
					size_t n = 0;
					bool end = false;
					for (; not end && n < args.size(); ++n) {
						switch (args[n]._type) {
						case ERE_EL:
							break;
						case ERE_INT: {
							int current = tuple[n]->value()._int;
							if (current == args[n]._value._intrange->second) {
								current = args[n]._value._intrange->first;
							} else {
								++current;
								end = true;
							}
							tuple[n] = createDomElem(current);
							break;
						}
						case ERE_CHAR: {
							char current = tuple[n]->value()._string->operator[](0);
							if (current == args[n]._value._charrange->second) {
								current = args[n]._value._charrange->first;
							} else {
								++current;
								end = true;
							}
							tuple[n] = createDomElem(StringPointer(string(1, current)));
							break;
						}
						}
					}
					if (n < args.size()) {
						if (t) {
							inter->makeTrue(tuple);
						} else {
							inter->makeFalse(tuple);
						}
					} else {
						break;
					}
				}
			}
		} else {
			notInVocabularyOf(ComponentType::Predicate, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else {
		notDeclared(ComponentType::Predicate, toString(nst), pi);
	}
	delete (nst);
}

void Insert::predatom(NSPair* nst, bool t) const {
	vector<ElRange> ver;
	predatom(nst, ver, t);
}

void Insert::funcatom(NSPair*, const vector<ElRange>&, const DomainElement*, bool) const {
	// TODO TODO TODO
}

void Insert::funcatom(NSPair* nst, const DomainElement* d, bool t) const {
	vector<ElRange> ver;
	funcatom(nst, ver, d, t);
}

vector<ElRange>* Insert::domaintuple(vector<ElRange>* dt, const DomainElement* d) const {
	dt->push_back(ElRange(d));
	return dt;
}

vector<ElRange>* Insert::domaintuple(vector<ElRange>* dt, pair<int, int>* p) const {
	dt->push_back(ElRange(p));
	return dt;
}

vector<ElRange>* Insert::domaintuple(vector<ElRange>* dt, pair<char, char>* p) const {
	dt->push_back(ElRange(p));
	return dt;
}

vector<ElRange>* Insert::domaintuple(const DomainElement* d) const {
	vector<ElRange>* dt = new vector<ElRange>(0);
	dt->push_back(ElRange(d));
	return dt;
}

vector<ElRange>* Insert::domaintuple(pair<int, int>* p) const {
	vector<ElRange>* dt = new vector<ElRange>(0);
	dt->push_back(ElRange(p));
	return dt;
}

vector<ElRange>* Insert::domaintuple(pair<char, char>* p) const {
	vector<ElRange>* dt = new vector<ElRange>(0);
	dt->push_back(ElRange(p));
	return dt;
}

const DomainElement* Insert::exec(const std::string& chunk) {
	return LuaConnection::execute(chunk);
}

void Insert::procarg(const string& argname) const {
	_currprocedure->addarg(argname);
}

template<class OptionValue>
void setOptionValue(Options* options, const string& opt, const OptionValue& val, const ParseInfo& pi) {
	if (not options->isOption(opt)) {
		unknoption(opt, pi);
	}
	if (not options->isAllowedValue(opt, val)) {
		wrongvalue(opt, convertToString(val), pi);
	}
	options->setValue(opt, val);
}

void Insert::assignunknowntables() {
	// Assign the unknown predicate interpretations
	for (auto it = _unknownpredtables.cbegin(); it != _unknownpredtables.cend(); ++it) {
		auto pri = _currstructure->inter(it->first);
		auto ctable = _cpreds[it->first] == UTF_CT ? pri->ct() : pri->cf();
		auto pt = new PredTable(ctable->internTable(), ctable->universe());
		for (auto tit = it->second->begin(); not tit.isAtEnd(); ++tit) {
			pt->add(*tit);
		}
		_cpreds[it->first] == UTF_CT ? pri->pt(pt) : pri->pf(pt);
		delete (it->second);
	}
	// Assign the unknown function interpretations
	for (auto it = _unknownfunctables.cbegin(); it != _unknownfunctables.cend(); ++it) {
		auto pri = _currstructure->inter(it->first)->graphInter();
		auto ctable = _cfuncs[it->first] == UTF_CT ? pri->ct() : pri->cf();
		auto pt = new PredTable(ctable->internTable(), ctable->universe());
		for (auto tit = it->second->begin(); not tit.isAtEnd(); ++tit) {
			pt->add(*tit);
		}
		_cfuncs[it->first] == UTF_CT ? pri->pt(pt) : pri->pf(pt);
		delete (it->second);
	}
	_unknownpredtables.clear();
	_unknownfunctables.clear();
}
