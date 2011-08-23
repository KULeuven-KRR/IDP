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
using namespace std;

/*********************
	Abstract terms
*********************/

void Term::setfvars() {
	_freevars.clear();
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		_freevars.insert((*it)->freevars().begin(),(*it)->freevars().end());
	}
	for(vector<SetExpr*>::const_iterator it = _subsets.begin(); it != _subsets.end(); ++it) {
		_freevars.insert((*it)->freevars().begin(),(*it)->freevars().end());
	}
}

void Term::recursiveDelete() {
	for(vector<Term*>::iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(vector<SetExpr*>::iterator it = _subsets.begin(); it != _subsets.end(); ++it) {
		(*it)->recursiveDelete();
	}
	delete(this);
}

bool Term::contains(const Variable* v) const {
	for(set<Variable*>::const_iterator it = _freevars.begin(); it != _freevars.end(); ++it) {
		if(*it == v) return true;
	}
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	for(vector<SetExpr*>::const_iterator it = _subsets.begin(); it != _subsets.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	return false;
}

string Term::to_string() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}

ostream& operator<<(ostream& output, const Term& t) {
	return t.put(output);
}

/***************
	VarTerm
***************/

void VarTerm::setfvars() {
	_freevars.clear();
	_freevars.insert(_var);
}

void VarTerm::sort(Sort* s) {
	_var->sort(s);
}

VarTerm::VarTerm(Variable* v, const TermParseInfo& pi) : Term(pi), _var(v) {
	setfvars();
}

VarTerm* VarTerm::clone() const {
	return new VarTerm(_var,_pi.clone());
}

VarTerm* VarTerm::clone(const map<Variable*,Variable*>& mvv) const {
	map<Variable*,Variable*>::const_iterator it = mvv.find(_var);
	if(it != mvv.end()) return new VarTerm(it->second,_pi);
	else return new VarTerm(_var,_pi.clone(mvv));
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

ostream& VarTerm::put(std::ostream& output) const {
	output << *_var;
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
	for(vector<Term*>::const_iterator it = subterms().begin(); it != subterms().end(); ++it)
		newargs.push_back((*it)->clone(mvv));
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

ostream& FuncTerm::put(ostream& output) const {
	output << *_function;
	if(!subterms().empty()) {
		output << '(' << *subterms()[0];
		for(unsigned int n = 1; n < subterms().size(); ++n) {
			output << ',' << *subterms()[n];
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

ostream& DomainTerm::put(ostream& output) const {
	output << *_value;
	return output;
}

/**************
	AggTerm
**************/

AggTerm::AggTerm(SetExpr* set, AggFunction function, const TermParseInfo& pi) :
	Term(pi), _function(function) {
	addset(set);
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

ostream& AggTerm::put(ostream& output) const {
	switch(_function) {
		case AggFunction::CARD:	output << '#'; break;
		case AggFunction::SUM:	output << "sum"; break;
		case AggFunction::PROD:	output << "prod"; break;
		case AggFunction::MIN:	output << "min"; break;
		case AggFunction::MAX:	output << "max"; break;
	}
	output << *subsets()[0];
	return output;
}

/**************
	SetExpr
**************/

void SetExpr::setfvars() {
	_freevars.clear();
	for(vector<Formula*>::const_iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		_freevars.insert((*it)->freevars().begin(),(*it)->freevars().end());
	}
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		_freevars.insert((*it)->freevars().begin(),(*it)->freevars().end());
	}
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		_freevars.erase(*it);
	}
}

void SetExpr::recursiveDelete() {
	for(vector<Formula*>::iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(vector<Term*>::iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		(*it)->recursiveDelete();
	}
	for(set<Variable*>::iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		delete(*it);
	}
	delete(this);
}

bool SetExpr::contains(const Variable* v) const {
	for(set<Variable*>::const_iterator it = _freevars.begin(); it != _freevars.end(); ++it) {
		if(*it == v) return true;
	}
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		if(*it == v) return true;
	}
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	for(vector<Formula*>::const_iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		if((*it)->contains(v)) return true;
	}
	return false;
}

std::string SetExpr::to_string() const {
	stringstream sstr;
	put(sstr);
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
	setfvars();
}

EnumSetExpr* EnumSetExpr::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EnumSetExpr* EnumSetExpr::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> newforms;
	vector<Term*> newweights;
	for(vector<Formula*>::const_iterator it = _subformulas.begin(); it != _subformulas.end(); ++it) {
		newforms.push_back((*it)->clone(mvv));
	}
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		newweights.push_back((*it)->clone(mvv));
	}
	return new EnumSetExpr(newforms,newweights,_pi.clone(mvv));
}

Sort* EnumSetExpr::sort() const {
	Sort* currsort = VocabularyUtils::natsort();
	for(vector<Term*>::const_iterator it = _subterms.begin(); it != _subterms.end(); ++it) {
		if((*it)->sort()) currsort = SortUtils::resolve(currsort,(*it)->sort());
		else return 0;
	}
	if(currsort) {
		if(SortUtils::isSubsort(currsort,VocabularyUtils::natsort())) return VocabularyUtils::natsort();
		else if(SortUtils::isSubsort(currsort,VocabularyUtils::intsort())) return VocabularyUtils::intsort();
		else if(SortUtils::isSubsort(currsort,VocabularyUtils::floatsort())) return VocabularyUtils::floatsort();
		else return 0;
	}
	else return 0;
}

void EnumSetExpr::accept(TheoryVisitor* v) const {
	v->visit(this);
}

SetExpr* EnumSetExpr::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& EnumSetExpr::put(ostream& output) const {
	output << "[ ";
	if(!_subformulas.empty()) {
		output << '(' << *_subformulas[0] << ',' << *_subterms[0] << ')';
		for(unsigned int n = 1; n < _subformulas.size(); ++n) {
			output << "; (" << *_subformulas[n] << ',' << *_subterms[n] << ')';
		}
	}
	output << " ]";
	return output;
}

/*******************
	QuantSetExpr
*******************/

QuantSetExpr::QuantSetExpr(const set<Variable*>& qvars, Term* term, Formula* formula, const SetParseInfo& pi) :
	SetExpr(pi) {
	_quantvars = qvars;
	_subterms.push_back(term);
	_subformulas.push_back(formula);
	setfvars();
}

QuantSetExpr* QuantSetExpr::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

QuantSetExpr* QuantSetExpr::clone(const map<Variable*,Variable*>& mvv) const {
	set<Variable*> newvars;
	map<Variable*,Variable*> nmvv = mvv;
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		Variable* nv = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
		newvars.insert(nv);
		nmvv[*it] = nv;
	}
	Term* newterm = _subterms[0]->clone(nmvv);
	Formula* nf = _subformulas[0]->clone(nmvv);
	return new QuantSetExpr(newvars,newterm,nf,_pi.clone(mvv));
}

Sort* QuantSetExpr::sort() const {
	Sort* termsort = (*_subterms.begin())->sort();
	if(termsort) {
		if(SortUtils::isSubsort(termsort,VocabularyUtils::natsort())) return VocabularyUtils::natsort();
		else if(SortUtils::isSubsort(termsort,VocabularyUtils::intsort())) return VocabularyUtils::intsort();
		else if(SortUtils::isSubsort(termsort,VocabularyUtils::floatsort())) return VocabularyUtils::floatsort();
		else return 0;
	}
	else return 0;
}

void QuantSetExpr::accept(TheoryVisitor* v) const {
	v->visit(this);
}

SetExpr* QuantSetExpr::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

ostream& QuantSetExpr::put(ostream& output) const {
	output << "{";
	for(set<Variable*>::const_iterator it = _quantvars.begin(); it != _quantvars.end(); ++it) {
		output << ' ' << *(*it);
	}
	output << " : " << *_subterms[0] << " : ";
	output << *_subformulas[0] << " }";
	return output;
}

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
	if(inter->approxtwovalued()) {
		for(vector<Term*>::const_iterator it = pf->subterms().begin(); it != pf->subterms().end(); ++it) {
			(*it)->accept(this);
			if(!_returnvalue) return;
		}
	}
	else _returnvalue = false;
}

void ApproxTwoValChecker::visit(const FuncTerm* ft) {
	FuncInter* inter = _structure->inter(ft->function());
	if(inter->approxtwovalued()) {
		for(vector<Term*>::const_iterator it = ft->subterms().begin(); it != ft->subterms().end(); ++it) {
			(*it)->accept(this);
			if(!_returnvalue) return;
		}
	}
	else _returnvalue = false;
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
					if(!SortUtils::isSubsort(ft->subterms()[argpos]->sort(),ft->function()->insort(argpos))) {
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
		for(vector<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
			terms.push_back(new VarTerm(*it,TermParseInfo()));
		}
		return terms;
	}

	bool isPartial(Term* term) {
		PartialChecker pc;
		return pc.run(term);
	}
}

