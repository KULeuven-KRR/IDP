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

#include <iostream>

//#include <set>

template<typename Stream>
class TPTPPrinter: public StreamPrinter<Stream> {
private:
	bool						_conjecture;
	bool						_arithmetic;
	unsigned int				_count;
	std::set<DomainTerm*>		_typedDomainTerms;
	std::set<std::string>		_typedDomainTermNames;
	std::stringstream			_typeStream; // The types. (for TFF)
	std::stringstream			_typeAxiomStream; // The type predicates (defining what atomic symbols have what type,
												  // and which functions/predicates take which types)
	std::stringstream			_axiomStream; // The first theory as axioms
	std::stringstream			_conjectureStream; // The second theory as conjectures

	using StreamPrinter<Stream>::output;

public:
	TPTPPrinter(bool conjecture, bool arithmetic, Stream& stream):
			StreamPrinter<Stream>(stream),
			_conjecture(conjecture),
			_arithmetic(arithmetic),
			_count(0) { }
	
	bool conjecture() {
		return _conjecture;
	}

	void conjecture(bool conjecture) {
		_conjecture = conjecture;
	}

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
					_axiomStream << "tff";
				} else {
					_axiomStream << "fof";
				}
				_axiomStream << "(";
				_axiomStream << "a" << _count << ",axiom,";
				(*it)->accept(this);
				_axiomStream << ").\n";
				_count ++;
			}
		} else if (t->sentences().begin() != t->sentences().end()) {
			// Output a conjecture as a conjunction.
			if(_arithmetic) {
				_conjectureStream << "tff";
			} else {
				_conjectureStream << "fof";
			}
			_conjectureStream << "(cnj,conjecture,";
			auto it = t->sentences().begin();
			while(it != t->sentences().end()) {
				_conjectureStream << "(";
				(*it)->accept(this);
				_conjectureStream << ")";
				++ it;
				if(it != t->sentences().end()) {
					_conjectureStream << "\n & ";
				}
			}
			_conjectureStream << ").\n";
		}
		// Do nothing with definitions or fixpoint definitions
		//if (!_conjecture) // TODO: Is this right, can I just leave them out?
		outputDomainTermTypeAxioms();
		if (_conjecture)
			writeStreams();
	}
	
	void writeStreams() {
		// Add t_nat, t_int and t_float types:
		if (_arithmetic) {
			output() << "tff(nat_type,type,(t_nat: $int > $o)).\n";
			output() << "tff(int_type,type,(t_int: $int > $o)).\n";
			//output() << "tff(float_type,type,(t_float: $real > $o)).\n";
		}
		output() << _typeStream.str();
		//output() << _funcPredTypeStream.str();
		// Add t_nat > t_int > t_float hierarchy
		output() << _typeAxiomStream.str();
		if (_arithmetic) {
			output() << "tff(nat_is_int,axiom,(! [X: $int] : (~t_nat(X) | t_int(X)))).\n";
			//output() << "tff(int_is_float,axiom,(! [X: $int] : ~t_int(X) | t_float(X))).\n";
		}
		output() << _axiomStream.str();
		output() << _conjectureStream.str();
		_typeStream.str(std::string());
		//_funcPredTypeStream.str(std::string());
		_typeAxiomStream.str(std::string());
		_axiomStream.str(std::string());
		_conjectureStream.str(std::string());
	}
	
	void outputDomainTermTypeAxioms() {
		for(auto it = _typedDomainTerms.begin(); it != _typedDomainTerms.end(); ++ it) {
			if(_arithmetic) {
				_typeAxiomStream << "tff";
			} else {
				_typeAxiomStream << "fof";
			}
			_typeAxiomStream << "(";
			// This will cause axioms to be double-printed sometimes, but it's better
			// than having them never printed. I'm not sure whether I need them.
			if (_conjecture) {
				_typeAxiomStream << "dttac";
			}
			else {
				_typeAxiomStream << "dtta";
			}
			_typeAxiomStream << _count << ",axiom,(";
			std::string sortName = (*it)->sort()->name();
			_typeAxiomStream << "t_" << sortName << "(";
			outputDomainTermName((*it), &_typeAxiomStream);
			_typeAxiomStream << ")";
			_typeAxiomStream << ")).\n";
			if(_arithmetic && sortName != "int" && sortName != "nat" && sortName != "float") { // TODO: output these only ONCE! Also, output them at the START!
 				_typeStream << "tff(";
 				if (_conjecture) {
 					_typeStream << "dttc";
 				}
 				else {
 					_typeStream << "dtt";
 				}
 				_typeStream << _count << ",type,(";
 				_typeStream << "t_" << (*it)->sort()->name() << ": ";
 				outputTFFType((*it)->sort(), &_typeStream);
 				_typeStream << " > $o";
 				_typeStream << ")).\n";
 			}
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
		if (pos != std::string::npos)
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
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		if(! f->sign())	outputStream << "~";
		if(f->symbol()->to_string(false) == "=") {
			outputStream << "(";
			f->subterms()[0]->accept(this);
			outputStream << " = ";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == ">") {
			outputStream << "$greater(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "<") {
			outputStream << "$less(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "+") {
			outputStream << "($sum(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ") = ";
			f->subterms()[2]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "-" && f->subterms().size() == 3) {
			outputStream << "($difference(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ") = ";
			f->subterms()[2]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "-" && f->subterms().size() == 2) {
			outputStream << "($uminus(";
			f->subterms()[0]->accept(this);
			outputStream << ") = ";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "*") {
			outputStream << "($product(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ") = ";
			f->subterms()[2]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "PRED") {
			outputStream << "($difference(";
			f->subterms()[0]->accept(this);
			outputStream << ",1) = ";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "SUCC") {
			outputStream << "($sum(";
			f->subterms()[0]->accept(this);
			outputStream << ",1) = ";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "MAX") {
			// NOTE: $itett is often unsupported
			outputStream << "($itett($greater(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << "),";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ") = ";
			f->subterms()[2]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "MIN") {
			// NOTE: $itett is often unsupported
			outputStream << "($itett($less(";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << "),";
			f->subterms()[0]->accept(this);
			outputStream << ",";
			f->subterms()[1]->accept(this);
			outputStream << ") = ";
			f->subterms()[2]->accept(this);
			outputStream << ")";
		} else if(f->symbol()->to_string(false) == "abs") {
			// NOTE: $itett is often unsupported
			outputStream << "($itett($greater(";
			f->subterms()[0]->accept(this);
			outputStream << ",$uminus(";
			f->subterms()[0]->accept(this);
			outputStream << ")),";
			f->subterms()[0]->accept(this);
			outputStream << ",$uminus(";
			f->subterms()[0]->accept(this);
			outputStream << ")) = ";
			f->subterms()[1]->accept(this);
			outputStream << ")";
		} else {
			outputStream << "p_" << rewriteLongname(f->symbol()->to_string(true));
			if(!f->subterms().empty()) {
				outputStream << "(";
				f->subterms()[0]->accept(this);
				for(unsigned int n = 1; n < f->subterms().size(); ++n) {
					outputStream << ",";
					f->subterms()[n]->accept(this);
				}
				outputStream << ")";
			}
		}
	}

	// TODO TFF
	void visit(const EqChainForm* f) {
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		if(! f->sign())	outputStream << "~";
		outputStream << "(";
		f->subterms()[0]->accept(this);
		for(unsigned int n = 0; n < f->comps().size(); ++n) {
			switch(f->comps()[n]) {
				case CT_EQ:
					outputStream << " = ";
					break;
				case CT_NEQ:
					outputStream << " != ";
					break;
			}
			f->subterms()[n+1]->accept(this);
			if(n+1 < f->comps().size()) {
				if(f->conj())
					outputStream << " & ";
				else
					outputStream << " | ";
				f->subterms()[n+1]->accept(this);
			}
		}
		outputStream << ")";
	}

	void visit(const EquivForm* f) {
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		if(! f->sign())	outputStream << "~";
		outputStream << "(";
		f->left()->accept(this);
		outputStream << " <=> ";
		f->right()->accept(this);
		outputStream << ")";
	}

	void visit(const BoolForm* f) {
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		if(f->subformulas().empty()) {
			if(f->sign() == f->conj())
				outputStream << "$true";
			else
				outputStream << "$false";
		}
		else {
			if(! f->sign())	outputStream << "~";
			outputStream << "(";
			f->subformulas()[0]->accept(this);
			for(unsigned int n = 1; n < f->subformulas().size(); ++n) {
				if(f->conj())
					outputStream << " & ";
				else
					outputStream << " | ";
				f->subformulas()[n]->accept(this);
			}
			outputStream << ")";
		}
	}
	
	void outputTFFType(const Sort* s, std::stringstream* outputStreamPtr) {
		std::stringstream& outputStream = *outputStreamPtr;
		if(SortUtils::isSubsort(const_cast<Sort*>(s),VocabularyUtils::natsort())) {
			//output() << ": nat"; // TODO: What to do with nats? Just add an axiom? Or a type?
			outputStream << "$int";
		}
		else if(SortUtils::isSubsort(const_cast<Sort*>(s),VocabularyUtils::intsort())) {
			outputStream << "$int";
		}
		else if(SortUtils::isSubsort(const_cast<Sort*>(s),VocabularyUtils::floatsort())) {
			outputStream << "$float";
		}
		else {
			outputStream << "$tType";
		}
	}

	// TODO: SORTS!
	void visit(const QuantForm* f) {
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		if(! f->sign())	outputStream << "~";
		outputStream << "(";
		if(f->univ())
			outputStream << "! [";
		else
			outputStream << "? [";
		auto it = f->quantvars().begin();
		outputStream << "V_" << (*it)->name();
		if(_arithmetic && (*it)->sort()) {
			outputStream << ": ";
			outputTFFType((*it)->sort(), outputStreamPtr);
		}
		++ it;
		for(; it != f->quantvars().end(); ++it) {
			outputStream << ",";
			outputStream << "V_" << (*it)->name();
			if(_arithmetic && (*it)->sort()) {
				outputStream << ": ";
				outputTFFType((*it)->sort(), outputStreamPtr);
			}
		}
		outputStream << "] : (";
		
		// When quantifying over types, add these.
		it = f->quantvars().begin();
		while(it != f->quantvars().end() && !((*it)->sort()))
			++ it;
		if (it != f->quantvars().end()) {
		 	if(f->univ())
				outputStream << "~";
			outputStream << "(";
			outputStream << "t_" << (*it)->sort()->name() << "(V_" << (*it)->name() << ")";
			++ it;
			for(; it != f->quantvars().end(); ++it) {
				if((*it)->sort()) {
					outputStream << " & ";
					outputStream << "t_" << (*it)->sort()->name() << "(V_" << (*it)->name() << ")";
				}
			}
			outputStream << ")";
			if(f->univ())
				outputStream << " | ";
			else
				outputStream << " & ";
		}
		
		outputStream << "(";
		f->subformulas()[0]->accept(this);
		outputStream << ")))";
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
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		outputStream << "V_" << t->var()->name();
	}

	// TODO: Handle predefined functions
	// Actually, we won't have any of these anymore...
	void visit(const FuncTerm* t) {
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		outputStream << "f_" << rewriteLongname(t->function()->to_string(true));
		if(!t->subterms().empty()) {
			outputStream << "(";
			t->subterms()[0]->accept(this);
			for(unsigned int n = 1; n < t->subterms().size(); ++n) {
				outputStream << ",";
				t->subterms()[n]->accept(this);
			}
			outputStream << ")";
		}
	}

	// TODO: Figure out what to do with these.
	// I hope this is okay...
	void visit(const DomainTerm* t) {
		std::stringstream* outputStreamPtr;
		if(_conjecture) {
			outputStreamPtr = &_conjectureStream;
		} else {
			outputStreamPtr = &_axiomStream;
		}
		std::stringstream& outputStream = *outputStreamPtr;
		if(t->sort() && _typedDomainTermNames.find(t->value()->to_string()) == _typedDomainTermNames.end()) {
			_typedDomainTermNames.insert(t->value()->to_string());
			_typedDomainTerms.insert(const_cast<DomainTerm*>(t));
		}
		outputDomainTermName(t, outputStreamPtr);
	}
	
	void outputDomainTermName(const DomainTerm* t, std::stringstream* outputStreamPtr) {
		std::stringstream& outputStream = *outputStreamPtr;
		std::string str = t->value()->to_string();
		if(t->sort()) {
			if(SortUtils::isSubsort(t->sort(),VocabularyUtils::charsort())) {
				outputStream << "char_" << str;
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::stringsort())) {
				outputStream << '\"' << str << '\"';
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::floatsort())) {
				outputStream << str;
			}
			else {
				outputStream << "tt_" << str;
			}
		}
		else outputStream << "utt_" << str;
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

	void visit(const Sort* s) {
		if (!(s->parents().empty())) {
			if (_arithmetic) {
				_typeAxiomStream << "tff";
			} else {
				_typeAxiomStream << "fof";
			}
			_typeAxiomStream << "(";
			if(_conjecture) {
				_typeAxiomStream << "cta";
			} else {
				_typeAxiomStream << "ta";
			}
			_typeAxiomStream << _count << ",axiom,(";
			_typeAxiomStream << "! [X";
			if (_arithmetic) {
				_typeAxiomStream << ": ";
				outputTFFType(s, &_typeAxiomStream);
			}
			_typeAxiomStream << "] : (~" << "t_" << s->name() << "(X) | ";
			auto it = s->parents().begin();
			_typeAxiomStream << "(" << "t_" << (*it)->name() << "(X)";
			++it;
			for(; it != s->parents().end(); ++it) {
				_typeAxiomStream << " & " << "t_" << (*it)->name() << "(X)";
			}
			_typeAxiomStream << ")))).\n";
			_count ++;
		}
	}

	// TODO: in TFF, handle ints and floats (and nats)
	void visit(const Predicate* p) {
		if (!p->overloaded() && p->arity() > 0) {
			if (_arithmetic) {
				outputTFFPFSymbolType(p);
				_typeAxiomStream << "tff";
			} else {
				_typeAxiomStream << "fof";
			}
			_typeAxiomStream << "(";
			if(_conjecture) {
				_typeAxiomStream << "cta";
			} else {
				_typeAxiomStream << "ta";
			}
			_typeAxiomStream << _count;
			_typeAxiomStream << ",axiom,(";
			_typeAxiomStream << "! [";
			_typeAxiomStream << "V0";
			if(_arithmetic) {
				_typeAxiomStream << ": ";
				outputTFFType(p->sort(0), &_typeAxiomStream);
			}
			for(unsigned int n = 1; n < p->arity(); ++n) {
				_typeAxiomStream << ",V" << n;
				if (_arithmetic) {
					_typeAxiomStream << ": ";
					outputTFFType(p->sort(n), &_typeAxiomStream);
				}
			}
			_typeAxiomStream << "] : (";
			if(p->arity() != 1 || p->to_string(false) != p->sort(0)->name())
				_typeAxiomStream << "~";
			_typeAxiomStream << "p_" << rewriteLongname(p->to_string(true)) << "(";
			_typeAxiomStream << "V0";
			for(unsigned int n = 1; n < p->arity(); ++n) {
				_typeAxiomStream << ",V" << n;
			}
			if(p->arity() == 1 && p->to_string(false) == p->sort(0)->name())
				_typeAxiomStream << ") <=> (";
			else
				_typeAxiomStream << ") | (";
			_typeAxiomStream << "t_" << p->sort(0)->name() << "(V0)";
			for(unsigned int n = 1; n < p->arity(); ++n) {
				_typeAxiomStream << " & " << "t_" << p->sort(n)->name() << "(V" << n << ")";
			}
			_typeAxiomStream << ")))).\n";
			_count ++;
		}
		else {
			std::cout << "Predicate: " << p->to_string(true) << std::endl;
		}
	}
	
	void outputTFFPFSymbolType(const PFSymbol* pfs) {
		_typeStream << "tff(t" << _count;
		_typeStream << ",type,(";
		_typeStream << "p_" << rewriteLongname(pfs->to_string(true));
		_typeStream << ": ";
		if (pfs->nrSorts() > 1) {
			_typeStream << "(";
		}
		for(unsigned int n = 0; n < pfs->nrSorts(); ++ n) {
			if(pfs->sort(n)) {
				outputTFFType(pfs->sort(n), &_typeStream);
			}
			else {
				_typeStream << "$tType";
			}
			if (n + 1 < pfs->nrSorts()) {
				_typeStream << " * ";
			}
		}
		if (pfs->nrSorts() > 1)
			_typeStream << ")";
		_typeStream << " > $o)).\n";
	}

	void visit(const Function* f) {
		if (!f->overloaded()) {
			if (_arithmetic) {
				outputTFFPFSymbolType(f);
				_typeAxiomStream << "tff";
			} else {
				_typeAxiomStream << "fof";
			}
			_typeAxiomStream << "(";
			if(_conjecture) {
				_typeAxiomStream << "cta";
			} else {
				_typeAxiomStream << "ta";
			}
			_typeAxiomStream << _count;
			_typeAxiomStream << ",axiom,(";
			_typeAxiomStream << "! [";
			_typeAxiomStream << "V0";
			if(_arithmetic) {
				_typeAxiomStream << ": ";
				outputTFFType(f->sort(0), &_typeAxiomStream);
			}
			for(unsigned int n = 1; n < f->arity() + 1; ++n) {
				_typeAxiomStream << ",V" << n;
				if (_arithmetic) {
					_typeAxiomStream << ": ";
					outputTFFType(f->sort(n), &_typeAxiomStream);
				}
			}
			_typeAxiomStream << "] : (";
			// TODO: Waar ga je het onderscheid nog zien?
			// You won't, but names may not clash, so you can call these p_ too
			_typeAxiomStream << "~p_" << rewriteLongname(f->to_string(true)) << "(";
			_typeAxiomStream << "V0";
			for(unsigned int n = 1; n < f->arity() + 1; ++n) {
				_typeAxiomStream << ",V" << n;
			}
			_typeAxiomStream << ") | (";
			_typeAxiomStream << "t_" << f->sort(0)->name() << "(V0)";
			for(unsigned int n = 1; n < f->arity() + 1; ++n) {
				_typeAxiomStream << " & " << "t_" << f->sort(n)->name() << "(V" << n << ")";
			}
			_typeAxiomStream << ")))).\n";
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
		else {
			std::cout << "Function: " << f->to_string(true) << std::endl;
		}
	}
	
	void outputTotalFuncAxiom(const Function* f) {
		if(_arithmetic) {
			_typeAxiomStream << "tff";
		} else {
			_typeAxiomStream << "fof";
		}
		_typeAxiomStream << "(tfa" << _count << ",axiom,(";
		_typeAxiomStream << "! [";
		_typeAxiomStream << "V0";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->sort(0), &_typeAxiomStream);
		}
		for(unsigned int n = 1; n < f->arity(); ++n) {
			_typeAxiomStream << ",V" << n;
			if (_arithmetic) {
				_typeAxiomStream << ": ";
				outputTFFType(f->sort(n), &_typeAxiomStream);
			}
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "~(";
		_typeAxiomStream << "t_" << f->sort(0)->name() << "(V0)";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			_typeAxiomStream << " & " << "t_" << f->sort(n)->name() << "(V" << n << ")";
		}
		_typeAxiomStream << ") | ";
		_typeAxiomStream << "(? [X1";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->outsort(), &_typeAxiomStream);
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "t_" << f->outsort()->name() << "(X1) & ";
		_typeAxiomStream << "(! [X2";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->outsort(), &_typeAxiomStream);
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "p_" << rewriteLongname(f->to_string(true)) << "(";
		_typeAxiomStream << "V0";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			_typeAxiomStream << ",V" << n;
		}
		_typeAxiomStream << ",X2) <=> X1 = X2";
		_typeAxiomStream << "))))))).\n";
	}
	
	void outputPartialFuncAxiom(const Function* f) {
		if(_arithmetic) {
			_typeAxiomStream << "tff";
		} else {
			_typeAxiomStream << "fof";
		}
		_typeAxiomStream << "(fa" << _count << ",axiom,(";
		_typeAxiomStream << "! [";
		_typeAxiomStream << "V0";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->sort(0), &_typeAxiomStream);
		}
		for(unsigned int n = 1; n < f->arity(); ++n) {
			_typeAxiomStream << ",V" << n;
			if (_arithmetic) {
				_typeAxiomStream << ": ";
				outputTFFType(f->sort(n), &_typeAxiomStream);
			}
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "~(";
		_typeAxiomStream << "t_" << f->sort(0)->name() << "(V0)";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			_typeAxiomStream << " & " << "t_" << f->sort(n)->name() << "(V" << n << ")";
		}
		_typeAxiomStream << ") | ";
		_typeAxiomStream << "(? [X1";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->outsort(), &_typeAxiomStream);
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "t_" << f->outsort()->name() << "(X1) & ";
		_typeAxiomStream << "(! [X2";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->outsort(), &_typeAxiomStream);
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "~p_" << rewriteLongname(f->to_string(true)) << "(";
		_typeAxiomStream << "V0";
		for(unsigned int n = 1; n < f->arity(); ++n) {
			_typeAxiomStream << ",V" << n;
		}
		_typeAxiomStream << ",X2) | X1 = X2";
		_typeAxiomStream << "))))))).\n";
	}
	
	void outputArity0FuncAxiom(const Function* f) {
		if(_arithmetic) {
			_typeAxiomStream << "tff";
		} else {
			_typeAxiomStream << "fof";
		}
		_typeAxiomStream << "(ta0fa" << _count << ",axiom,";
		_typeAxiomStream << "(? [X1";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->outsort(), &_typeAxiomStream);
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "t_" << f->outsort()->name() << "(X1) & ";
		_typeAxiomStream << "(! [X2";
		if(_arithmetic) {
			_typeAxiomStream << ": ";
			outputTFFType(f->outsort(), &_typeAxiomStream);
		}
		_typeAxiomStream << "] : (";
		_typeAxiomStream << "p_" << rewriteLongname(f->to_string(true)) << "(X2)";
		_typeAxiomStream << " <=> X1 = X2";
		_typeAxiomStream << "))))).\n";
	}

	void visit(SortTable* table) {
	}

	void visit(const GroundSet* s) {
	}
};

#endif /* TPTPPRINTER_HPP_ */
