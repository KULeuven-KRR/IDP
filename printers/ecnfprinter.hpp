/************************************
	ecnfprinter.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ECNFPRINTER_HPP_
#define ECNFPRINTER_HPP_

#include "printers/print.hpp"
#include "theory.hpp"
#include "ground.hpp"
#include "ecnf.hpp"

template<typename Stream>
class EcnfPrinter : public StreamPrinter<Stream> {
private:
	int							_currenthead;
	unsigned int 				_currentdefnr;
	AbstractStructure*			_structure;
	const GroundTermTranslator*	_termtranslator;
	std::set<unsigned int> 		_printedvarids;
	bool 						writeTranslation_;

	bool writeTranlation() const { return writeTranslation_; }

	using StreamPrinter<Stream>::output;
	using StreamPrinter<Stream>::printtab;
	using StreamPrinter<Stream>::unindent;
	using StreamPrinter<Stream>::indent;
	using StreamPrinter<Stream>::isClosed;
	using StreamPrinter<Stream>::isOpen;
	using StreamPrinter<Stream>::setOpen;

public:
	EcnfPrinter(bool writetranslation, Stream& stream):
			StreamPrinter<Stream>(stream),
			writeTranslation_(writetranslation){}

	void visit(const Vocabulary* ) {
		output() <<"(vocabulary cannot be printed in ecnf)";
	}

	void visit(const Namespace* ) {
		output() << "(namespace cannot be printed in ecnf)";
	}

	void visit(const AbstractStructure* ) {
		output() <<"(structure cannot be printed in ecnf)";
	}

	void visit(const GroundClause& g){
		for(unsigned int m = 0; m < g.size(); ++m){
			output() << g[m] << ' ';
		}
		output() << '0' << "\n";
	}

	void visit(const GroundTheory* g) {
		_structure = g->structure();
		_termtranslator = g->termtranslator();
		output() << "p ecnf\n";
		for(unsigned int n = 0; n < g->nrClauses(); ++n) {
			visit(g->clause(n));
		}
		_currentdefnr = 1; //NOTE: 0 is not accepted as defition identifier by ECNF
		for(unsigned int n = 0; n < g->nrDefinitions(); ++n) {
			_currentdefnr++;
			g->definition(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrSets(); ++n){ //IMPORTANT: Print sets before aggregates!!
			g->set(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrAggregates(); ++n){
			g->aggregate(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrFixpDefs(); ++n){
			g->fixpdef(n)->accept(this);
		}
		for(unsigned int n = 0; n < g->nrCPReifications(); ++n) {
			g->cpreification(n)->accept(this);
		}

		if(writeTranlation()){
			output() <<"=== Atomtranslation ===" << "\n";
			GroundTranslator* translator = g->translator();
			int atom = 1;
			while(translator->isSymbol(atom)){
				if(!translator->isTseitin(atom)){
					output() << atom <<"|" <<translator->printAtom(atom) <<"\n";
				}
				atom++;
			}
			output() <<"==== ====" <<"\n";
		}
	}

	void visit(const GroundFixpDef*) {
		/*TODO not implemented*/
		output() <<"c Warning, fixpoint definitions are not printed yet\n";
	}

	void openDefinition(int defid){
		assert(!isClosed());
		setOpen(defid);
		printtab();
		output() << "{\n";
		indent();
	}

	void closeDefinition(){
		assert(!isClosed());
		setOpen(-1);
		unindent();
		output() << "}\n";
	}

	void visit(const GroundDefinition* d) {
		_currentdefnr++;
		for(auto it = d->begin(); it != d->end(); ++it) {
			_currenthead = it->first;
			(it->second)->accept(this);
		}
	}

	void visit(int defid, int tseitin, const PCGroundRuleBody* b) {
		assert(isOpen(defid));
		output() << (b->type() == RT_CONJ ? "C " : "D ");
		output() << "<- " << defid << ' ' << tseitin << ' ';
		for(unsigned int n = 0; n < b->size(); ++n){
			output() << b->literal(n) << ' ';
		}
		output() << "0\n";
	}

	void visit(const PCGroundRuleBody* b) {
		visit(_currentdefnr, _currenthead, b);
	}

	void visit(const GroundAggregate* b) {
		visit(_currentdefnr, b);
	}

	void visit(int defnr, const GroundAggregate* a) {
		assert(a->arrow() != TS_RULE);
		printAggregate(a->type(),a->arrow(),defnr,a->lower(),a->head(),a->setnr(),a->bound());
	}

	void visit(const GroundSet* s) {
		output() << (s->weighted() ? "WSet" : "Set") << ' ' << s->setnr();
		for(unsigned int n = 0; n < s->size(); ++n) {
			output() << ' ' << s->literal(n);
			if(s->weighted()) output() << '=' << s->weight(n);
		}
		output() << " 0\n";
	}

	void visit(const CPReification* cpr) {
		CompType comp = cpr->_body->comp();
		CPTerm* left = cpr->_body->left();
		CPBound right = cpr->_body->right();
		if(typeid(*left) == typeid(CPVarTerm)) {
			CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
			printCPVariable(term->_varid);
			if(right._isvarid) { // CPBinaryRelVar
				printCPVariable(right._varid);
				printCPReification("BINTRT",cpr->_head,term->_varid,comp,right._varid);
			}
			else { // CPBinaryRel
				printCPReification("BINTRI",cpr->_head,term->_varid,comp,right._bound);
			}
		}
		else if(typeid(*left) == typeid(CPSumTerm)) {
			CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
			printCPVariables(term->_varids);
			if(right._isvarid) { // CPSumWithVar
				printCPVariable(right._varid);
				printCPReification("SUMSTRT",cpr->_head,term->_varids,comp,right._varid);
			}
			else { // CPSum
				printCPReification("SUMSTRI",cpr->_head,term->_varids,comp,right._bound);
			}
		}
		else {
			assert(typeid(*left) == typeid(CPWSumTerm));
			CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
			printCPVariables(term->_varids);
			if(right._isvarid) { // CPSumWeightedWithVar
				printCPVariable(right._varid);
				printCPReification("SUMSTSIRT",cpr->_head,term->_varids,term->_weights,comp,right._varid);
			}
			else { // CPSumWeighted
				printCPReification("SUMSTSIRI",cpr->_head,term->_varids,term->_weights,comp,right._bound);
			}
		}
	}

private:
	void printAggregate(AggFunction aggtype, TsType arrow, unsigned int defnr, bool lower, int head, unsigned int setnr, double bound) {
		switch(aggtype) {
			case AGG_CARD: 	output() << "Card "; break;
			case AGG_SUM: 	output() << "Sum "; break;
			case AGG_PROD: 	output() << "Prod "; break;
			case AGG_MIN: 	output() << "Min "; break;
			case AGG_MAX: 	output() << "Max "; break;
		}
		#warning "Replacing implication by equivalence in ecnfprinter, should change in future.";
		switch(arrow) {
			case TS_EQ: case TS_IMPL: case TS_RIMPL:  // (Reverse) implication is not supported by solver (yet)
				output() << "C ";
				break;
			case TS_RULE:
				assert(isOpen(defnr));
				output() << "<- " << defnr << ' ';
				break;
		}
		output() << (lower ? 'G' : 'L') << ' ' << head << ' ' << setnr << ' ' << bound << " 0" <<"\n";
	}
	void printCPVariables(std::vector<unsigned int> varids) {
		for(std::vector<unsigned int>::const_iterator it = varids.begin(); it != varids.end(); ++it) {
			printCPVariable(*it);
		}
	}

	void printCPVariable(unsigned int varid) {
		if(_printedvarids.find(varid) == _printedvarids.end()) {
			_printedvarids.insert(varid);
			SortTable* domain = _termtranslator->domain(varid);
			if(domain->isRange()) {
				int minvalue = domain->first()->value()._int;
				int maxvalue = domain->last()->value()._int;
				output() << "INTVAR " << varid << ' ' << minvalue << ' ' << maxvalue << ' ';
			} else {
				output() << "INTVARDOM " << varid << ' ';
				for(SortIterator it = domain->sortbegin(); it.hasNext(); ++it) {
					int value = (*it)->value()._int;
					output() << value << ' ';
				}
			}
			output() << '0' << "\n";
		}
	}

	void printCPReification(std::string type, int head, unsigned int left, CompType comp, long right) {
		output() << type << ' ' << head << ' ' << left << ' ' << comp << ' ' << right << " 0" << "\n";
	}

	void printCPReification(std::string type, int head, std::vector<unsigned int> left, CompType comp, long right) {
		output() << type << ' ' << head << ' ';
		for(std::vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
			output() << *it << ' ';
		output() << comp << ' ' << right << " 0" << "\n";
	}

	void printCPReification(std::string type, int head, std::vector<unsigned int> left, std::vector<int> weights, CompType comp, long right) {
		output() << type << ' ' << head << ' ';
		for(std::vector<unsigned int>::const_iterator it = left.begin(); it != left.end(); ++it)
			output() << *it << ' ';
		output() << " | ";
		for(std::vector<int>::const_iterator it = weights.begin(); it != weights.end(); ++it)
			output() << *it << ' ';
		output() << comp << ' ' << right << " 0" << "\n";
	}
};

#endif /* ECNFPRINTER_HPP_ */
