/************************************
	tptpprinter.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TPTPPRINTER_HPP_
#define TPTPPRINTER_HPP_

#include "printers/print.hpp"
#include "theory.hpp"
#include "vocabulary.hpp"
#include "namespace.hpp"

//#include <set>

template<typename Stream>
class TPTPPrinter: public StreamPrinter<Stream> {
private:
	bool						_conjecture;
	bool						_arithmetic;
	unsigned int				_count;
	std::set<DomainTerm*>		_typedDomainTerms;
	std::set<std::string>		_typedDomainTermNames;

	using StreamPrinter<Stream>::output;

public:
	TPTPPrinter(bool conjecture, bool arithmetic, Stream& stream):
			StreamPrinter<Stream>(stream),
			_conjecture(conjecture),
			_arithmetic(arithmetic),
			_count(0) { }

	

	void visit(const AbstractStructure* structure) {
	}

	void visit(const Vocabulary* v) {
		for(std::map<std::string,std::set<Sort*> >::const_iterator it = v->firstsort(); it != v->lastsort(); ++it) {
			for(std::set<Sort*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
				if(!(*jt)->builtin() || v == Vocabulary::std()) visit(*jt);
			}
		}
		for(std::map<std::string,Predicate*>::const_iterator it = v->firstpred(); it != v->lastpred(); ++it) {
			if(!it->second->builtin() || v == Vocabulary::std()) visit(it->second);
		}
		for(std::map<std::string,Function*>::const_iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
			if(!it->second->builtin() || v == Vocabulary::std()) visit(it->second);
		}
	}

	void visit(const Namespace* s) {
	}

	void visit(const GroundFixpDef*) {
	}

	void visit(const Theory* t) {
		if(!_conjecture) {
			for(auto it = t->sentences().begin(); it != t->sentences().end(); ++it) {
				if(_arithmetic) {
					output() << "tff";
				} else {
					output() << "fof";
				}
				output() << "(";
				output() << "a" << _count << ",axiom,";
				(*it)->accept(this);
				//output() << ")";
				output() << ").\n";
				_count ++;
			}
		} else if (t->sentences().begin() != t->sentences().end()) {
			// Output a conjecture as a conjunction.
			if(_arithmetic) {
				output() << "tff";
			} else {
				output() << "fof";
			}
			output() << "(cnj,conjecture,";
			auto it = t->sentences().begin();
			while(it != t->sentences().end()) {
				output() << "(";
				(*it)->accept(this);
				output() << ")";
				++ it;
				if(it != t->sentences().end()) {
					output() << "\n & ";
				}
			}
			output() << ").\n";
		}
		// Do nothing with definitions or fixpoint definitions
		//if (!_conjecture) // TODO: Is this right, can I just leave them out?
		outputDomainTermTypeAxioms();
	}
	
	void outputDomainTermTypeAxioms() {
		for(auto it = _typedDomainTerms.begin(); it != _typedDomainTerms.end(); ++ it) {
			if(_arithmetic) {
				output() << "tff";
			} else {
				output() << "fof";
			}
			output() << "(";
			// This will cause axioms to be double-printed sometimes, but it's better
			// than having them never printed. I'm not sure whether I need them.
			if (_conjecture) {
				output() << "dttac";
			}
			else {
				output() << "dtta";
			}
			output() << _count << ",axiom,(";
			output() << rewriteSortName((*it)->sort()->name()) << "(";
			outputDomainTermName((*it));
			output() << ")";
			output() << ")).\n";
			_count ++;
		}
		_typedDomainTerms.clear();
	}

	void visit(const GroundTheory* g) {
	}
	
	std::string rewriteLongname(const std::string& longname) {
		// Fancy stuff here.
		std::string result = longname;
		
		// Remove types
		auto pos = result.find("[");
		result.erase(pos);
		
		// Append 1 more '_' to sequences of 3 or more '_'s
		pos = result.find("___");
		while(pos != std::string::npos) {
			while(result[pos + 3] == '_') {
				++ pos;
			}
			result.replace(pos, 3, "____");
			pos = result.find("___", pos + 3);
		}
		
		// Replace :: with ___
		pos = result.find("::");
		while(pos != std::string::npos) {
			result.replace(pos, 2, "___");
			pos = result.find("::", pos + 3);
		}
		return result;
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		if(! f->sign())	output() << "~";
		if(f->symbol()->to_string(false) == "=") {
			output() << "(";
			f->subterms()[0]->accept(this);
			output() << " = ";
			f->subterms()[1]->accept(this);
			output() << ")";
		} else {
			output() << "p_" << rewriteLongname(f->symbol()->to_string(true));
			if(!f->subterms().empty()) {
				output() << "(";
				f->subterms()[0]->accept(this);
				for(unsigned int n = 1; n < f->subterms().size(); ++n) {
					output() << ",";
					f->subterms()[n]->accept(this);
				}
				output() << ")";
			}
		}
	}

	// TODO TFF
	void visit(const EqChainForm* f) {
		if(! f->sign())	output() << "~";
		output() << "(";
		f->subterms()[0]->accept(this);
		if (!_arithmetic) {
			for(unsigned int n = 0; n < f->comps().size(); ++n) {
				switch(f->comps()[n]) {
					case CT_EQ:
						output() << " = ";
						break;
					case CT_NEQ:
						output() << " != ";
						break;
					// case CT_LEQ:
					// 	output() << " =< ";
					// 	break;
					// case CT_GEQ:
					// 	output() << " >= ";
					// 	break;
					// case CT_LT:
					// 	output() << " < ";
					// 	break;
					// case CT_GT:
					// 	output() << " > ";
					// 	break;
				}
				f->subterms()[n+1]->accept(this);
				if(n+1 < f->comps().size()) {
					if(f->conj())
						output() << " & ";
					else
						output() << " | ";
					f->subterms()[n+1]->accept(this);
				}
			}
		} else {
			for(unsigned int n = 0; n < f->comps().size(); ++n) {
				// TODO
			}
		}
		output() << ")";
	}

	void visit(const EquivForm* f) {
		if(! f->sign())	output() << "~";
		output() << "(";
		f->left()->accept(this);
		output() << " <=> ";
		f->right()->accept(this);
		output() << ")";
	}

	void visit(const BoolForm* f) {
		if(f->subformulas().empty()) {
			if(f->sign() == f->conj())
				output() << "$true";
			else
				output() << "$false";
		}
		else {
			if(! f->sign())	output() << "~";
			output() << "(";
			f->subformulas()[0]->accept(this);
			for(unsigned int n = 1; n < f->subformulas().size(); ++n) {
				if(f->conj())
					output() << " & ";
				else
					output() << " | ";
				f->subformulas()[n]->accept(this);
			}
			output() << ")";
		}
	}

	// TODO: SORTS!
	void visit(const QuantForm* f) {
		if(! f->sign())	output() << "~";
		output() << "(";
		if(f->univ())
			output() << "! [";
		else
			output() << "? [";
		auto it = f->quantvars().begin();
		output() << "V_" << (*it)->name();
		++ it;
		for(; it != f->quantvars().end(); ++it) {
			output() << ",";
			output() << "V_" << (*it)->name();
			//if((*it)->sort())
			//	output() << "[" << *((*it)->sort()) << "]";
		}
		output() << "] : (";
		it = f->quantvars().begin();
		while(it != f->quantvars().end() && !((*it)->sort()))
			++ it;
		if (it != f->quantvars().end()) {
		 	if(f->univ())
				output() << "~";
			output() << "(";
			output() << rewriteSortName((*it)->sort()->name()) << "(V_" << (*it)->name() << ")";
			++ it;
			for(; it != f->quantvars().end(); ++it) {
				if((*it)->sort()) {
					output() << " & ";
					output() << rewriteSortName((*it)->sort()->name()) << "(V_" << (*it)->name() << ")";
				}
			}
			output() << ")";
			if(f->univ())
				output() << " | ";
			else
				output() << " & ";
		}
		output() << "(";
		f->subformulas()[0]->accept(this);
		output() << ")))";
	}

	/** Definitions **/

	void visit(const Rule* r) {
	}

	void visit(const Definition* d) {
	}

	void visit(const FixpDef* d) {
	}

	/** Terms **/

	void visit(const VarTerm* t) {
		output() << "V_" << t->var()->name();
	}

	// TODO: Handle predefined functions
	// Actually, we won't have any of these anymore...
	void visit(const FuncTerm* t) {
		output() << "f_" << rewriteLongname(t->function()->to_string(true));
		if(!t->subterms().empty()) {
			output() << "(";
			t->subterms()[0]->accept(this);
			for(unsigned int n = 1; n < t->subterms().size(); ++n) {
				output() << ",";
				t->subterms()[n]->accept(this);
			}
			output() << ")";
		}
	}

	// TODO: Figure out what to do with these.
	// I hope this is okay...
	void visit(const DomainTerm* t) {
		if(t->sort() && _typedDomainTermNames.find(t->value()->to_string()) == _typedDomainTermNames.end()) {
			_typedDomainTermNames.insert(t->value()->to_string());
			_typedDomainTerms.insert(const_cast<DomainTerm*>(t));
		}
		outputDomainTermName(t);
	}
	
	void outputDomainTermName(const DomainTerm* t) {
		std::string str = t->value()->to_string();
		if(t->sort()) {
			if(SortUtils::isSubsort(t->sort(),VocabularyUtils::charsort())) {
				output() << "char_" << str;
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::stringsort())) {
				output() << '\"' << str << '\"';
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::floatsort())) {
				output() << str;
			}
			else {
				output() << "tt_" << str;
			}
		}
		else output() << "utt_" << str;
	}

	void visit(const AggTerm* t) {
	}

	void visit(const GroundClause& g){
	}

	void visit(const GroundDefinition* d) {
	}

	void visit(int defid, int tseitin, const PCGroundRuleBody* b) {
	}

	void visit(const PCGroundRuleBody* b) {
	}

	void visit(const AggGroundRuleBody* b) {
	}

	void visit(int defid, const GroundAggregate* b) {
	}

	void visit(const GroundAggregate* a) {
	}

	void visit(const CPReification* cpr) {
	}

	void visit(const CPSumTerm* cpt) {
	}

	void visit(const CPWSumTerm* cpt) {
	}

	void visit(const CPVarTerm* cpt) {
	}


	void visit(const PredTable* table) {
	}

	void visit(FuncTable* table) {
	}
	
	// TODO: How to handle types regularly and types in tff?
	std::string rewriteSortName(const std::string& sortName) {
		if(_arithmetic) {
			// TODO: Do this properly
			if(sortName == "int") {
				return "$int";
			}
			else if(sortName == "float") {
				return "$real";
			}
			else {
				return "t_" + sortName;
			}
		}
		else {
			return "t_" + sortName;
		}
	}

	void visit(const Sort* s) {
		if (!(s->parents().empty())) {
			if (_arithmetic) {
				output() << "tff";
			} else {
				output() << "fof";
			}
			output() << "(";
			if(_conjecture) {
				output() << "cta";
			} else {
				output() << "ta";
			}
			output() << _count << ",axiom,(";
			output() << "! [X] : (~" << rewriteSortName(s->name()) << "(X) | ";
			auto it = s->parents().begin();
			output() << "(" << rewriteSortName((*it)->name()) << "(X)";
			++it;
			for(; it != s->parents().end(); ++it) {
				output() << " & " << rewriteSortName((*it)->name()) << "(X)";
			}
			output() << ")))).\n";
			_count ++;
		}
	}

	// TODO: in TFF, handle ints and floats (and nats)
	void visit(const Predicate* p) {
		if (!p->overloaded() && p->arity() > 0) {
			if (_arithmetic) {
				output() << "tff";
			} else {
				output() << "fof";
			}
			output() << "(";
			if(_conjecture) {
				output() << "cta";
			} else {
				output() << "ta";
			}
			output() << _count;
			output() << ",axiom,(";
			output() << "! [";
			output() << "V0";
			for(unsigned int n = 1; n < p->arity(); ++n) {
				output() << ",V" << n;
			}
			output() << "] : (";
			if(p->arity() != 1 || p->to_string(false) != p->sort(0)->name())
				output() << "~";
			output() << "p_" << rewriteLongname(p->to_string(true)) << "(";
			output() << "V0";
			for(unsigned int n = 1; n < p->arity(); ++n) {
				output() << ",V" << n;
			}
			if(p->arity() == 1 && p->to_string(false) == p->sort(0)->name())
				output() << ") <=> (";
			else
				output() << ") | (";
			output() << rewriteSortName(p->sort(0)->name()) << "(V0)";
			for(unsigned int n = 1; n < p->arity(); ++n) {
				output() << " & " << rewriteSortName(p->sort(n)->name()) << "(V" << n << ")";
			}
			output() << ")))).\n";
			_count ++;
		}
	}

	void visit(const Function* f) {
		if (!f->overloaded()) {
			if (_arithmetic) {
				output() << "tff";
			} else {
				output() << "fof";
			}
			output() << "(";
			if(_conjecture) {
				output() << "cta";
			} else {
				output() << "ta";
			}
			output() << _count;
			output() << ",axiom,(";
			output() << "! [";
			output() << "V0";
			for(unsigned int n = 1; n < f->arity() + 1; ++n) {
				output() << ",V" << n;
			}
			output() << "] : (";
			// TODO: Waar ga je het onderscheid nog zien?
			// You won't, but names may not clash, so you can call these p_ too
			output() << "~p_" << rewriteLongname(f->to_string(true)) << "(";
			output() << "V0";
			for(unsigned int n = 1; n < f->arity() + 1; ++n) {
				output() << ",V" << n;
			}
			output() << ") | (";
			output() << rewriteSortName(f->sort(0)->name()) << "(V0)";
			for(unsigned int n = 1; n < f->arity() + 1; ++n) {
				output() << " & " << rewriteSortName(f->sort(n)->name()) << "(V" << n << ")";
			}
			output() << ")))).\n";
			if(f->arity() > 0 && !f->partial()) {
				outputTotalFuncAxiom(f);
			}
			else if(f->arity() > 0) {
				outputPartialFuncAxiom(f);
			}
			else {
				outputArity0FuncAxiom(f);
			}
			_count ++;
		}
	}
	
	void outputTotalFuncAxiom(const Function* f) {
		if(_arithmetic) {
			output() << "tff";
		} else {
			output() << "fof";
		}
		output() << "(tfa" << _count << ",axiom,(";
		output() << "! [";
		output() << "V0";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			output() << ",V" << n;
		}
		output() << "] : (";
		output() << "~(";
		output() << rewriteSortName(f->sort(0)->name()) << "(V0)";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			output() << " & " << rewriteSortName(f->sort(n)->name()) << "(V" << n << ")";
		}
		output() << ") | ";
		output() << "(? [X1] : (";
		output() << rewriteSortName(f->outsort()->name()) << "(X1) & ";
		output() << "(! [X2] : (";
		output() << "p_" << rewriteLongname(f->to_string(true)) << "(";
		output() << "V0";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			output() << ",V" << n;
		}
		output() << ",X2) <=> X1 = X2";
		output() << "))))))).\n";
	}
	
	void outputPartialFuncAxiom(const Function* f) {
		if(_arithmetic) {
			output() << "tff";
		} else {
			output() << "fof";
		}
		output() << "(fa" << _count << ",axiom,(";
		output() << "! [";
		output() << "V0";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			output() << ",V" << n;
		}
		output() << "] : (";
		output() << "~(";
		output() << rewriteSortName(f->sort(0)->name()) << "(V0)";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			output() << " & " << rewriteSortName(f->sort(n)->name()) << "(V" << n << ")";
		}
		output() << ") | ";
		output() << "(? [X1] : (";
		output() << rewriteSortName(f->outsort()->name()) << "(X1) & ";
		output() << "(! [X2] : (";
		output() << "~p_" << rewriteLongname(f->to_string(true)) << "(";
		output() << "V0";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			output() << ",V" << n;
		}
		output() << ",X2) | X1 = X2";
		output() << "))))))).\n";
	}
	
	void outputArity0FuncAxiom(const Function* f) {
		if(_arithmetic) {
			output() << "tff";
		} else {
			output() << "fof";
		}
		output() << "(ta0fa" << _count << ",axiom,";
		output() << "(? [X1] : (";
		output() << rewriteSortName(f->outsort()->name()) << "(X1) & ";
		output() << "(! [X2] : (";
		output() << "p_" << rewriteLongname(f->to_string(true)) << "(X2)";
		output() << " <=> X1 = X2";
		output() << "))))).\n";
	}

	void visit(SortTable* table) {
	}

	void visit(const GroundSet* s) {
	}
};

#endif /* TPTPPRINTER_HPP_ */
