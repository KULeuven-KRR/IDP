/************************************
	ecnf.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ecnf.hpp"
#include <iostream>
#include <sstream>

using namespace MinisatID;

/*************************
	Ground definitions
*************************/

void GroundDefinition::addTrueRule(int head) {
	//FIXME: Probably have to be more careful and check (it->second)._type for RT_AGG and act appropriately!!
	PCGroundRuleBody& grb(RT_TRUE,vector<int>(0));
	_rules[head] = grb;
//	GroundRuleBody& grb = _rules[head];
//	grb._type = RT_TRUE;
//	grb._body = vector<int>(0);
}

void GroundDefinition::addRule(int head, const vector<int>& body, bool conj, bool recursive) {
	map<int,GroundRuleBody>::iterator it = _rules.find(head);
	//FIXME: Probably have to be more careful and check (it->second)._type for RT_AGG and act appropriately!!
	if(it == _rules.end() || (it->second)._type == RT_FALSE) {
		PCGroundRuleBody& grb = (it == _rules.end() ? _rules[head] : it->second);
		if(body.empty()) grb._type = (conj ? RT_TRUE : RT_FALSE);
		else if(body.size() == 1) grb._type = RT_UNARY;
		else grb._type = (conj ? RT_CONJ : RT_DISJ);
		grb._body = body;
	}
	else if(body.empty()) {
		if(conj) {
			PCGroundRuleBody& newgrb(RT_TRUE,body);
			_rules[head] = newgrb;
		}
	}
	else {
		switch(it->second._type) {
			case RT_TRUE: break;
			case RT_FALSE: assert(false); break;
			case RT_UNARY:
				PCGroundRuleBody& grb = it->second;
				if(body.size() == 1) {
					grb._type = RT_DISJ;
					grb._body.push_back(body[0]);
				}
				else if(!conj) {
					grb._type = RT_DISJ;
					int temp = grb._body[0];
					grb._body = body;
					grb._body.push_back(temp);
				}
				else {
					int ts = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					grb._type = RT_DISJ;
					grb._body.push_back(ts);
				}
				if(recursive) grb._recursive = true;
				break;
			case RT_DISJ:
				PCGroundRuleBody& grb = it->second;
				if((!conj) || body.size() == 1) {
					for(unsigned int n = 0; n < body.size(); ++n) 
						grb._body.push_back(body[n]);
				}
				else {
					int ts = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					grb._body.push_back(ts);
				}
				if(recursive) grb._recursive = true;
				break;
			case RT_CONJ:
				PCGroundRuleBody& grb = it->second;
				if((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb._body,true,(grb._recursive ? TS_RULE : TS_EQ));
					grb._type = RT_DISJ;
					grb._body = body;
					grb._body.push_back(ts);
				}
				else {
					int ts1 = _translator->translate(grb._body,true,(grb._recursive ? TS_RULE : TS_EQ));
					int ts2 = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					grb._type = RT_DISJ;
					vector<int> vi(2) ; vi[0] = ts1; vi[1] = ts2;
					grb._body = vi;
				}
				if(recursive) grb._recursive = true;
				break;
			case RT_AGG:
				AggGroundRuleBody& grb = it->second;
				if((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb._body,true,(grb._recursive ? TS_RULE : TS_EQ));
					PCGroundRuleBody& pcgrb(RT_DISJ,body);
					pcgrb._body.push_back(ts);
					_rules[head] = pcgrb;
				}
				else {
					int ts1 = _translator->translate(grb._body,true,(grb._recursive ? TS_RULE : TS_EQ));
					int ts2 = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					vector<int> vi(2) ; vi[0] = ts1; vi[1] = ts2;
					PCGroundRuleBody& pcgrb(RT_DISJ,body);
					pcgrb._body = vi;
					_rules[head] = pcgrb;
				}
				if(recursive) grb._recursive = true;
				break;
			default:
				assert(false);
		}
	}
}

void GroundDefinition::addAgg(int head, int setnr, AggType aggtype, bool lower, double bound, bool recursive) {
	map<int,GroundRuleBody>::iterator it = _rules.find(head);
	if(it == _rules.end() || (it->second)._type == RT_FALSE) {
		AggGroundRuleBody& agggrb(setnr,aggtype,lower,bound);
		_rules[head] = agggbr;
	}
	else {
		GroundRuleBody& grb = it->second;
		switch(grb.type) {
			case RT_TRUE: break;
			case RT_FALSE: assert(false); break;
			case RT_UNARY: {
				PCGroundRuleBody& pcgrb = dynamic_cast<PCGroundRuleBody&>(grb);
				int ts = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
				pcgrb._type = RT_DISJ;
				pcgrb._body.push_back(ts);
//				AggGroundRuleBody& agggrb(setnr,aggtype,lower,bound);
//				_rules[ts] = aggrb;
				break;
			}
			case RT_DISJ: {
				PCGroundRuleBody& pcgrb = dynamic_cast<PCGroundRuleBody&>(grb);
				int ts = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
				pcgrb._body.push_back(ts);
//				AggGroundRuleBody& agggrb(setnr,aggtype,lower,bound);
//				_rules[ts] = aggrb;
				break;
			}
			case RT_CONJ: {
				PCGroundRuleBody& pcgrb = dynamic_cast<PCGroundRuleBody&>(grb);
				int ts1 = _translator->translate(pcgrb._body,true,(grb._recursive ? TS_RULE : TS_EQ));
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
//				PCGroundRuleBody& newpcgrb(pcgrb);
//				_rules[ts1] = newpcgrb;
//				AggGroundRuleBody& agggrb(setnr,aggtype,lower,bound);
//				_rules[ts2] = aggrb;
				vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
				pcgrb._type = RT_DISJ;
				pcgrb._body = vi;
				break;
			}
			case RT_AGG: {
				AggGroundRuleBody& agggrb = dynamic_cast<AggGroundRuleBody&>(grb);
				int ts1 = _translator->translate(agggrb._body,true,(grb._recursive ? TS_RULE : TS_EQ));
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
//				_rules[ts1] = agggrb;
//				AggGroundRuleBody& newagggrb(setnr,aggtype,lower,bound);
//				_rules[ts2] = newaggrb;
				vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
				PCGroundRuleBody& newpcgrb(RT_DISJ,vi);
				_rules[head] = newpcgrb;
				break;
			}
		}
	}
}

string GroundDefinition::to_string() const {
	stringstream s;
	s << "{\n";
	for(map<int,GroundRuleBody>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) {
		s << _translator->printatom(it->first) << " <- ";
		const GroundRuleBody& body = it->second;
		if(body._type == RT_TRUE) s << "true. ";
		else if(body._type == RT_FALSE) s << "false. ";
		else if(body._type == RT_AGG) {
			// TODO
			assert(false);
		}
		else {
			char c = body._type == RT_CONJ ? '&' : '|';
			if(!body._body.empty()) {
				if(body._body[0] < 0) s << '~';
				s << _translator->printatom(body._body[0]);
				for(unsigned int n = 1; n < body._body.size(); ++n) {
					s << ' ' << c << ' ';
					if(body._body[n] < 0) s << '~';
					s << _translator->printatom(body._body[n]);
				}
			}
			s << ". ";
		}
		s << "// ";
		if(body._type == RT_TRUE || body._type == RT_CONJ || body._type == RT_UNARY) s << "C ";
		else if(body._type == RT_FALSE || body._type == RT_DISJ) s << "D "; 
		else /* TODO */ assert(false);
		s << it->first << ' ';
		for(unsigned int n = 0; n < body._body.size(); ++n) 
			s << body._body[n] << ' ';
		s << "0\n";
	}
	s << "}\n";
	return s.str();
}

/*******************************
	Abstract Ground theories
*******************************/

const int _nodef = -1;

/*
 * AbstractGroundTheory::transformForAdd(vector<int>& vi, VIType vit, int defnr, bool skipfirst)
 * DESCRIPTION
 *		Adds defining rules for tseitin literals in a given vector of literals to the ground theory.
 *		This method may apply unfolding which changes the given vector of literals.
 * PARAMETERS
 *		vi			- given vector of literals
 *		vit			- indicates whether vi represents a disjunction, conjunction or set of literals
 *		defnr		- number of the definition vi belongs to. Is _nodef when vi does not belong to a definition
 *		skipfirst	- if true, the defining rule for the first literal is not added to the ground theory
 * TODO
 *		implement unfolding
 */
void AbstractGroundTheory::transformForAdd(vector<int>& vi, VIType vit, int defnr, bool skipfirst) {
	unsigned int n = 0;
	if(skipfirst) ++n;
	for(; n < vi.size(); ++n) {
		int atom = abs(vi[n]);
		if(_translator->isTseitin(atom) && _printedtseitins.find(atom) == _printedtseitins.end()) {
			_printedtseitins.insert(atom);
			TsBody* tsbody = _translator->tsbody(atom);
			if(typeid(*tsbody) == typeid(PCTsBody)) {
				PCTsBody& body = dynamic_cast<PCTsBody&>(*tsbody);
				if(body._type == TS_IMPL || body._type == TS_EQ) {
					if(body._conj) {
						for(unsigned int m = 0; m < body._body.size(); ++m) {
							vector<int> cl(2,-atom);
							cl[1] = body._body[m];
							addClause(cl,true);	
						}
					}
					else {
						vector<int> cl(body._body.size()+1,-atom);
						for(unsigned int m = 0; m < body._body.size(); ++m) cl[m+1] = body._body[m];
						addClause(cl,true);
					}
				}
				if(body._type == TS_RIMPL || body._type == TS_EQ) {
					if(body._conj) {
						vector<int> cl(body._body.size()+1,atom);
						for(unsigned int m = 0; m < body._body.size(); ++m) cl[m+1] = -body._body[m];
						addClause(cl,true);
					}
					else {
						for(unsigned int m = 0; m < body._body.size(); ++m) {
							vector<int> cl(2,atom);
							cl[1] = -body._body[m];
							addClause(cl,true);
						}
					}
				}
				if(body._type == TS_RULE) {
					assert(defnr != _nodef);
					addPCRule(defnr,atom,body);
				}
			}
			else {	// body of the tseitin is an aggregate expression
				assert(typeid(tsbody) == typeid(AggTsBody));
				AggTsBody& body = dynamic_cast<AggTsBody&>(*tsbody);
				if(body._type == TS_RULE) {
					assert(defnr != _nodef);
					addAggRule(defnr,atom,body);
				}
				else {
					addAggregate(atom,body);
				}
			}
		}
	}
}


/*****************************
	Internal ground theories
*****************************/

void GroundTheory::addClause(GroundClause& cl, bool skipfirst) {
	transformForAdd(cl,VIT_DISJ,_nodef,skipfirst);
	_clauses.push_back(cl);
}

void GroundTheory::addDefinition(GroundDefinition& d) {
	int defnr = _definitions.size();
	_definitions.push_back(d);
	for(map<int,GroundRuleBody>::iterator it = d._rules.begin(); it != d._rules.end(); ++it) {
		int head = it->first;
		if(_printedtseitins.find(head) == _printedtseitins.end()) {
			GroundRuleBody grb = it->second;
			if(typeid(grb) == typeid(PCGroundRuleBody)) {
				PCGroundRuleBody& pcgrb = dynamic_cast<PCGroundRuleBody&>(grb);
				transformForAdd(pcgrb._body,pcgrb._conj ? VIT_CONJ : VIT_DISJ,defnr);
			}
			else {
				assert(typeid(grb) == typeid(AggGroundRuleBody));
				AggGroundRuleBody& agggrb = dynamic_cast<AggGroundRuleBody&>(grb);
				addSet(agggrb._setnr,defnr,agggrb._aggtype != AGGCARD);
			}
		}
	}
}

void GroundTheory::addFixpDef(GroundFixpDef& d) {
	assert(false);
	/* TODO */
}

void GroundTheory::addAggregate(GroundAggregate& agg) {
	addSet(agg._set,_nodef,agg._type != AGGCARD);
	_aggregates.push_back(agg);
}

void GroundTheory::addAggregate(int head, AggTsBody& body) {
	addSet(body._setnr,_nodef,body._aggtype != AGGCARD);
	GroundAggregate agg(body._aggtype,body._lower,body._type,head,body._setnr,body._bound);
	_aggregates.push_back(agg);
}

void GroundTheory::addSet(int setnr, int defnr, bool) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		const GroundSet& grs = _translator->groundset(setnr);
		transformForAdd(grs._setlits,VIT_SET,defnr);
		_sets.push_back(GroundSet(setnr,grs._setlits,grs._litweights));
	}
}

void GroundTheory::addPCRule(int defnr, int head, PCTsBody& body) {
	_definitions[defnr].addRule(head,body._body,body._conj,true);
}

void GroundTheory::addAggRule(int defnr, int head, AggTsBody& body) {
	_definitions[defnr].addAgg(head,body._setnr,body._aggtype,body._lower,body._bound,true);
}

string GroundTheory::to_string() const {
	stringstream s;
	for(unsigned int n = 0; n < _clauses.size(); ++n) {
		if(_clauses[n].empty()) {
			s << "false";
		}
		else {
			for(unsigned int m = 0; m < _clauses[n].size(); ++m) {
				if(_clauses[n][m] < 0) s << '~';
				s << _translator->printatom(_clauses[n][m]);
				if(m < _clauses[n].size()-1) s << " | ";
			}
		}
		s << ". // ";
		for(unsigned int m = 0; m < _clauses[n].size(); ++m) {
			s << _clauses[n][m] << " ";
		}
		s << "0\n";
	}
	for(unsigned int n = 0; n < _definitions.size(); ++n) {
		s << _definitions[n].to_string();
	}
	for(unsigned int n = 0; n < _sets.size(); ++n) {
		s << "Set nr. " << _sets[n]._setnr << " = { ";
		for(unsigned int m = 0; m < _sets[n]._set.size(); ++m) {
			s << _translator->printatom(_sets[n]._set[m]);
			s << _sets[n]._weights[m];
		}
		s << "}\n";
	}
	for(unsigned int n = 0; n < _aggregates.size(); ++n) {
		const GroundAggregate& agg = _aggregates[n];
		s << _translator->printatom(agg._head);
		switch(agg._eha) {
			case TS_RULE: s << " <- "; break;
			case TS_IMPL: s << " => "; break;
			case TS_RIMPL: s << " <= "; break;
			case TS_EQ: s << " <=> "; break;
			default: assert(false);
		}
		s << agg._bound;
		s << (agg._lower ? " =< " : " >= ");
		switch(agg._type) {
			case AGGCARD: s << "card("; break;
			case AGGSUM: s << "sum("; break;
			case AGGPROD: s << "prod("; break;
			case AGGMIN: s << "min("; break;
			case AGGMAX: s << "max("; break;
			default: assert(false);
		}
		s << agg._set << ")\n";
	}
	//TODO: repeat above for fixpoint definitions
	return s.str();
}

Formula* GroundTheory::sentence(unsigned int) const{
	assert(false); // TODO: not yet implemented
	return 0;
}

Definition* GroundTheory::definition(unsigned int) const {
	assert(false); // TODO: not yet implemented
	return 0;
}

FixpDef* GroundTheory::fixpdef(unsigned int) const {
	assert(false); // TODO: not yet implemented
	return 0;
}

/********************************
	Solver theory definitions
********************************/

void SolverTheory::addClause(GroundClause& cl, bool skipfirst) {
	transformForAdd(cl,VIT_DISJ,_nodef,skipfirst);
	vector<MinisatID::Literal> mcl;
	for(unsigned int n = 0; n < cl.size(); ++n) {
		MinisatID::Literal l(abs(cl[n]),cl[n]<0);
		mcl.push_back(l);
	}
	_solver->addClause(mcl);
}

void SolverTheory::addSet(int setnr, int defnr, bool weighted) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		const GroundSet& grs = _translator->groundset(setnr);
		transformForAdd(grs._setlits,VIT_SET,defnr);
		vector<MinisatID::Literal> lits;
		for(unsigned int n = 0; n < grs._setlits.size(); ++n) {
			MinisatID::Literal l(abs(grs._setlits[n]),grs._setlits[n]<0);
			lits.push_back(l);
		}
		if(!weighted) _solver->addSet(setnr,lits);
		else {
			vector<MinisatID::Weight> weights;
			for(unsigned int n = 0; n < grs._litweights.size(); ++n) {
				MinisatID::Weight w(int(grs._litweights[n]));	// TODO: remove cast if supported by the solver
				weights.push_back(w);
			}
			_solver->addSet(setnr,lits,weights);
		}
	}
}

void SolverTheory::addAggregate(GroundAggregate& agg) {
	addSet(agg._set,_nodef,agg._type != AGGCARD);
	MinisatID::AggSign sg = agg._lower ? AGGSIGN_LB : AGGSIGN_UB;
	MinisatID::AggType tp;
	switch(body._type) {
		case AGGCARD: tp = CARD; break;
		case AGGSUM: tp = SUM; break;
		case AGGPROD: tp = PROD; break;
		case AGGMIN: tp = MIN; break;
		case AGGMAX: tp = MAX; break;
	}
	MinisatID::AggSem sem;
	switch(agg._eha) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL: sem = COMP; break;
		case TS_RULE: sem = DEF; break;
	}
	MinisatID::Literal headlit(agg._head,false);
	MinisatID::Weight weight(int(agg._bound));		// TODO: remove cast if supported by the solver
	_solver->addAggrExpr(headlit,agg._set,weight,sg,tp,sem);
}

void SolverTheory::addAggregate(int head, AggTsBody& body) {
	addSet(body._setnr,_nodef,body._aggtype != AGGCARD);
	MinisatID::AggSign sg = body._lower ? AGGSIGN_LB : AGGSIGN_UB;
	MinisatID::AggType tp;
	switch(body._aggtype) {
		case AGGCARD: tp = CARD; break;
		case AGGSUM: tp = SUM; break;
		case AGGPROD: tp = PROD; break;
		case AGGMIN: tp = MIN; break;
		case AGGMAX: tp = MAX; break;
	}
	MinisatID::AggSem sem;
	switch(body._type) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL: sem = COMP; break;
		case TS_RULE: sem = DEF; break;
	}
	MinisatID::Literal headlit(head,false);
	MinisatID::Weight weight(int(body._bound));		// TODO: remove cast if supported by the solver
	_solver->addAggrExpr(headlit,body._setnr,weight,sg,tp,sem);
}

void SolverTheory::addDefinition(GroundDefinition& d) {
	int defnr = 1; //FIXME: We should ask the solver to give us the next number
	// TODO: include definition ID
	for(map<int,GroundRuleBody>::iterator it = d._rules.begin(); it != d._rules.end(); ++it) {
		GroundRuleBody& grb = it->second;
		if(typeid(grb) == typeid(PCGroundRuleBody)) {
			PCGroundRuleBody& pcgrb = dynamic_cast<PCGroundRuleBody&>(grb);
			transformForAdd(pcgrb._body,(pcgrb._conj ? VIT_CONJ : VIT_DISJ),defnr);
			PCTsBody tsbody(TS_RULE,pcgrb._body,pcgrb._conj);
			addPCRule(defnr,it->first,tsbody);
		}
		else {
			assert(typeid(grb) == typeid(AggGroundRuleBody));
			AggGroundRuleBody& agggrb = dynamic_cast<AggGroundRuleBody&>(grb);
			addSet(agggrb._setnr,defnr,agggrb._aggtype != AGGCARD);
			AggTsBody tsbody(TS_RULE,agggrb._setnr,agggrb._aggtype,agggrb._lower,agggrb._bound);
			addAggRule(defnr,it->first,tsbody);
		}
		// add head to set of defined atoms
		_defined[_translator->symbol(it->first)].insert(it->first);
	}
}

void SolverTheory::addPCRule(int defnr, int head, PCTsBody& tsb) {
	MinisatID::Atom mhead(head);
	vector<MinisatID::Literal> mbody;
	for(unsigned int n = 0; n < tsb._body.size(); ++n) {
		MinisatID::Literal l(abs(tsb._body[n]),tsb._body[n]<0);
		mbody.push_back(l);
	}
	if(tsb._conj)
		_solver->addConjRule(mhead,mbody);
	else
		_solver->addDisjRule(mhead,mbody);
}

void SolverTheory::addAggRule(int defnr, int head, AggTsBody& body) {
	addSet(body._setnr,defnr,body._aggtype != AGGCARD);
	MinisatID::AggSign sg = body._lower ? AGGSIGN_LB : AGGSIGN_UB;
	MinisatID::AggType tp;
	switch(body._aggtype) {
		case AGGCARD: tp = CARD; break;
		case AGGSUM: tp = SUM; break;
		case AGGPROD: tp = PROD; break;
		case AGGMIN: tp = MIN; break;
		case AGGMAX: tp = MAX; break;
	}
	MinisatID::AggSem sem = DEF;
	MinisatID::Literal headlit(head,false);
	MinisatID::Weight weight(int(body._bound));		// TODO: remove cast if supported by the solver
	_solver->addAggrExpr(headlit,body._setnr,weight,sg,tp,sem);
}

class DomelementEquality {
	private:
		unsigned int	_arity;
	public:
		DomelementEquality(unsigned int arity) : _arity(arity) { }
		bool operator()(const vector<domelement>& v1, const vector<domelement>& v2) {
			for(unsigned int n = 0; n < _arity; ++n) {
				if(v1[n] != v2[n]) return false;
			}
			return true;
		}
};

/*
 * void SolverTheory::addFuncConstraints()
 * DESCRIPTION
 *		Adds constraints to the theory that state that each of the functions that occur in the theory is indeed a function.
 *		This method should be called before running the SAT solver and after grounding.
 */
void SolverTheory::addFuncConstraints() {
	
	for(unsigned int n = 0; n < _translator->nrOffsets(); ++n) {
		PFSymbol* pfs = _translator->getSymbol(n);
		const map<vector<domelement>,int>& tuples = _translator->getTuples(n);
		if(!(pfs->ispred()) && !(tuples.empty())) {
			Function* f = dynamic_cast<Function*>(pfs);
			SortTable* st = _structure->inter(f->outsort());
			DomelementEquality de(f->arity());
			vector<vector<int> > sets(1);
			map<vector<domelement>,int>::const_iterator pit = tuples.begin();
			for(map<vector<domelement>,int>::const_iterator it = tuples.begin(); it != tuples.end(); ++it) {
				if(de(it->first,pit->first)) sets.back().push_back(it->second);
				else sets.push_back(vector<int>(1,it->second));
				pit = it;
			}
			for(unsigned int s = 0; s < sets.size(); ++s) {
				vector<double> lw(sets[s].size(),1);
				vector<double> tw(0);
				int setnr = _translator->translateSet(sets[s],lw,tw);
				int tseitin;
				if(f->partial() || !(st->finite()) || st->size() != sets[s].size()) {
					tseitin = _translator->translate(setnr,AGGCARD,'>',false,1,TS_IMPL);
				}
				else {
					tseitin = _translator->translate(setnr,AGGCARD,'=',true,1,TS_IMPL);
				}
				addUnitClause(tseitin);
			}
		}
	}
	
}

void SolverTheory::addFalseDefineds() {
	for(unsigned int n = 0; n < _translator->nrOffsets(); ++n) {
		PFSymbol* s = _translator->getSymbol(n);
		map<PFSymbol*,set<int> >::const_iterator it = _defined.find(s);
		if(it != _defined.end()) {
			const map<vector<domelement>,int>& tuples = _translator->getTuples(n);
			for(map<vector<domelement>,int>::const_iterator jt = tuples.begin(); jt != tuples.end(); ++jt) {
				if(it->second.find(jt->second) == it->second.end()) addUnitClause(-jt->second);
			}
		}
	}
}

		void addDefinition(GroundDefinition& d)		{ transformForAdd(d);
													  _definitions.push_back(d); }
