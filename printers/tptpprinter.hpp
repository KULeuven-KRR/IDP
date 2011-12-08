#ifndef TPTPPRINTER_HPP_
#define TPTPPRINTER_HPP_

#include "printers/print.hpp"
#include "theory.hpp"
#include "vocabulary.hpp"
#include "namespace.hpp"

#include <iostream>

#include "groundtheories/GroundPolicy.hpp"
#include "visitors/VisitorFriends.hpp"

template<typename Stream>
class TPTPPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	bool						_conjecture;
	bool						_arithmetic;
	bool						_nats;
	bool						_ints;
	bool						_floats;
	unsigned int				_count;
	std::set<DomainTerm*>		_typedDomainTerms;
	std::set<DomainElement*>	_typedDomainElements;
	std::set<Sort*>				_types;
	std::stringstream*			_os;
	std::stringstream			_typeStream; // The types. (for TFF)
	std::stringstream			_typeAxiomStream; // The type predicates (defining what atomic symbols have what type,
												  // and which functions/predicates take which types)
	std::stringstream			_axiomStream; // The first theory as axioms
	std::stringstream			_conjectureStream; // The second theory as conjectures

	using StreamPrinter<Stream>::output;

public:
	TPTPPrinter(bool arithmetic, Stream& stream):
			StreamPrinter<Stream>(stream),
			_conjecture(false),
			_arithmetic(arithmetic),
			_nats(false),
			_ints(false),
			_floats(false),
			_count(0){ }
	
	bool conjecture() {
		return _conjecture;
	}

	void conjecture(bool conjecture) {
		_conjecture = conjecture;
	}

	void startTheory() { }
	void endTheory() { }

protected:
	void visit(const AbstractStructure*) {
		Assert(false);
	}

	void visit(const Vocabulary* v) {
		for(auto it = v->firstSort(); it != v->lastSort(); ++it) {
			for(auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
				if(not (*jt)->builtin() || v == Vocabulary::std()) { visit(*jt); }
			}
		}
		for(auto it = v->firstPred(); it != v->lastPred(); ++it) {
			if(not it->second->builtin() || v == Vocabulary::std()) { visit(it->second); }
		}
		for(auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
			if(not it->second->builtin() || v == Vocabulary::std()) { visit(it->second); }
		}
	}

	void visit(const Theory* t) {
		if(!_conjecture) {
			for(auto it = t->sentences().cbegin(); it != t->sentences().cend(); ++it) {
				startAxiom("a");
				(*it)->accept(this);
				endAxiom();
				_count ++;
			}
		} else if (t->sentences().cbegin() != t->sentences().cend()) {
			// Output a conjecture as a conjunction.
			startAxiom("cnj");
			auto it = t->sentences().cbegin();
			while(it != t->sentences().cend()) {
				_conjectureStream << "(";
				(*it)->accept(this);
				_conjectureStream << ")";
				++ it;
				if(it != t->sentences().cend()) {
					_conjectureStream << " & ";
				}
			}
			endAxiom();
		}
		// Do nothing with definitions or fixpoint definitions
		if (_conjecture) {
			outputDomainTermTypeAxioms();
			writeStreams();
		}
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		if(isNeg(f->sign())){
			(*_os) << "~";
		}
		if(toString(f->symbol()) == "=") {
			(*_os) << "(";
			f->subterms()[0]->accept(this);
			(*_os) << " = ";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == ">") {
			(*_os) << "$greater(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "<") {
			(*_os) << "$less(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "+") {
			(*_os) << "($sum(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ") = ";
			f->subterms()[2]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "-" && f->subterms().size() == 3) {
			(*_os) << "($difference(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ") = ";
			f->subterms()[2]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "-" && f->subterms().size() == 2) {
			(*_os) << "($uminus(";
			f->subterms()[0]->accept(this);
			(*_os) << ") = ";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "*") {
			(*_os) << "($product(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ") = ";
			f->subterms()[2]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "PRED") {
			(*_os) << "($difference(";
			f->subterms()[0]->accept(this);
			(*_os) << ",1) = ";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "SUCC") {
			(*_os) << "($sum(";
			f->subterms()[0]->accept(this);
			(*_os) << ",1) = ";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "MAX") {
			// NOTE: $itett is often unsupported
			(*_os) << "($itett($greater(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << "),";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ") = ";
			f->subterms()[2]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "MIN") {
			// NOTE: $itett is often unsupported
			(*_os) << "($itett($less(";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << "),";
			f->subterms()[0]->accept(this);
			(*_os) << ",";
			f->subterms()[1]->accept(this);
			(*_os) << ") = ";
			f->subterms()[2]->accept(this);
			(*_os) << ")";
		} else if(toString(f->symbol()) == "abs") {
			// NOTE: $itett is often unsupported
			(*_os) << "($itett($greater(";
			f->subterms()[0]->accept(this);
			(*_os) << ",$uminus(";
			f->subterms()[0]->accept(this);
			(*_os) << ")),";
			f->subterms()[0]->accept(this);
			(*_os) << ",$uminus(";
			f->subterms()[0]->accept(this);
			(*_os) << ")) = ";
			f->subterms()[1]->accept(this);
			(*_os) << ")";
		} else {
			(*_os) << "p_" << rewriteLongname(toString(f->symbol()));
			if(!f->subterms().empty()) {
				(*_os) << "(";
				f->subterms()[0]->accept(this);
				for(unsigned int n = 1; n < f->subterms().size(); ++n) {
					(*_os) << ",";
					f->subterms()[n]->accept(this);
				}
				(*_os) << ")";
			}
		}
	}

	void visit(const EqChainForm* f) {
		if(isNeg(f->sign())){
			(*_os) << "~";
		}
		(*_os) << "(";
		f->subterms()[0]->accept(this);
		for(unsigned int n = 0; n < f->comps().size(); ++n) {
			if(f->comps()[n] == CompType::EQ)
				(*_os) << " = ";
			else if(f->comps()[n] == CompType::NEQ)
				(*_os) << " != ";
			f->subterms()[n+1]->accept(this);
			if(n+1 < f->comps().size()) {
				if(f->conj())
					(*_os) << " & ";
				else
					(*_os) << " | ";
				f->subterms()[n+1]->accept(this);
			}
		}
		(*_os) << ")";
	}

	void visit(const EquivForm* f) {
		if(isNeg(f->sign())){
			(*_os) << "~";
		}
		(*_os) << "(";
		f->left()->accept(this);
		(*_os) << " <=> ";
		f->right()->accept(this);
		(*_os) << ")";
	}

	void visit(const BoolForm* f) {
		if(f->subformulas().empty()) {
			if(f->isConjWithSign()){
				(*_os) << "$true";
			}else{
				(*_os) << "$false";
			}
		}
		else {
			if(isNeg(f->sign())){
				(*_os) << "~";
			}
			(*_os) << "(";
			f->subformulas()[0]->accept(this);
			for(unsigned int n = 1; n < f->subformulas().size(); ++n) {
				if(f->conj())
					(*_os) << " & ";
				else
					(*_os) << " | ";
				f->subformulas()[n]->accept(this);
			}
			(*_os) << ")";
		}
	}

	void visit(const QuantForm* f) {
		if(isNeg(f->sign())){
			(*_os) << "~";
		}
		(*_os) << "(";
		if(f->isUniv())
			(*_os) << "! [";
		else
			(*_os) << "? [";
		auto it = f->quantVars().cbegin();
		(*_os) << "V_" << (*it)->name();
		if(_arithmetic && (*it)->sort()) {
			(*_os) << ": ";
			(*_os) << TFFTypeString((*it)->sort());
		}
		++ it;
		for(; it != f->quantVars().cend(); ++it) {
			(*_os) << ",";
			(*_os) << "V_" << (*it)->name();
			if(_arithmetic && (*it)->sort()) {
				(*_os) << ": ";
				(*_os) << TFFTypeString((*it)->sort());
			}
		}
		(*_os) << "] : (";
		
		// When quantifying over types, add these.
		it = f->quantVars().cbegin();
		while(it != f->quantVars().cend() && !(*it)->sort())
			++ it;
		if (it != f->quantVars().cend()) {
		 	if(f->isUniv()){
				(*_os) << "~";
		 	}
			(*_os) << "(";
			if(_types.find((*it)->sort()) == _types.cend()) {
				_types.insert((*it)->sort());
			}
			if(SortUtils::isSubsort((*it)->sort(),VocabularyUtils::natsort())) {
				_nats = true;
			}
			else if(SortUtils::isSubsort((*it)->sort(),VocabularyUtils::intsort())) {
				_ints = true;
			}
			else if(SortUtils::isSubsort((*it)->sort(),VocabularyUtils::floatsort())) {
				_floats = true;
			}
			(*_os) << "t_" << (*it)->sort()->name() << "(V_" << (*it)->name() << ")";
			++ it;
			for(; it != f->quantVars().cend(); ++it) {
				if((*it)->sort()) {
					(*_os) << " & ";
					(*_os) << "t_" << (*it)->sort()->name() << "(V_" << (*it)->name() << ")";
				}
			}
			(*_os) << ")";
			if(f->isUniv()){
				(*_os) << " | ";
			}else{
				(*_os) << " & ";
			}
		}
		
		(*_os) << "(";
		f->subformulas()[0]->accept(this);
		(*_os) << ")))";
	}

	/** Terms **/

	void visit(const VarTerm* t) {
		(*_os) << "V_" << t->var()->name();
	}

	void visit(const FuncTerm*) {
		// Functions have been replaced by predicates
		Assert(false);
	}

	void visit(const DomainTerm* t) {
		if(t->sort() && _typedDomainElements.find(const_cast<DomainElement*>(t->value())) == _typedDomainElements.cend()) {
			_typedDomainElements.insert(const_cast<DomainElement*>(t->value()));
			_typedDomainTerms.insert(const_cast<DomainTerm*>(t));
			if(SortUtils::isSubsort(t->sort(),VocabularyUtils::natsort())) {
				_nats = true;
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::intsort())) {
				_ints = true;
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::floatsort())) {
				_floats = true;
			}
		}
		if(t->sort() && _types.find(t->sort()) == _types.cend()) {
			_types.insert(t->sort());
		}
		(*_os) << domainTermNameString(t);
	}

	void visit(const Sort* s) {
		if (!(s->parents().empty())) {
			startAxiom("ta", &_typeAxiomStream);
			(*_os) << "! [X";
			if (_arithmetic) {
				(*_os) << ": ";
				(*_os) << TFFTypeString(s);
			}
			(*_os) << "] : (~" << "t_" << s->name() << "(X) | ";
			auto it = s->parents().cbegin();
			(*_os) << "(" << "t_" << (*it)->name() << "(X)";
			++it;
			for(; it != s->parents().cend(); ++it) {
				(*_os) << " & " << "t_" << (*it)->name() << "(X)";
			}
			(*_os) << "))";
			endAxiom();
			_count ++;
		}
	}

	void visit(const Predicate* p) {
		outputPFSymbolType(p);
		if (!p->overloaded() && p->arity() > 0) {
			_count ++;
		}
	}
	
	void visit(const Function* f) {
		outputPFSymbolType(f);
		if (!f->overloaded()) {
			outputFuncAxiom(f);
			_count ++;
		}
	}

	// Unimplemented virtual methods
	void visit(const GroundSet*) { }
	void visit(const Namespace*) { }
	void visit(const GroundFixpDef*) { }
	void visit(const GroundClause&) { }
	void visit(const AggGroundRule*) { }
	void visit(const GroundAggregate*) { }
	void visit(const CPReification*) { }
	void visit(const PCGroundRule*) { }

private:
	void startAxiom(std::string prefix) {
		if (_conjecture)
			startAxiom(prefix, "conjecture", &_conjectureStream);
		else
			startAxiom(prefix, "axiom", &_axiomStream);
	}
	
	void startAxiom(std::string prefix, std::stringstream* output) {
		startAxiom(prefix, "axiom", output);
	}
	
	void startAxiom(std::string prefix, std::string type, std::stringstream* output) {
		_os = output;
		if(_arithmetic)
			(*_os) << "tff";
		else
			(*_os) << "fof";
		(*_os) << "(" << prefix << _count << "," << type << ",(";
	}
	
	void endAxiom() {
		(*_os) << ")).\n";
	}
	
	void outputPFSymbolType(const PFSymbol* pfs) {
		if (!pfs->overloaded() && pfs->nrSorts() > 0) {
			startAxiom("ta", &_typeAxiomStream);
			(*_os) << "! [";
			(*_os) << "V0";
			if(_arithmetic) {
				(*_os) << ": ";
				(*_os) << TFFTypeString(pfs->sort(0));
			}
			for(unsigned int n = 1; n < pfs->nrSorts(); ++n) {
				(*_os) << ",V" << n;
				if (_arithmetic) {
					(*_os) << ": ";
					(*_os) << TFFTypeString(pfs->sort(n));
				}
			}
			(*_os) << "] : (";
			if(pfs->nrSorts() != 1 || toString(pfs) != pfs->sort(0)->name())
				(*_os) << "~";
			(*_os) << "p_" << rewriteLongname(toString(pfs)) << "("; //TODO: because of the rewrite, the longname options cant be given here!
			(*_os) << "V0";
			for(unsigned int n = 1; n < pfs->nrSorts(); ++n) {
				(*_os) << ",V" << n;
			}
			if(pfs->nrSorts() == 1 && toString(pfs) == pfs->sort(0)->name())
				(*_os) << ") <=> (";
			else
				(*_os) << ") | (";
			(*_os) << "t_" << pfs->sort(0)->name() << "(V0)";
			for(unsigned int n = 1; n < pfs->nrSorts(); ++n) {
				(*_os) << " & " << "t_" << pfs->sort(n)->name() << "(V" << n << ")";
			}
			(*_os) << "))";
			if (_arithmetic) {
				outputTFFPFSymbolType(pfs);
			}
			endAxiom();
		}
	}
	
	void outputTFFPFSymbolType(const PFSymbol* pfs) {
		//_typeStream << "tff(t" << _count;
		//_typeStream << ",type,(";
		startAxiom("t", "type", &_typeStream);
		(*_os) << "p_" << rewriteLongname(toString(pfs));
		(*_os) << ": ";
		if (pfs->nrSorts() > 1) {
			(*_os) << "(";
		}
		for(unsigned int n = 0; n < pfs->nrSorts(); ++ n) {
			if(pfs->sort(n)) {
				(*_os) << TFFTypeString(pfs->sort(n));
			}
			else {
				(*_os) << "$i";
			}
			if (n + 1 < pfs->nrSorts()) {
				(*_os) << " * ";
			}
		}
		if (pfs->nrSorts() > 1)
			(*_os) << ")";
		(*_os) << " > $o";
		//_typeStream << ")).\n";
		endAxiom();
	}
	
	void outputDomainTermTypeAxioms() {
		std::vector<DomainTerm*> strings;
		for(auto it = _typedDomainTerms.cbegin(); it != _typedDomainTerms.cend(); ++ it) {
			startAxiom("dtta", &_typeAxiomStream);
			std::string sortName = (*it)->sort()->name();
			(*_os) << "t_" << sortName << "(";
			(*_os) << domainTermNameString(*it);
			(*_os) << ")";
			endAxiom();
			if(SortUtils::isSubsort((*it)->sort(),VocabularyUtils::stringsort())) {
				strings.push_back((*it));
			}
			_count ++;
			// if(_arithmetic && sortName != "int" && sortName != "nat" && sortName != "float" && sortName != "string") {
			// 	_typeStream << "tff(dtt" << _count << ",type,(";
			// 	_typeStream << domainTermNameString(*it) << ": ";
			// 	_typeStream << "$tType";
			// 	_typeStream << ")).\n";
			// }
			// _count ++;
		}
		if(_arithmetic) {
			for(auto it = _types.cbegin(); it != _types.cend(); ++ it) {
 				_typeStream << "tff(";
 				_typeStream << "dtt";
 				_typeStream << _count << ",type,(";
 				_typeStream << "t_" << (*it)->name() << ": ";
 				_typeStream << TFFTypeString(*it);
 				_typeStream << " > $o";
 				_typeStream << ")).\n";
				_count ++;
	 		}
		}
		if (strings.size() >= 2) {
			startAxiom("stringineqa", &_typeAxiomStream);
			for(unsigned int i = 0; i < strings.size() - 1; i++) {
				for(unsigned int j = i + 1; j < strings.size(); j++) {
					(*_os) << domainTermNameString(strings[i]) << " != " << domainTermNameString(strings[j]);
					if(!(i == strings.size() - 2 && j == strings.size() - 1)) {
						(*_os) << " & ";
					}
				}
			}
			endAxiom();
		}
		_typedDomainElements.clear();
		_typedDomainTerms.clear();
		_types.clear();
	}
	
	//FIXME: everywhere this method was called, the longname option was given.  However, this option disappeared!
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
			while(result[pos + 3] == '_')
				++ pos;
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

	std::string TFFTypeString(const Sort* s) {
		if(SortUtils::isSubsort(const_cast<Sort*>(s),VocabularyUtils::natsort())) {
			return "$int";
		}
		else if(SortUtils::isSubsort(const_cast<Sort*>(s),VocabularyUtils::intsort())) {
			return "$int";
		}
		else if(SortUtils::isSubsort(const_cast<Sort*>(s),VocabularyUtils::floatsort())) {
			return "$float";
		}
		else {
			return "$i";
		}
	}

	std::string domainTermNameString(const DomainTerm* t) {
		std::string str = toString(t->value());
		if(t->sort()) {
			if(SortUtils::isSubsort(t->sort(),VocabularyUtils::stringsort())) {
				std::stringstream result;
				result << "str_" << t->value()->value()._string;
				return result.str();
			}
			else if(SortUtils::isSubsort(t->sort(),VocabularyUtils::floatsort())) {
				return str;
			}
			else {
				return "tt_" + str;
			}
		}
		else return "utt_" + str;
	}

	void outputFuncAxiom(const Function* f) {
		startAxiom("fa", &_typeAxiomStream);
		if(f->arity() > 0) {
			(*_os) << "! [";
			(*_os) << "V0";
			if(_arithmetic) {
				(*_os) << ": ";
				(*_os) << TFFTypeString(f->sort(0));
			}
			for(unsigned int n = 1; n < f->arity(); ++n) {
				(*_os) << ",V" << n;
				if (_arithmetic) {
					(*_os) << ": ";
					(*_os) << TFFTypeString(f->sort(n));
				}
			}
			(*_os) << "] : (";
			(*_os) << "~(";
			(*_os) << "t_" << f->sort(0)->name() << "(V0)";
			for(unsigned int n = 1; n < f->arity(); ++n) {
				(*_os) << " & " << "t_" << f->sort(n)->name() << "(V" << n << ")";
			}
			(*_os) << ") | ";
		}
		(*_os) << "(? [X1";
		if(_arithmetic) {
			(*_os) << ": ";
			(*_os) << TFFTypeString(f->outsort());
		}
		(*_os) << "] : (";
		(*_os) << "t_" << f->outsort()->name() << "(X1) & ";
		(*_os) << "(! [X2";
		if(_arithmetic) {
			(*_os) << ": ";
			(*_os) << TFFTypeString(f->outsort());
		}
		(*_os) << "] : (";
		if(f->partial())
			(*_os) << "~";
		(*_os) << "p_" << rewriteLongname(toString(f)) << "(";
		if(f->arity() > 0) {
			(*_os) << "V0";
			for(unsigned int n = 1; n < f->arity(); ++n) {
				(*_os) << ",V" << n;
			}
			(*_os) << ",";
		}
		(*_os) << "X2)";
		if(f->partial())
			(*_os) << " | ";
		else
			(*_os) << " <=> ";
		(*_os) << "X1 = X2";
		(*_os) << "))))";
		if(f->arity() > 0)
			(*_os) << ")";
		endAxiom();
	}

	void writeStreams() {
		output() << _typeStream.str();
		// Add t_nat > t_int > t_float hierarchy
		output() << _typeAxiomStream.str();
		if (_arithmetic) {
			if (_nats && _ints)
				output() << "tff(nat_is_int,axiom,(! [X: $int] : (~t_nat(X) | t_int(X)))).\n";
			if (_ints && _floats)
				output() << "tff(int_is_float,axiom,(! [X: $int] : (~t_int(X) | t_float(X)))).\n";
			if (_nats && _floats)
				output() << "tff(nat_is_float,axiom,(! [X: $int] : (~t_nat(X) | t_float(X)))).\n";
		}
		startAxiom("char_is_string", &_axiomStream);
		(*_os) << "! [X] : (~t_char(X) | t_string(X))";
		endAxiom();
		output() << _axiomStream.str();
		output() << _conjectureStream.str();
		_typeStream.str(std::string());
		_typeAxiomStream.str(std::string());
		_axiomStream.str(std::string());
		_conjectureStream.str(std::string());
	}
};

#endif /* TPTPPRINTER_HPP_ */
