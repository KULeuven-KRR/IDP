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

// FIXME rewrite the printers to correctly handle visiting incrementally, making sure all arguments are instantiated, ...

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
	using StreamPrinter<Stream>::isDefClosed;
	using StreamPrinter<Stream>::isDefOpen;
	using StreamPrinter<Stream>::closeDef;
	using StreamPrinter<Stream>::openDef;
	using StreamPrinter<Stream>::isTheoryOpen;
	using StreamPrinter<Stream>::closeTheory;
	using StreamPrinter<Stream>::openTheory;

public:
	EcnfPrinter(bool writetranslation, Stream& stream):
			StreamPrinter<Stream>(stream),
			_currenthead(-1),
			_currentdefnr(0),
			_structure(NULL),
			_termtranslator(NULL),
			writeTranslation_(writetranslation){

	}

	virtual void setStructure(AbstractStructure* t){ _structure = t; }
	virtual void setTermTranslator(GroundTermTranslator* t){ _termtranslator = t; }

	void visit(const Vocabulary* ) {
		output() <<"(vocabulary cannot be printed in ecnf)";
	}

	void visit(const Namespace* ) {
		output() << "(namespace cannot be printed in ecnf)";
	}

	void visit(const AbstractStructure* ) {
		output() <<"(structure cannot be printed in ecnf)";
	}

	void startTheory(){
		if(!isTheoryOpen()){
			output() << "p ecnf\n";
			openTheory();
		}
	}

	void endTheory(){
		if(isTheoryOpen()){
			closeTheory();
		}
	}

	void visit(const GroundClause& g){
		startTheory();
		for(unsigned int m = 0; m < g.size(); ++m){
			output() << g[m] << ' ';
		}
		output() << '0' << "\n";
	}

	void visit(const GroundTheory* g) {
		setStructure(g->structure());
		setTermTranslator(g->termtranslator());
		startTheory();
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
		endTheory();
	}

	void visit(const GroundFixpDef*) {
		/*TODO not implemented*/
		output() <<"c Warning, fixpoint definitions are not printed yet\n";
	}

	void openDefinition(int defid){
		assert(isDefClosed());
		openDef(defid);
	}

	void closeDefinition(){
		assert(!isDefClosed());
		closeDef();
	}

	void visit(const GroundDefinition* d) {
		startTheory();
		_currentdefnr++;
		openDefinition(_currentdefnr);
		for(auto it = d->begin(); it != d->end(); ++it) {
			_currenthead = it->first;
			(it->second)->accept(this);
		}
		closeDefinition();
	}

	void visit(int defid, int tseitin, const PCGroundRuleBody* b) {
		startTheory();
		assert(isDefOpen(defid));
		output() << (b->type() == RT_CONJ ? "C " : "D ");
		output() << "<- " << defid << ' ' << tseitin << ' ';
		for(unsigned int n = 0; n < b->size(); ++n){
			output() << b->literal(n) << ' ';
		}
		output() << "0\n";
	}

	void visit(const PCGroundRuleBody* b) {
		startTheory();
		visit(_currentdefnr, _currenthead, b);
	}

	void visit(const GroundAggregate* b) {
		startTheory();
		visit(_currentdefnr, b);
	}

	void visit(int defnr, const GroundAggregate* a) {
		startTheory();
		assert(a->arrow() != TS_RULE);
		printAggregate(a->type(),a->arrow(),defnr,a->lower(),a->head(),a->setnr(),a->bound());
	}

	void visit(const GroundSet* s) {
		startTheory();
		output() << (s->weighted() ? "WSet" : "Set") << ' ' << s->setnr();
		for(unsigned int n = 0; n < s->size(); ++n) {
			output() << ' ' << s->literal(n);
			if(s->weighted()) output() << '=' << s->weight(n);
		}
		output() << " 0\n";
	}

	void addWeightedSum(int head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, CompType rel){
		assert(varids.size()==weights.size());
		for(auto i=varids.begin(); i<varids.end(); ++i){
			printCPVariable(*i);
		}
		printCPReification("SUMSTSIRI",head,varids,weights,rel,bound);
	}

	void visit(const CPReification* cpr) {
		startTheory();
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
		} else if(typeid(*left) == typeid(CPSumTerm)) {
			CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
			std::vector<int> weights;
			weights.resize(term->_varids.size(), 1);

			if(right._isvarid) {
				std::vector<VarId> varids = term->_varids;
				int bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);

				addWeightedSum(cpr->_head, varids, weights, bound, comp);
			} else {
				addWeightedSum(cpr->_head, term->_varids, weights, right._bound, comp);
			}
		} else {
			assert(typeid(*left) == typeid(CPWSumTerm));
			CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
			if(right._isvarid) {
				std::vector<VarId> varids = term->_varids;
				std::vector<int> weights = term->_weights;

				int bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);

				addWeightedSum(cpr->_head, varids, weights, bound, comp);
			} else {
				addWeightedSum(cpr->_head, term->_varids, term->_weights, right._bound, comp);
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
				assert(isDefOpen(defnr));
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

	std::string toString(CompType comp){
		switch (comp) {
			case CT_EQ: return "=";
			case CT_NEQ: return "~=";
			case CT_LEQ: return "=<";
			case CT_GEQ: return ">=";
			case CT_GT: return ">";
			case CT_LT: return "<";
		}
		assert(false);
		return "";
	}

	void printCPReification(std::string type, int head, unsigned int left, CompType comp, long right) {
		output() << type << ' ' << head << ' ' << left << ' ' << toString(comp) << ' ' << right << " 0" << "\n";
>>>>>>> 7178c49c1db4c5f60805d86cc866296d7970e099
	}

	void printCPReification(std::string type, int head, std::vector<unsigned int> varids, std::vector<int> weights, CompType comp, long right) {
		output() << type << ' ' << head << ' ';
		for(std::vector<unsigned int>::const_iterator it = varids.begin(); it != varids.end(); ++it){
			output() << *it << ' ';
		}
		output() << " | ";
		for(std::vector<int>::const_iterator it = weights.begin(); it != weights.end(); ++it){
			output() << *it << ' ';
		}
		output() << toString(comp) << ' ' << right << " 0" << "\n";
	}
};

#endif /* ECNFPRINTER_HPP_ */
