/************************************
	term.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "common.hpp"
using namespace std;

/*********************
	Abstract terms
*********************/

void Term::setFreeVars() {
	_freevars.clear();
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		_freevars.insert((*it)->freeVars().begin(),(*it)->freeVars().end());
	}
	for(auto it = _subsets.begin(); it != _subsets.end(); ++it) {
		_freevars.insert((*it)->freeVars().begin(),(*it)->freeVars().end());
	}
}

void Term::recursiveDelete() {
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _subsets.begin(); it != _subsets.end(); ++it) {
		(*it)->recursiveDelete();
	}
	delete(this);
}

bool Term::contains(const Variable* v) const {
	for(auto it = _freevars.begin(); it != _freevars.end(); ++it) {
		if(*it == v) { return true; }
	}
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->contains(v)) { return true; }
	}
	for(auto it = _subsets.begin(); it != _subsets.end(); ++it) {
		if((*it)->contains(v)) { return true; }
	}
	return false;
}

string Term::toString(bool longnames) const {
	stringstream sstr;
	put(sstr,longnames);
	return sstr.str();
}

ostream& operator<<(ostream& output, const Term& t) {
	return t.put(output);
}

/***************
	VarTerm
***************/

void VarTerm::setFreeVars() {
	_freevars.clear();
	_freevars.insert(_var);
}

void VarTerm::sort(Sort* s) {
	_var->sort(s);
}

VarTerm::VarTerm(Variable* v, const TermParseInfo& pi) : Term(pi), _var(v) {
	setFreeVars();
}

VarTerm* VarTerm::clone() const {
	return new VarTerm(_var,_pi.clone());
}

VarTerm* VarTerm::clone(const map<Variable*,Variable*>& mvv) const {
	map<Variable*,Variable*>::const_iterator it = mvv.find(_var);
	if(it != mvv.end()) { return new VarTerm(it->second,_pi); }
	else { return new VarTerm(_var,_pi.clone(mvv)); }
}

inline Sort* VarTerm::sort() const {
	return _var->sort();
}

void VarTerm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Term* VarTerm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& VarTerm::put(std::ostream& output, bool longnames) const {
	var()->put(output,longnames);
	return output;
}

/*****************
	FuncTerm
*****************/

FuncTerm::FuncTerm(Function* func, const vector<Term*>& args, const TermParseInfo& pi) : 
	Term(pi), _function(func) { 
	subterms(args);
}

FuncTerm* FuncTerm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

FuncTerm* FuncTerm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> newargs;
	for(auto it = subterms().begin(); it != subterms().end(); ++it) {
		newargs.push_back((*it)->clone(mvv));
	}
	return new FuncTerm(_function,newargs,_pi.clone(mvv));
}

Sort* FuncTerm::sort() const { 
	return _function->outsort();
}

void FuncTerm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Term* FuncTerm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& FuncTerm::put(ostream& output, bool longnames) const {
	function()->put(output,longnames);
	if(not subterms().empty()) {
		output << '('; 
		subterms()[0]->put(output,longnames);
		for(size_t n = 1; n < subterms().size(); ++n) {
			output << ',';
			subterms()[n]->put(output,longnames);
		}
		output << ')';
	}
	return output; 
}

/*****************
	DomainTerm
*****************/

DomainTerm::DomainTerm(Sort* sort, const DomainElement* value, const TermParseInfo& pi) :
	Term(pi), _sort(sort), _value(value) {
	
}

DomainTerm* DomainTerm::clone() const {
	return new DomainTerm(_sort,_value,_pi.clone());
}

DomainTerm* DomainTerm::clone(const map<Variable*,Variable*>& mvv) const {
	return new DomainTerm(_sort,_value,_pi.clone(mvv));
}

void DomainTerm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Term* DomainTerm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& DomainTerm::put(ostream& output, bool) const {
	value()->put(output);
	return output;
}

/**************
	AggTerm
**************/

AggTerm::AggTerm(SetExpr* set, AggFunction function, const TermParseInfo& pi) :
	Term(pi), _function(function) {
	addSet(set);
}

AggTerm* AggTerm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

AggTerm* AggTerm::clone(const map<Variable*,Variable*>& mvv) const {
	SetExpr* newset = subsets()[0]->clone(mvv);
	return new AggTerm(newset,_function,_pi.clone(mvv));
}

Sort* AggTerm::sort() const {
	if(_function == AggFunction::CARD) {
		return VocabularyUtils::natsort();
	}
	else {
		return set()->sort();
	}
}

void AggTerm::accept(TheoryVisitor* v) const {
	v->visit(this);
}

Term* AggTerm::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& AggTerm::put(ostream& output, bool longnames) const {
	output << function(); 
	subsets()[0]->put(output,longnames);
	return output;
}

/**************
	SetExpr
**************/

void SetExpr::setFreeVars() {
	_freevars.clear();
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		_freevars.insert((*it)->freeVars().begin(),(*it)->freeVars().end());
	}
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		_freevars.insert((*it)->freeVars().begin(),(*it)->freeVars().end());
	}
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		_freevars.erase(*it);
	}
}

void SetExpr::recursiveDelete() {
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		delete(*it);
	}
	delete(this);
}

bool SetExpr::contains(const Variable* v) const {
	for(auto it = _freevars.begin(); it != _freevars.end(); ++it) {
		if(*it == v) { return true; }
	}
	for(auto it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		if(*it == v) { return true; }
	}
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->contains(v)) { return true; }
	}
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		if((*it)->contains(v)) { return true; }
	}
	return false;
}

std::string SetExpr::toString(bool longnames) const {
	stringstream sstr;
	put(sstr,longnames);
	return sstr.str();
}

ostream& operator<<(ostream& output,const SetExpr& set) {
	return set.put(output);
}

/******************
	EnumSetExpr
******************/

EnumSetExpr::EnumSetExpr(const vector<Formula*>& subforms, const vector<Term*>& weights, const SetParseInfo& pi) :
	SetExpr(pi) {
	_subformulas = subforms;
	_subterms = weights;
	setFreeVars();
}

EnumSetExpr* EnumSetExpr::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EnumSetExpr* EnumSetExpr::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> newforms;
	vector<Term*> newweights;
	for(auto it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		newforms.push_back((*it)->clone(mvv));
	}
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		newweights.push_back((*it)->clone(mvv));
	}
	return new EnumSetExpr(newforms,newweights,_pi.clone(mvv));
}

Sort* EnumSetExpr::sort() const {
	Sort* currsort = VocabularyUtils::natsort();
	for(auto it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->sort()) {
			currsort = SortUtils::resolve(currsort,(*it)->sort());
		}
		else { return 0; }
	}
	if(currsort) {
		if(SortUtils::isSubsort(currsort,VocabularyUtils::natsort())) {
			return VocabularyUtils::natsort();
		}
		else if(SortUtils::isSubsort(currsort,VocabularyUtils::intsort())) {
			return VocabularyUtils::intsort();
		}
		else if(SortUtils::isSubsort(currsort,VocabularyUtils::floatsort())) {
			return VocabularyUtils::floatsort();
		}
		else { return 0; }
	}
	else return 0;
}

void EnumSetExpr::accept(TheoryVisitor* v) const {
	v->visit(this);
}

SetExpr* EnumSetExpr::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& EnumSetExpr::put(ostream& output, bool longnames) const {
	output << "[ ";
	if(not subformulas().empty()) {
		for(size_t n = 0; n < subformulas().size(); ++n) {
			output << '('; subformulas()[n]->put(output,longnames); 
			output << ','; subterms()[n]->put(output,longnames); 
			output << ')';
			if(n < subformulas().size()-1) { output << "; "; }
		}
	}
	output << " ]";
	return output;
}

/*******************
	QuantSetExpr
*******************/

QuantSetExpr::QuantSetExpr(const set<Variable*>& qvars, Formula* formula, Term* term, const SetParseInfo& pi) :
	SetExpr(pi) {
	_quantvars = qvars;
	_subterms.push_back(term);
	_subformulas.push_back(formula);
	setFreeVars();
}

QuantSetExpr* QuantSetExpr::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

QuantSetExpr* QuantSetExpr::clone(const map<Variable*,Variable*>& mvv) const {
	set<Variable*> newvars;
	map<Variable*,Variable*> nmvv = mvv;
	for(auto it = quantVars().begin(); it != quantVars().end(); ++it) {
		Variable* nv = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
		newvars.insert(nv);
		nmvv[*it] = nv;
	}
	Term* newterm = subterms()[0]->clone(nmvv);
	Formula* nf = subformulas()[0]->clone(nmvv);
	return new QuantSetExpr(newvars,nf,newterm,_pi.clone(mvv));
}

Sort* QuantSetExpr::sort() const {
	Sort* termsort = (*_subterms.begin())->sort();
	if(termsort) {
		if(SortUtils::isSubsort(termsort,VocabularyUtils::natsort())) {
			return VocabularyUtils::natsort();
		}
		else if(SortUtils::isSubsort(termsort,VocabularyUtils::intsort())) {
			return VocabularyUtils::intsort();
		}
		else if(SortUtils::isSubsort(termsort,VocabularyUtils::floatsort())) {
			return VocabularyUtils::floatsort();
		}
		else { return 0; }
	}
	else { return 0; }
}

void QuantSetExpr::accept(TheoryVisitor* v) const {
	v->visit(this);
}

SetExpr* QuantSetExpr::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& QuantSetExpr::put(ostream& output, bool longnames) const {
	output << "{";
	for(auto it = quantVars().begin(); it != quantVars().end(); ++it) {
		output << ' '; (*it)->put(output,longnames);
	}
	output << " : "; subformulas()[0]->put(output,longnames);
	output << " : "; subterms()[0]->put(output,longnames); 
	output << " }";
	return output;
}


/****************
	Utilities
****************/

class ApproxTwoValChecker : public TheoryVisitor {
	private:
		AbstractStructure*	_structure;
		bool				_returnvalue;
	public:
		ApproxTwoValChecker(AbstractStructure* str) : _structure(str), _returnvalue(true) { }
		bool	returnvalue()	const { return _returnvalue;	}
		void	visit(const PredForm*);
		void	visit(const FuncTerm*);
};

void ApproxTwoValChecker::visit(const PredForm* pf) {
	PredInter* inter = _structure->inter(pf->symbol());
	if(inter->approxTwoValued()) {
		for(auto it = pf->subterms().begin(); it != pf->subterms().end(); ++it) {
			(*it)->accept(this);
			if(not _returnvalue) { return; }
		}
	}
	else { _returnvalue = false; }
}

void ApproxTwoValChecker::visit(const FuncTerm* ft) {
	FuncInter* inter = _structure->inter(ft->function());
	if(inter->approxTwoValued()) {
		for(auto it = ft->subterms().begin(); it != ft->subterms().end(); ++it) {
			(*it)->accept(this);
			if(not _returnvalue) { return; }
		}
	}
	else { _returnvalue = false; }
}

namespace SetUtils {
	bool approxTwoValued(SetExpr* exp, AbstractStructure* str) {
		ApproxTwoValChecker tvc(str);
		exp->accept(&tvc);
		return tvc.returnvalue();
	}
}

/**
 * Class to implement TermUtils::isPartial
 */
class PartialChecker : public TheoryVisitor {
	private:
		bool			_result;

		void visit(const VarTerm* ) { }
		void visit(const DomainTerm* ) { }
		void visit(const AggTerm* ) { }	// NOTE: we are not interested whether at contains partial functions. 
										// So we don't visit it recursively.

		void visit(const FuncTerm* ft) {
			if(ft->function()->partial()) { 
				_result = true; 
				return;	
			}
			else {
				for(unsigned int argpos = 0; argpos < ft->subterms().size(); ++argpos) {
					if(not SortUtils::isSubsort(ft->subterms()[argpos]->sort(),ft->function()->insort(argpos))) {
						_result = true;
						return;
					}
				}
				TheoryVisitor::traverse(ft);
			}
		}

	public:
		bool run(Term* t) {
			_result = false;
			t->accept(this);
			return _result;
		}
};

namespace TermUtils {

	vector<Term*> makeNewVarTerms(const vector<Variable*>& vars) {
		vector<Term*> terms;
		for(auto it = vars.begin(); it != vars.end(); ++it) {
			terms.push_back(new VarTerm(*it,TermParseInfo()));
		}
		return terms;
	}

	bool isPartial(Term* term) {
		PartialChecker pc;
		return pc.run(term);
	}
}

