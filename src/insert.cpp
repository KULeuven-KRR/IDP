/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "IncludeComponents.hpp"
#include "insert.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

#include "theory/information/CheckSorts.hpp"
#include "theory/transformations/DeriveSorts.hpp"

#include "parser/yyltype.hpp"
#include "parser.hh"
#include "utils/ListUtils.hpp"
#include "errorhandling/error.hpp"
#include "options.hpp"
#include "internalargument.hpp"
#include "lua/luaconnection.hpp"
#include "theory/Query.hpp"
#include "fobdds/FoBdd.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/progression/data/LTCData.hpp"
#include "inferences/progression/data/StateVocInfo.hpp"
#include "fobdds/CommonBddTypes.hpp"
#include "fobdds/FoBddManager.hpp"
#include "GlobalData.hpp"
#include "fobdds/FoBddQuantKernel.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;
using namespace LuaConnection;
using namespace Error;
//TODO add abstraction to remove lua dependence here

typedef std::vector<ParseInfo> plist;
template<class LocatedList>
plist getLocations(const LocatedList& elems) {
	plist infos;
	for (auto p : elems) {
		infos.push_back(p->pi());
	}
	return infos;
}

/**
 * Rewrite a vector of strings s1,s2,...,sn to the single string s1::s2::...::sn
 */

template<>
std::ostream& operator<<(std::ostream& output, const longname& vs){
	if (!vs.empty()) {
		output << vs[0];
		for (unsigned int n = 1; n < vs.size(); ++n) {
			output << "::" << vs[n];
		}
	}
	return output;
}

string predName(const longname& name, const vector<Sort*>& vs) {
	stringstream sstr;
	sstr << print(name);
	if (!vs.empty()) {
		sstr << '[' << toString(vs[0]);
		for (unsigned int n = 1; n < vs.size(); ++n)
			sstr << ',' << toString(vs[n]);
		sstr << ']';
	}
	return sstr.str();
}

string predArityName(const longname& name, int arity) {
	stringstream sstr;
	sstr << print(name);
	sstr << "/" << arity << " ";
	return sstr.str();
}

string funcName(const longname& name, const vector<Sort*>& vs) {
	Assert(!vs.empty());
	stringstream sstr;
	sstr << print(name) << '[';
	if (vs.size() > 1) {
		sstr << toString(vs[0]);
		for (unsigned int n = 1; n < vs.size() - 1; ++n)
			sstr << ',' << toString(vs[n]);
	}
	sstr << ':' << toString(vs.back()) << ']';
	return sstr.str();
}

string funcArityName(const longname& name, int arity) {
	stringstream sstr;
	sstr << print(name);
	sstr << "/" << arity << ":1 ";
	return sstr.str();
}

/**Generates good error message for not-in-scope stuff in which arity can be important */
void undeclaredPred(NSPair* nst, int arity) {
	if (nst->_sortsincluded) {
		notDeclared(ComponentType::Predicate, toString(nst), nst->_pi);
	} else {
		notDeclared(ComponentType::Predicate, predArityName(nst->_name, arity), nst->_pi);
	}
}
void undeclaredFunc(NSPair* nst, int arity) {
	if (nst->_sortsincluded) {
		notDeclared(ComponentType::Function, toString(nst), nst->_pi);
	} else {
		notDeclared(ComponentType::Function, funcArityName(nst->_name, arity), nst->_pi);
	}
}

/*************
 VARNAME
 *************/

std::ostream& VarName::put(std::ostream& os) const {
	os << " (" << _name << "," << print(_var) << ") ";
	return os;
}

/*************
 NSPair
 *************/

ostream& NSPair::put(ostream& output) const {
	Assert(not _name.empty());
	string str = _name[0];
	for (size_t n = 1; n < _name.size(); ++n) {
		str = str + "::" + _name[n];
	}
	if (_sortsincluded) {
		str = str + '[';
		if (not _sorts.empty()) {
			if (_func and _sorts.size() == 1) {
				str = str + ':';
			}
			if (_sorts[0] != NULL) {
				str = str + _sorts[0]->name();
			}
			for (size_t n = 1; n < _sorts.size() - 1; ++n) {
				if (_sorts[n] != NULL) {
					str = str + ',' + _sorts[n]->name();
				}
			}
			if (_sorts.size() > 1) {
				if (_func) {
					str = str + ':';
				} else {
					str = str + ',';
				}
				if (_sorts[_sorts.size() - 1] != NULL) {
					str = str + _sorts[_sorts.size() - 1]->name();
				}
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

varset Insert::varVectorToSet(std::vector<Variable*>* v) {
	varset out = varset();
	copy(v->begin(), v->end(), inserter(out, out.begin()));
	return out;
}

bool Insert::belongsToVoc(Sort* s) const {
	if (_currvocabulary->contains(s)) {
		return true;
	}
	return false;
}

bool Insert::belongsToVoc(PFSymbol* p) const {
	if (_currvocabulary->contains(p)) {
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

Function* Insert::funcInScope(const string& name, int arity) const {
	std::set<Function*> vf;
	stringstream ss;
	ss <<name <<"/" <<arity;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		auto f = _usingvocab[n]->func(ss.str());
		if (f!=NULL) {
			vf.insert(f);
		}
	}
	if (vf.empty()) {
		return NULL;
	} else {
		return FuncUtils::overload(vf);
	}
}

Function* Insert::funcInScope(const vector<string>& vs, int arity, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return funcInScope(vs[0], arity);
	}
	vector<string> vv(vs.size() - 1);
	for (size_t n = 0; n < vv.size(); ++n) {
		vv[n] = vs[n];
	}
	auto v = vocabularyInScope(vv, pi);
	if(v==NULL){
		return NULL;
	}
	stringstream ss;
	ss <<vs.back() <<"/" <<arity;
	return v->func(ss.str());
}

Predicate* Insert::predInScope(const string& name, int arity) const {
	std::set<Predicate*> vp;
	stringstream ss;
	ss <<name <<"/" <<arity;
	for (size_t n = 0; n < _usingvocab.size(); ++n) {
		auto p = _usingvocab[n]->pred(ss.str());
		if (p!=NULL) {
			vp.insert(p);
		}
	}
	if (vp.empty()) {
		return NULL;
	} else {
		return PredUtils::overload(vp);
	}
}

Predicate* Insert::predInScope(const longname& vs, int arity, const ParseInfo& pi) const {
	Assert(not vs.empty());
	if (vs.size() == 1) {
		return predInScope(vs[0], arity);
	} else {
		vector<string> vv(vs.size() - 1);
		for (size_t n = 0; n < vv.size(); ++n) {
			vv[n] = vs[n];
		}
		auto v = vocabularyInScope(vv, pi);
		if (v==NULL) {
			return NULL;
		}
		stringstream ss;
		ss <<vs.back() <<"/"<<arity;
		return v->pred(ss.str());
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
			overloaded(ComponentType::Sort, s->name(), plist { s->pi(), temp->pi() }, pi);
		} else {
			s = temp;
		}
	}
	return s;
}

Sort* Insert::sortInScope(const longname& vs, const ParseInfo& pi) const {
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
				overloaded(ComponentType::Namespace, name, plist { _usingspace[n]->pi(), ns->pi() }, pi);
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
				overloaded(ComponentType::Vocabulary, name, plist { _usingspace[n]->vocabulary(name)->pi(), v->pi() }, pi);
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

Structure* Insert::structureInScope(const string& name, const ParseInfo& pi) const {
	Structure* s = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isStructure(name)) {
			if (s) {
				overloaded(ComponentType::Structure, name, plist { _usingspace[n]->structure(name)->pi(), s->pi() }, pi);
			} else {
				s = _usingspace[n]->structure(name);
			}
		}
	}
	return s;
}

Structure* Insert::structureInScope(const vector<string>& vs, const ParseInfo& pi) const {
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
				overloaded(ComponentType::Query, name, plist { _usingspace[n]->query(name)->pi(), q->pi() }, pi);
			} else {
				q = _usingspace[n]->query(name);
			}
		}
	}
	return q;
}

const FOBDD* Insert::fobddInScope(const string& name, const ParseInfo& pi) const {
	const FOBDD* b = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isFOBDD(name)) {
			if (b) {
				overloaded(ComponentType::FOBDD, name, plist { _usingspace[n]->fobdd(name)->pi(), b->pi() }, pi);
			} else {
				b = _usingspace[n]->fobdd(name);
			}
		}
	}
	return b;
}

Term* Insert::termInScope(const string& name, const ParseInfo& pi) const {
	Term* t = NULL;
	for (size_t n = 0; n < _usingspace.size(); ++n) {
		if (_usingspace[n]->isTerm(name)) {
			if (t) {
				overloaded(ComponentType::Term, name, plist { _usingspace[n]->term(name)->pi(), t->pi() }, pi);
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
				overloaded(ComponentType::Theory, name, plist { _usingspace[n]->theory(name)->pi(), th->pi() }, pi);
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
				overloaded(ComponentType::Procedure, name, plist { _usingspace[n]->procedure(name)->pi(), lp->pi() }, pi);
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

Insert::Insert(Namespace * ns)
		: parsingType(NULL) {
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
	_currfile = new std::string(s);
}

void Insert::currfile(string* s) {
	_currfile = s;
}

void Insert::partial(Function* f) const {
	if(f){
		f->partial(true);
	}
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

varset Insert::freevars(const ParseInfo& pi, bool critical) {
	varset vv;
	string vs;
	for (auto i = _curr_vars.cbegin(); i != _curr_vars.cend(); ++i) {
		vv.insert(i->_var);
		vs = vs + ' ' + i->_name;
	}
	if (not vv.empty()) {
		if (critical) {
			Error::freevars(vs, pi);
		} else if (getOption(BoolType::SHOWWARNINGS)) {
			Warning::freevars(vs, pi);
		}
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

void Insert::remove_vars(const varset& v) {
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

void Insert::closevocab() {
	Assert(_currvocabulary);
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(_currvocabulary);
	}
	closeblock();
}

void Insert::finishLTCVocab(Vocabulary* voc, const LTCVocInfo* ltcVocInfo) {
	auto singlestatevoc = ltcVocInfo->stateVoc;
	auto bistatevoc = ltcVocInfo->biStateVoc;
	auto singlestateEarlier = vocabularyInScope(singlestatevoc->name(), voc->pi());
	auto bistateEarlier = vocabularyInScope(bistatevoc->name(), voc->pi());
	if (singlestateEarlier) {
		declaredEarlier(ComponentType::Vocabulary, singlestatevoc->name(), voc->pi(), singlestateEarlier->pi());
	}
	if (bistateEarlier) {
		declaredEarlier(ComponentType::Vocabulary, bistatevoc->name(), voc->pi(), bistateEarlier->pi());
	}
	_currspace->add(singlestatevoc);
	_currspace->add(bistatevoc);
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(singlestatevoc);
	}
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(bistatevoc);
	}
}

void Insert::closeLTCvocab() {
	auto voc = _currvocabulary;
	auto ltcvocs = LTCData::instance()->getStateVocInfo(voc);
	finishLTCVocab(voc, ltcvocs);
	closevocab();
}

void Insert::closeLTCvocab(NSPair* time, NSPair* start, NSPair* next) {
	auto voc = _currvocabulary;

	LTCInputData symbols;

	symbols.time = sortInScope(time->_name, time->_pi);
	if (symbols.time == NULL) {
		error("Could not find the provided Time symbol ", voc->pi());
	}

	auto startname = start->_name;
	auto startnameBack = startname.back();
	startname.pop_back();
	startname.push_back(startnameBack);
	symbols.start = funcInScope(startname, 0, start->_pi);
	if (symbols.start == NULL) {
		error("Could not find the provided Start symbol", voc->pi());
	}

	auto nextname = next->_name;
	auto nextnameBack = nextname.back();
	nextname.pop_back();
	nextname.push_back(nextnameBack);
	symbols.next = funcInScope(nextname, 1, start->_pi);
	if (symbols.next == NULL) {
		error("Could not find the provided Next symbol", voc->pi());
	}

	closevocab();

	auto ltcvocs = LTCData::instance()->getStateVocInfo(voc, symbols);
	finishLTCVocab(voc, ltcvocs);
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
void Insert::openfobdd(const string& bddname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	auto b = fobddInScope(bddname, pi);
	if (b) {
		declaredEarlier(ComponentType::FOBDD, bddname, pi, b->pi());
	}
	_currfobdd = bddname;
	_currmanager = FOBDDManager::createManager(false);
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

void Insert::closetheory() {
	Assert(_currtheory);
	if (_currspace->isGlobal()) {
		LuaConnection::addGlobal(_currtheory);
	}
	closeblock();
}

template<class T>
bool varIsUnused(T const& t, Variable* var) {
	return t != NULL && t->contains(var);
}

template<class ... Args>
void checkForUnusedVariables(const varset& vv, Args&... args) {
	for (auto var : vv) {
		auto list = { varIsUnused(args, var)... };
		auto containsvar = false;
		for (auto l : list) {
			containsvar |= l;
		}
		if (not containsvar) {
			Warning::unusedvar(toString(var), var->pi());
		}
	}
}

void Insert::closequery(Query* q) {
	if (q != NULL) {
		freevars(q->pi(), true);
	}

	_curr_vars.clear();
	if (q != NULL) { // Allows for better error catching
		varset sv(q->variables().cbegin(), q->variables().cend());
		auto qf = new QuantForm(SIGN::POS, QUANT::UNIV, sv, q->query(), FormulaParseInfo());
		checkForUnusedVariables(sv, qf);
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
void Insert::closefobdd(const FOBDD* b) {
	freevars(b->pi(), false);
	_curr_vars.clear();
	if (b) {
		_currspace->add(_currfobdd, b);
		if (_currspace->isGlobal()) {
			LuaConnection::addGlobal(_currfobdd, b);
		}
	}
}

void Insert::closeterm(Term* t) {
	if (t != NULL) {
		freevars(t->pi(), true);
	}

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
	Structure* s = structureInScope(sname, pi);
	if (s) {
		declaredEarlier(ComponentType::Structure, sname, pi, s->pi());
	}
	_currstructure = new Structure(sname, pi);
	_currspace->add(_currstructure);
}

void Insert::closestructure(bool assumeClosedWorld) {
	Assert(_currstructure);

	if(getGlobal()->getErrorCount()==0){ // If the vocabulary might have errors, we should not do complex manipulations
		finalizePendingAssignments();
		_currstructure->checkAndAutocomplete();
		if (not getOption(BoolType::ASSUMECONSISTENTINPUT)) {
			_currstructure->sortCheck();
			_currstructure->satisfiesFunctionConstraints(true);
		}
		if (assumeClosedWorld) {
			makeUnknownsFalse(_currstructure);
		}
		_currstructure->clean();
	}

	parsedpreds.clear();
	parsedfuncs.clear();
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
	auto pi = parseinfo(l);
	auto p = predInScope(vs,arity, pi);
	if (p==NULL) {
		notDeclared(ComponentType::Predicate, predArityName(vs, arity), pi);
	}
	return p;
}

Predicate* Insert::predpointer(longname& vs, const vector<Sort*>& va,
YYLTYPE l) const {
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
	auto pi = parseinfo(l);
	auto f = funcInScope(vs, arity, pi);
	if (f == NULL) {
		notDeclared(ComponentType::Function, funcArityName(vs, arity), pi);
	}
	return f;
}

Function* Insert::funcpointer(longname& vs, const vector<Sort*>& va,
YYLTYPE l) const {
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
	return new NSPair(name, sorts, parseinfo(l));
}

NSPair* Insert::internfuncpointer(const vector<string>& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	auto nsp = new NSPair(name, insorts, parseinfo(l));
	nsp->_sorts.push_back(outsort);
	nsp->_func = true;
	return nsp;
}

NSPair* Insert::internpointer(const vector<string>& name, YYLTYPE l) const {
	return new NSPair(name, parseinfo(l));
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name			the name of the sort
 * \param fixedInter	an interpretation for this sort,
 * 						e.g. for a constructed sort or a fixed interpretation sort.
 * 						if NULL, it has no fixed interpretation
 */
Sort* Insert::sort(const string& name, YYLTYPE l, SortTable* fixedInter) {
	vector<Sort*> vs(0);
	return sort(name, vs, vs, l, fixedInter);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name			the name of the sort
 * \param supbs			the super- or subsorts of the sort
 * \param super			true if supbs are the supersorts, false if supbs are the subsorts
 * \param fixedInter	an interpretation for this sort,
 * 						e.g. for a constructed sort or a fixed interpretation sort.
 * 						if NULL, it has no fixed interpretation
 */
Sort* Insert::sort(const string& name, const vector<Sort*> supbs, bool super, YYLTYPE l, SortTable* fixedInter) {
	vector<Sort*> vs(0);
	if (super) {
		return sort(name, supbs, vs, l, fixedInter);
	} else {
		return sort(name, vs, supbs, l, fixedInter);
	}
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name			the name of the new sort
 * \param sups			the supersorts of the new sort
 * \param subs			the subsorts of the new sort
 * \param fixedInter	an interpretation for this sort,
 * 						e.g. for a constructed sort or a fixed interpretation sort.
 * 						if NULL, it has no fixed interpretation
 */
Sort* Insert::sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE l, SortTable* fixedInter) {
	ParseInfo pi = parseinfo(l);

	// Create the sort
	auto s = new Sort(name, pi, fixedInter);

	// Add the sort to the current vocabulary
	if (_currvocabulary->hasSortWithName(s->name())) {
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
			if (subs[n]->isConstructed()) {
				constructedTypeAsSubtype(ComponentType::Sort, subs[n]->name(), pi);
			}
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

	parsingType = s;

	return s;
}

void Insert::addConstructors(const std::vector<Function*>* functionlist) const {
	Assert(parsingType!=NULL);
	parsingType->setConstructed(true);
	for (auto f : (*functionlist)) {
		parsingType->addConstructor(f);
	}
}

Predicate* Insert::predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	auto pi = parseinfo(l);
	auto nar = string(name) + '/' + convertToString(sorts.size());
	for (size_t n = 0; n < sorts.size(); ++n) {
		if (sorts[n] == NULL) {
			return NULL;
		}
	}
	auto p = new Predicate(nar, sorts, pi);
	if (_currvocabulary->hasPredWithName(p->name())) {
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

Function* Insert::createfunction(const string& name, const vector<Sort*>& insorts, Sort* outsort, bool isConstructor, YYLTYPE l) const {
	auto pi = parseinfo(l);
	auto nar = string(name) + '/' + convertToString(insorts.size());
	for (size_t n = 0; n < insorts.size(); ++n) {
		if (insorts[n] == NULL) {
			return NULL;
		}
	}
	if (outsort == NULL) {
		return NULL;
	}
	auto f = new Function(nar, insorts, outsort, pi, isConstructor);
	if (_currvocabulary->hasFuncWithName(f->name())) {
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

Function* Insert::function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	return createfunction(name, insorts, outsort, false, l);
}

Function* Insert::constructorfunction(const string& name, const vector<Sort*>& insorts, YYLTYPE l) const {
	Assert(parsingType!=NULL);
	return createfunction(name, insorts, parsingType, true, l);
}



// add a definition to the current theory
//in case the definition is NULL, nothing is done.
// This invariant is used, since empty defintions are not created during parsing.
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

Rule* Insert::rule(const varset& qv, Formula* head, Formula* body, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	remove_vars(qv);
	if (head && body) {
		// Quantify the free variables
		auto vv = freevars(head->pi());
		remove_vars(vv);
		// Split quantified variables in head and body variables
		varset hv, bv;
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

Rule* Insert::rule(const varset& qv, Formula* head, YYLTYPE l) {
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

Formula* Insert::predformVar(NSPair* nst, const vector<Variable*>& vt, YYLTYPE l) const {
	std::vector<Term*> vs = vector<Term*>();
	for (auto var : vt) {
		vs.push_back(new VarTerm(var, TermParseInfo()));
	}
	return predform(nst, vs, l);
}



Formula* Insert::predform(NSPair* nst, const vector<Term*>& vt, YYLTYPE l) const {
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vt.size()) {
			incompatiblearity(toString(nst), (nst->_sorts).size(), vt.size(), nst->_pi);
		}
		if (nst->_func) {
			prednameexpected(nst->_pi);
		}
	}

	auto p = predInScope(nst->_name, vt.size(), nst->_pi);
	if (p!=NULL && nst->_sortsincluded && (nst->_sorts).size() == vt.size()) {
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
		undeclaredPred(nst, vt.size());
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

Formula* Insert::funcgraphform(NSPair* nst, const vector<Term*>& vt, Term* t,
YYLTYPE l) const {
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vt.size() + 1) {
			incompatiblearity(toString(nst), (nst->_sorts).size(), vt.size() + 1, nst->_pi);
		}
		if (not nst->_func) {
			funcnameexpected(nst->_pi);
		}
	}
	auto f = funcInScope(nst->_name, vt.size(), nst->_pi);
	if (f!=NULL && nst->_sortsincluded && (nst->_sorts).size() == vt.size() + 1) {
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
		undeclaredFunc(nst, vt.size());
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

Formula* Insert::boolform(bool conj, Formula* lf, Formula* rf,
YYLTYPE l) const {
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

Formula* Insert::quantform(bool univ, const varset& vv, Formula* f, YYLTYPE l) {
	remove_vars(vv);
	if (f) {
		checkForUnusedVariables(vv, f);
		varset pivv;
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

Formula* Insert::univform(const varset& vv, Formula* f, YYLTYPE l) {
	return quantform(true, vv, f, l);
}

Formula* Insert::existform(const varset& vv, Formula* f, YYLTYPE l) {
	return quantform(false, vv, f, l);
}

Formula* Insert::bexform(CompType c, int bound, const varset& vv, Formula* f,
YYLTYPE l) {
	if (f == NULL) {
		return f;
	}
	auto aggterm = dynamic_cast<AggTerm*>(aggregate(AggFunction::CARD, set(f, l, vv), l));
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
	if (getVar(name) != NULL) {
		declaredEarlier(ComponentType::Variable, name, parseinfo(l), othervar->pi());
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
			incompatiblearity(toString(nst), (nst->_sorts).size(), vt.size() + 1, nst->_pi);
		}
		if (not nst->_func) {
			funcnameexpected(nst->_pi);
		}
	}
	auto f = funcInScope(nst->_name, vt.size(), nst->_pi);
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
		undeclaredFunc(nst, vt.size());
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
	Term* t = NULL;
	string name = (nst->_name)[0];
	auto v = getVar(name);
	auto f = funcInScope(nst->_name, 0, nst->_pi);
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
		t = functerm(nst, vt);
	} else {
		YYLTYPE l;
		l.first_line = (nst->_pi).linenumber();
		l.first_column = (nst->_pi).columnnumber();
		v = quantifiedvar(name, l);
		auto temp = new VarTerm(v, TermParseInfo());
		t = new VarTerm(v, termparseinfo(temp, nst->_pi));
		if (nst->_sortsincluded && nst->_sorts.size() == 1) {
			t->sort(*(nst->_sorts.begin())); //it's a variable with a declared sort, so include sort in var
		}
		temp->recursiveDelete();
		delete (nst);
	}
	return t;
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
		bool knowntype = (lt->sort() && rt->sort());
		if (knowntype) {
			auto fnew= f->disambiguate( { lt->sort(), rt->sort(), NULL }, _currvocabulary);
			if(fnew!=NULL){
				f=fnew;
			}
		}
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
		return NULL;
	}
	Function* f = _currvocabulary->func(s + "/1");
	Assert(f);
	vector<Term*> vt(1, t);
	vector<Term*> pivt(1, t->clone());
	bool knowntype = t->sort();
	if (knowntype) {
		auto temp = f->disambiguate( { t->sort(), NULL }, _currvocabulary);
		if(temp != NULL){
			f = temp;
		}else{
			nofuncsort(f->name(),f->pi());
		}
	}
	auto temp = new FuncTerm(f, pivt, TermParseInfo());
	auto res = new FuncTerm(f, vt, termparseinfo(temp, l));
	temp->recursiveDelete();
	return res;
}

DomainTerm* Insert::domterm(int i, YYLTYPE l) const {
	auto d = createDomElem(i);
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
	const DomainElement* d = createDomElem(*e);
	Sort* s = get(STDSORT::STRINGSORT);
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

DomainTerm* Insert::domterm(char c, YYLTYPE l) const {
	const DomainElement* d = createDomElem(string(1, c));
	Sort* s = get(STDSORT::CHARSORT);
	auto temp = new DomainTerm(s, d, TermParseInfo());
	TermParseInfo pi = termparseinfo(temp, l);
	temp->recursiveDelete();
	return new DomainTerm(s, d, pi);
}

DomainTerm* Insert::domterm(std::string* e, Sort* s, YYLTYPE l) const {
	const DomainElement* d = createDomElem(*e);
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

const FOBDD* Insert::fobdd(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) const {
	auto returnvalue = _currmanager->ifthenelseTryMaintainOrder(kernel, truebranch, falsebranch);
	return returnvalue;
}

const FOBDDKernel* Insert::atomkernel(Formula* p) const {
	if (isa<PredForm>(*p)) {
		auto f = dynamic_cast<PredForm*>(p);
		auto symbol = f->symbol();
		vector<const FOBDDTerm*> newargs;
		for (auto subterm : p->subterms()) {
			newargs.push_back(_currmanager->getFOBDDTerm(subterm));
		}
		const FOBDDKernel* returnvalue(_currmanager->getAtomKernel(symbol, AtomKernelType::AKT_TWOVALUED, newargs));
		return returnvalue;
	} else if (isa<EqChainForm>(*p)) {
		auto f = dynamic_cast<EqChainForm*>(p);
		auto length = f->comps().size();
		PFSymbol* symbol;
		if (length == 1) {
			for (auto comp : f->comps()) {
				switch (comp) {
				case CompType::EQ:
					symbol = get(STDPRED::EQ, f->subterms()[0]->sort());
					break;
				case CompType::LT:
					symbol = get(STDPRED::LT, f->subterms()[0]->sort());
					break;
				case CompType::GT:
					symbol = get(STDPRED::GT, f->subterms()[0]->sort());
					break;
				default:
					throw notyetimplemented("Parsing eqchains that isn't >, < or =");
					break;
				}
				vector<const FOBDDTerm*> newargs;
				for (auto subterm : p->subterms()) {
					newargs.push_back(_currmanager->getFOBDDTerm(subterm));
				}
				const FOBDDKernel* returnvalue(_currmanager->getAtomKernel(symbol, AtomKernelType::AKT_TWOVALUED, newargs));
				return returnvalue;
			}

		} else {
			throw notyetimplemented("Chains of (in)equalities in a single atomkernel");
		}
	} else {
		throw notyetimplemented("Parsing non(predicates/equalities) in FOBDD");
	}
	return NULL;
}
const FOBDDKernel* Insert::quantkernel(Variable* var, const FOBDD* bdd) const {
	auto debruijnbdd = _currmanager->substitute(bdd, _currmanager->getVariable(var), _currmanager->getDeBruijnIndex(var->sort(), 0));
	auto qkernel = _currmanager->getQuantKernel(var->sort(), debruijnbdd);
	return qkernel;
}

const FOBDD* Insert::truefobdd() const {
	return _currmanager->truebdd();
}

const FOBDD* Insert::falsefobdd() const {
	return _currmanager->falsebdd();
}

EnumSetExpr* Insert::set(Formula* f, Term* counter, YYLTYPE l, const varset& vv) {
	remove_vars(vv);
	checkForUnusedVariables(vv, f, counter);
	if (f && counter) {
		varset pivv;
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

EnumSetExpr* Insert::set(Formula* f, YYLTYPE l, const varset& vv) {
	auto d = createDomElem(1);
	auto counter = new DomainTerm(get(STDSORT::NATSORT), d, TermParseInfo());
	return set(f, counter, l, vv);
}
EnumSetExpr* Insert::trueset(Term* t, YYLTYPE l) {
	return set(trueform(l) , t, l, varset());
}

void Insert::addToFirst(EnumSetExpr* s1, EnumSetExpr* s2) {
	for (auto p : s2->getSets()) {
		s1->addSet(p);
	}
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

void Insert::addElement(SortTable* s, int i) const {
	s->add(element(i));
}

void Insert::addElement(SortTable* s, double f) const {
	s->add(element(f));
}

void Insert::addElement(SortTable* s, const std::string& e) const {
	s->add(element(e));
}

void Insert::addElement(SortTable* s, const Compound* c) const {
	s->add(element(c));
}

void Insert::addElement(SortTable* s, int i1, int i2) const {
	s->add(i1, i2);
}

void Insert::addElement(SortTable* s, char c1, char c2) const {
	for (char c = c1; c <= c2; ++c) {
		addElement(s, string(1, c));
	}
}

SortTable* Insert::createSortTable() const {
	return TableUtils::createSortTable();
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
	return createDomElem(string(1, c));
}

const DomainElement* Insert::element(const std::string& s) const {
	// The parser cannot parse strings without "()" at the end as constructor function images, so this warning should be issued:
	auto f = funcInScope(s,0);
	if (f != NULL && (f->isConstructorFunction() || f->overloaded())) {
		Warning::constructorDisambiguationInStructure(s);
		if (f->overloaded()) {
			Error::overloaded(ComponentType::Function, s, std::vector<ParseInfo> { f->pi() }, { }); // TODO add locations
			return createDomElem(s); // Om toch maar iets gelijkaardig terug te geven.
		}
		if (f->isConstructorFunction()) {
			return createDomElem(createCompound(f, vector<const DomainElement*>()));
		}
	}
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
		auto domain = tuple;
		domain.pop_back();
		auto value = ft->operator [](domain);
		if (value != NULL && value != tuple.back()) {
			stringstream ss;
			ss << "Multiple images for the same function tuple " << ::print(domain) << ": " << ::print(value) << " and " << ::print(tuple.back()) << "\n";
			Error::error(ss.str());
			return;
		}
		ft->add(tuple);
	} else if (ft->empty()) {
		ft->add(tuple);
	} else {
		ParseInfo pi = parseinfo(l);
		wrongarity(pi);
	}
}

void Insert::addTupleVal(FuncTable* ft, const DomainElement* d,
YYLTYPE l) const {
	ElementTuple et(1, d);
	addTupleVal(ft, et, l);
}

pair<int, int>* Insert::range(int i1, int i2, YYLTYPE l) const {
	if (i1 > i2) {
		invalidrange(i1, i2, parseinfo(l));
		i2 = i1;
	}
	return new pair<int, int>(i1, i2);
}

pair<char, char>* Insert::range(char c1, char c2, YYLTYPE l) const {
	if (c1 > c2) {
		invalidrange(c1, c2, parseinfo(l));
		c2 = c1;
	}
	return new pair<char, char>(c1, c2);
}

const Compound* Insert::compound(NSPair* nst, const vector<const DomainElement*>& vte) const {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != vte.size() + 1) {
			incompatiblearity(toString(nst), (nst->_sorts).size(), vte.size() + 1, pi);
		}
		if (not nst->_func) {
			funcnameexpected(pi);
		}
	}
	auto f = funcInScope(nst->_name, vte.size(), pi);
	const Compound* c = NULL;
	if (f!=NULL  && nst->_sortsincluded && (nst->_sorts).size() == vte.size() + 1) {
		f = f->resolve(nst->_sorts);
	}
	if (f!=NULL) {
		if (belongsToVoc(f)) {
			return createCompound(f, vte);
		} else {
			notInVocabularyOf(ComponentType::Function, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		}
	} else {
		undeclaredFunc(nst, vte.size());
	}
	return c;
}

const Compound* Insert::compound(NSPair* nst) const {
	ElementTuple t;
	return compound(nst, t);
}

void Insert::predatom(NSPair* nst, const vector<ElRange>& args, bool t) {
	ParseInfo pi = nst->_pi;
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != args.size()) {
			incompatiblearity(toString(nst), (nst->_sorts).size(), args.size(), pi);
		}
		if (nst->_func) {
			prednameexpected(pi);
		}
	}
	auto p = predInScope(nst->_name, args.size(), pi);
	if (p!=NULL && nst->_sortsincluded && (nst->_sorts).size() == args.size()) {
		p = p->resolve(nst->_sorts);
	}
	if (p == NULL) {
		undeclaredPred(nst, args.size());
		delete (nst);
		return;
	}
	if (not belongsToVoc(p)) {
		notInVocabularyOf(ComponentType::Predicate, ComponentType::Structure, toString(nst), _currstructure->name(), pi);
		delete (nst);
		return;
	}

	parsedpreds.insert(p);
	if (p->arity() == 1 && p == (*(p->sorts().cbegin()))->pred()) {
		auto s = *(p->sorts().cbegin());
		auto st = _currstructure->inter(s);
		switch (args[0]._type) {
		case ERE_EL:
			st->add(args[0]._value._element);
			break;
		case ERE_INT:
			st->add(args[0]._value._intrange->first, args[0]._value._intrange->second);
			break;
		case ERE_CHAR:
			for (char c = args[0]._value._charrange->first; c != args[0]._value._charrange->second; ++c) {
				st->add(createDomElem(string(1, c)));
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
				tuple[n] = createDomElem(string(1, args[n]._value._charrange->first));
				break;
			}
		}
		auto inter = _currstructure->inter(p);
		if (t) {
			inter->makeTrueAtLeast(tuple, true);
		} else {
			inter->makeFalseAtLeast(tuple, true);
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
					tuple[n] = createDomElem(string(1, current));
					break;
				}
				}
			}
			if (n >= args.size()) {
				break;
			}
			if (t) {
				inter->makeTrueAtLeast(tuple, true);
			} else {
				inter->makeFalseAtLeast(tuple, true);
			}
		}
	}
	delete (nst);
}

void Insert::predatom(NSPair* nst, bool t) {
	vector<ElRange> ver;
	predatom(nst, ver, t);
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

/*******************
 * Interpretations
 *******************/

bool Insert::interpretationSpecifiedByUser(Structure* structure, Sort *sort) const {
	if (not contains(sortsOccurringInUserDefinedStructure, structure)) {
		return false;
	}
	return contains(sortsOccurringInUserDefinedStructure.at(structure), sort);
}

void Insert::predinter(NSPair* nst, SortTable* t, const string& utf) const {
	auto pt = new PredTable(t->internTable(), TableUtils::fullUniverse(1));
	delete (t);
	predinter(nst, pt, utf);
}

void Insert::truepredinter(NSPair* nst, const string& utf) const {
	auto eipt = new EnumeratedInternalPredTable();
	auto pt = new PredTable(eipt, Universe(vector<SortTable*>(0)));
	ElementTuple et;
	pt->add(et);
	predinter(nst, pt, utf);
}

void Insert::falsepredinter(NSPair* nst, const string& utf) const {
	auto eipt = new EnumeratedInternalPredTable();
	auto pt = new PredTable(eipt, Universe(vector<SortTable*>(0)));
	predinter(nst, pt, utf);
}

bool Insert::isValidTruthType(const string& utf) const {
	return utf == "u" || utf == "ct" || utf == "cf" || /*utf == "pt" || utf == "pf" ||*/utf == "tv";
}
Insert::UTF Insert::getTruthType(const string& utf) const {
	if (utf == "u") {
		return Insert::UTF::U;
	} else if (utf == "ct") {
		return Insert::UTF::CT;
	} else if (utf == "cf") {
		return Insert::UTF::CF;
	}/* else if (utf == "pt") {
	 return Insert::UTF::PT;
	 } else if (utf == "pf") {
	 return Insert::UTF::PF;
	 }*/else if (utf == "tv") {
		return Insert::UTF::TWOVAL;
	} else {
		throw IdpException("Invalid code path");
	}
}

void Insert::predinter(NSPair* nst, PredTable* t, const string& utf) const {
	if (not isValidTruthType(utf)) {
		expectedutf(utf, nst->_pi);
		return;
	}
	setInter(nst, false, getTruthType(utf), t, t->arity());
}

void Insert::funcinter(NSPair* nst, PredTable* t, const string& utf) const {
	if (not isValidTruthType(utf)) {
		expectedutf(utf, nst->_pi);
		return;
	}
	setInter(nst, true, getTruthType(utf), t, t->arity() - 1);
}

void Insert::funcinter(NSPair* nst, FuncTable* t, const string& utf) const {
	if (not isValidTruthType(utf)) {
		expectedutf(utf, nst->_pi);
		return;
	}
	setInter(nst, true, getTruthType(utf), t, t->arity());
}

template<class Table>
PredTable* newTable(Table t, const Universe& universe);
template<>
PredTable* newTable(PredTable* t, const Universe& universe) {
	return new PredTable(t->internTable(), universe);
}
template<>
PredTable* newTable(FuncTable* t, const Universe& universe) {
	return new PredTable(new FuncInternalPredTable(new FuncTable(t->internTable(), universe), false), universe);
}

// Note: arity==-1 means arity unknown
PFSymbol* Insert::retrieveSymbolNoChecks(NSPair* nst, bool expectsFunc, int arity) const {
	auto pi = nst->_pi;
	if (nst->_sortsincluded) {
		if (arity != -1 && (int) (nst->_sorts).size() != (expectsFunc ? arity + 1 : arity)) {
			incompatiblearity(toString(nst), (nst->_sorts).size(), (expectsFunc ? arity + 1 : arity), pi);
			return NULL;
		}
		if (not expectsFunc && nst->_func) {
			prednameexpected(pi);
			return NULL;
		}
		if (expectsFunc && not nst->_func) {
			funcnameexpected(pi);
			return NULL;
		}
	}
	PFSymbol* p = NULL;
	if (arity != -1) {
		if(expectsFunc){
			p = funcInScope(nst->_name, arity, pi);
		}else{
			p = predInScope(nst->_name, arity, pi);
			auto p2 = funcInScope(nst->_name, arity-1, pi);
			if(p!=NULL && p2!=NULL){
				Error::overloaded(ComponentType::Symbol, toString(nst), getLocations<std::vector<PFSymbol*>>({p,p2}), {});
			}
			if(p==NULL){
				p = p2;
			}
		}
	} else {
		if (expectsFunc) {
			auto funcs = noArFuncInScope(nst->_name, pi);
			if (funcs.size() != 1) {
				overloaded(ComponentType::Function, toString(nst), getLocations(funcs), pi);
			} else {
				p = *funcs.begin();
			}
		} else {
			auto preds = noArPredInScope(nst->_name, pi);
			if (preds.size() != 1) {
				overloaded(ComponentType::Predicate, toString(nst), getLocations(preds), pi);
			} else {
				p = *preds.begin();
			}
		}
	}
	if (p && nst->_sortsincluded && arity != -1) {
#ifdef DEBUG
		if (not expectsFunc) {
			Assert((int) (nst->_sorts).size() == arity);
		} else {
			Assert((int) (nst->_sorts).size() == arity + 1);
		}
#endif
		p = p->resolve(nst->_sorts);
	}
	return p;
}

template<class Table>
void Insert::setInter(NSPair* nst, bool expectsFunc, UTF truthvalue, Table* t, int arity) const {
	auto p = retrieveSymbolNoChecks(nst, expectsFunc, arity);
	auto error = basicSymbolCheck(p, nst, truthvalue);
	if (not error && t->arity() == 1 && p->sort(0)->pred() == p && truthvalue != UTF::TWOVAL) {
		threevalsort(p->name(), nst->_pi);
		error = true;
	}
	if (not error) {
		auto nt = newTable(t, _currstructure->universe(p));
		delete (t);
		_pendingAssignments[p][truthvalue] = nt;
	}
	delete (nst);
}

bool Insert::basicSymbolCheck(PFSymbol* symbol, NSPair* nst) const {
	bool error = false;
	if (not error && symbol == NULL) {
		notDeclared(ComponentType::Symbol, toString(nst), nst->_pi);
		error = true;
	}
	if (not error && not belongsToVoc(symbol)) {
		auto comptype = symbol->isFunction() ? ComponentType::Function : ComponentType::Predicate;
		notInVocabularyOf(comptype, ComponentType::Structure, toString(nst), _currstructure->name(), nst->_pi);
		error = true;
	}
	if (not error && symbol->overloaded()) {
		error = true;
		if (symbol->isFunction()) {
			auto func = dynamic_cast<Function*>(symbol);
			Assert(func != NULL);
			overloaded(ComponentType::Function, toString(nst), getLocations(func->nonbuiltins()), nst->_pi);
		} else {
			auto pred = dynamic_cast<Predicate*>(symbol);
			Assert(pred != NULL);
			overloaded(ComponentType::Predicate, toString(nst), getLocations(pred->nonbuiltins()), nst->_pi);
		}
	}
	return error;
}

std::string Insert::printUTF(UTF utf) const {
	switch (utf) {
	case UTF::TWOVAL:
		return "tv";
	case UTF::U:
		return "u";
	case UTF::CT:
		return "ct";
	case UTF::CF:
		return "cf";
	}
	throw IdpException("invalid code path");
}

// Checks whether a certain symbol can be assigned an interpretation
bool Insert::basicSymbolCheck(PFSymbol* symbol, NSPair* nst, UTF utf) const {
	bool error = basicSymbolCheck(symbol, nst);
	if (contains(_pendingAssignments, symbol)) {
		auto type = symbol->isFunction() ? ComponentType::Function : ComponentType::Predicate;
		if (not error && contains(_pendingAssignments.at(symbol), utf)) {
			stringstream ss;
			ss << type << " " << symbol->name() << " was already interpreted for the truth value " << printUTF(utf) << ".";
			Error::error(ss.str(), nst->_pi);
			error = true;
		}
		if (not error && (_pendingAssignments.at(symbol).size() > 2 || contains(_pendingAssignments.at(symbol), UTF::TWOVAL) || utf == UTF::TWOVAL)) {
			stringstream ss;
			ss << type << " " << symbol->name() << " was already " << (utf == UTF::TWOVAL ? "partially" : "fully")
					<< " interpreted earlier by other truth values than " << printUTF(utf) << ".";
			Error::error(ss.str(), nst->_pi);
			error = true;
		}
	}
	if (not error && symbol->isFunction()) {
		auto func = (Function*) symbol;
		if (func->isConstructorFunction()) {
			stringstream ss;
			ss << symbol->name() << " is a constructor function: its interpretation is fixed and cannot change.";
			Error::error(ss.str(), nst->_pi);
			error = true;
		}
	}
	return error;
}

PFSymbol* Insert::findUniqueMatch(NSPair* nst) const {
	auto pi = nst->_pi;
	auto posspred = noArPredInScope(nst->_name, pi);
	auto possfuncs = noArFuncInScope(nst->_name, pi);

	if (posspred.empty() && possfuncs.empty()) {
		notDeclared(ComponentType::Symbol, toString(nst), pi);
		return NULL;
	}
	if (posspred.size() == 0 && possfuncs.size() == 1) {
		return *possfuncs.cbegin();
	} else if (posspred.size() == 1 && possfuncs.size() == 0) {
		return *posspred.cbegin();
	} else {
		std::vector<ParseInfo> infos;
		for (auto p : posspred) {
			infos.push_back(p->pi());
		}
		for (auto f : possfuncs) {
			infos.push_back(f->pi());
		}
		overloaded(ComponentType::Symbol, toString(nst), infos, pi);
	}
	return NULL;
}

// this method dispatches the declaration of an empty interpretation of a symbol to the appropriate method (sort, pred or func interpretation)
// NOTE: what about the types of the tables? Apparently, only enumerated tables are used, but the signature of the functions can contain e.g. ints?
void Insert::emptyinter(NSPair* nst, const string& utf) const {
	if (sortInScope(nst->_name, nst->_pi)) {
            auto ist = new EnumeratedInternalSortTable();
            auto st = new SortTable(ist);
            sortinter(nst,st);
            return;
        }
        
        // the empty interpretation is no sort, so it must be either a function or a predicate
	bool func = false;
	int universesize = -1;
	if (nst->_sortsincluded) {
		func = nst->_func;
		universesize = nst->_sorts.size();
	} else {
		auto result = findUniqueMatch(nst);
		if (result == NULL) {
			return;
		}
		func = result->isFunction();
		universesize = result->sorts().size();
	}

	if (func) {
		auto ift = new EnumeratedInternalFuncTable();
		auto ft = new FuncTable(ift, TableUtils::fullUniverse(universesize));
		funcinter(nst, ft, utf);
	} else {
		auto ipt = new EnumeratedInternalPredTable();
		auto pt = new PredTable(ipt, TableUtils::fullUniverse(universesize));
		predinter(nst, pt, utf);
	}
}

void Insert::interByProcedure(NSPair* nsp, const longname& procedure,
YYLTYPE l) const {
	auto pi = parseinfo(l);
	auto up = procedureInScope(procedure, pi);
	string* proc = NULL;
	if (up) {
		proc = new std::string(up->registryindex());
	} else {
		proc = LuaConnection::getProcedure(procedure, pi);
	}
	if (proc == NULL) {
		notDeclared(ComponentType::Procedure, toString(procedure), pi);
		return;
	}

	bool func = false;
	vector<SortTable*> univ;
	if (nsp->_sortsincluded) {
		func = nsp->_func;
		for (auto sort : nsp->_sorts) {
			if (sort != NULL) {
				univ.push_back(_currstructure->inter(sort));
			}
		}
	} else {
		auto result = findUniqueMatch(nsp);
		if (result == NULL) {
			return;
		}
		func = result->isFunction();
		for (auto sort : result->sorts()) {
			if (sort) {
				univ.push_back(_currstructure->inter(sort));
			}
		}
	}
	if (func) {
		auto pift = new ProcInternalFuncTable(proc);
		auto ft = new FuncTable(pift, Universe(univ));
		funcinter(nsp, ft);
	} else {
		auto pipt = new ProcInternalPredTable(proc);
		auto pt = new PredTable(pipt, Universe(univ));
		predinter(nsp, pt);
	}
}

void Insert::constructor(NSPair* nst) const {
	auto pi = nst->_pi;
	auto f = retrieveSymbolNoChecks(nst, true, -1);
	auto error = basicSymbolCheck(f, nst);
	if (not error) {
		auto function = dynamic_cast<Function*>(f);
		auto uift = new UNAInternalFuncTable(function);
		auto ft = new FuncTable(uift, _currstructure->universe(f));
		funcinter(nst, ft);
	}
}

void Insert::sortinter(NSPair* nst, SortTable* t) const {
	ParseInfo pi = nst->_pi;
	longname name = nst->_name;
	Sort* s = sortInScope(name, pi);
	if (nst->_sortsincluded) {
		if ((nst->_sorts).size() != 1) {
			incompatiblearity(toString(nst), (nst->_sorts).size(), 1, pi);
		}
		if (nst->_func) {
			prednameexpected(pi);
		}
	}
	auto p = predInScope(nst->_name, 1, pi);
	if (p!=NULL and nst->_sortsincluded and (nst->_sorts).size() == 1) {
		p = p->resolve(nst->_sorts);
	}
	if (s) {
		if (belongsToVoc(s) && !s->hasFixedInterpretation()) {
			auto st = _currstructure->inter(s);
			st->internTable(t->internTable());
			sortsOccurringInUserDefinedStructure[_currstructure].insert(s);
			delete (t);
		} else if (s->hasFixedInterpretation()) {
			fixedInterTypeReinterpretedInStructure(ComponentType::Sort, toString(name), pi);
		} else {
			notInVocabularyOf(ComponentType::Sort, ComponentType::Structure, toString(name), _currstructure->name(), pi);
		}
		delete (nst);
	} else if (p) {
		predinter(nst, t, "tv");
	} else {
		notDeclared(ComponentType::Predicate, toString(nst), pi);
		delete (nst);
	}
}

void Insert::finalizePendingAssignments() {
	for (auto symbol2valuetables : _pendingAssignments) {
		auto& tables = symbol2valuetables.second;
		Assert(tables.size() == 1 || tables.size() == 2);
		auto inter = _currstructure->inter(symbol2valuetables.first);
		if (tables.size() == 2) {
			if (contains(tables, UTF::CT) && contains(tables, UTF::CF)) {
				inter->ct(tables[UTF::CT]);
				inter->cf(tables[UTF::CF]);
			} else if (contains(tables, UTF::CT)) {
				inter->ct(tables[UTF::CT]);
				auto ctable = inter->ct();
				auto pt = new PredTable(ctable->internTable(), ctable->universe());
				for (auto i = tables[UTF::U]->begin(); not i.isAtEnd(); ++i) {
					pt->add(*i);
				}
				inter->pt(pt);
			} else if (contains(tables, UTF::CF)) {
				inter->cf(tables[UTF::CF]);
				auto ctable = inter->cf();
				auto pf = new PredTable(ctable->internTable(), ctable->universe());
				for (auto i = tables[UTF::U]->begin(); not i.isAtEnd(); ++i) {
					pf->add(*i);
				}
				inter->pf(pf);
			}
		} else {
			auto value2table = *tables.cbegin();
			switch (value2table.first) {
			case UTF::TWOVAL: {
				auto funcintern = dynamic_cast<FuncInternalPredTable*>(value2table.second->internTable());
				if (funcintern != NULL) {
					_currstructure->inter(dynamic_cast<Function*>(symbol2valuetables.first))->funcTable(funcintern->table());
				} else {
					inter->ctpt(value2table.second);
				}
				break;
			}
			case UTF::CT:
				inter->ct(value2table.second);
				break;
			case UTF::CF:
				inter->cf(value2table.second);
				break;
			case UTF::U: {
				stringstream ss;
				ss << "Only specified unknown truthvalue for symbol " << toString(symbol2valuetables.first) << "\n";
				Error::error(ss.str());
				break;
			}
			}
		}
	}
	_pendingAssignments.clear();
}
