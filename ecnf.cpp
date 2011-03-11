/************************************
	ecnf.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ecnf.hpp"
#include <iostream>
#include <sstream>

/*************************
	Ground definitions
*************************/

void GroundDefinition::addTrueRule(int head) {
	addPCRule(head,vector<int>(0),true,false);
}

void GroundDefinition::addFalseRule(int head) {
	addPCRule(head,vector<int>(0),false,false);
}

void GroundDefinition::addPCRule(int head, const vector<int>& body, bool conj, bool recursive) {
	// Search for a rule with the same head
	map<int,GroundRuleBody*>::iterator it = _rules.find(head);

	if(it == _rules.end()) {	// There is not yet a rule with the same head
		_rules[head] = new PCGroundRuleBody((conj ? RT_CONJ : RT_DISJ), body, recursive);
	}
	else if((it->second)->isFalse()) { // The existing rule is false
		PCGroundRuleBody* grb = dynamic_cast<PCGroundRuleBody*>(it->second);
		grb->_type = (conj ? RT_CONJ : RT_DISJ);
		grb->_body = body;
		grb->_recursive = recursive;
	}
	else if(body.empty()) {	// We are adding a rule with a true or false body
		if(conj) {
			delete(it->second);
			it->second = new PCGroundRuleBody(RT_CONJ,body,false);
		}
	}
	else if(!(it->second)->isTrue()) {	// There is a rule with the same head, and it is not true or false
		switch(it->second->_type) {
			case RT_DISJ:
			{
				PCGroundRuleBody* grb = dynamic_cast<PCGroundRuleBody*>(it->second);
				if((!conj) || body.size() == 1) {
					for(unsigned int n = 0; n < body.size(); ++n) grb->_body.push_back(body[n]);
				}
				else if(grb->_body.size() == 1) {
					grb->_type = RT_CONJ;
					for(unsigned int n = 0; n < body.size(); ++n) grb->_body.push_back(body[n]);
				}
				else {
					int ts = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					grb->_body.push_back(ts);
				}
				grb->_recursive = grb->_recursive || recursive;
				break;
			}
			case RT_CONJ:
			{
				PCGroundRuleBody* grb = dynamic_cast<PCGroundRuleBody*>(it->second);
				if(grb->_body.size() == 1) {
					grb->_type = conj ? RT_CONJ : RT_DISJ;
					for(unsigned int n = 0; n < body.size(); ++n) grb->_body.push_back(body[n]);
				}
				if((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb->_body,true,(grb->_recursive ? TS_RULE : TS_EQ));
					grb->_type = RT_DISJ;
					grb->_body = body;
					grb->_body.push_back(ts);
				}
				else {
					int ts1 = _translator->translate(grb->_body,true,(grb->_recursive ? TS_RULE : TS_EQ));
					int ts2 = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					grb->_type = RT_DISJ;
					vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
					grb->_body = vi;
				}
				grb->_recursive = grb->_recursive || recursive;
				break;
			}
			case RT_AGG:
			{
				AggGroundRuleBody* grb = dynamic_cast<AggGroundRuleBody*>(it->second);
				if((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb->_bound,(grb->_lower ? '<' : '>'),false,grb->_setnr,grb->_aggtype,grb->_recursive ? TS_RULE : TS_EQ);
					PCGroundRuleBody* newgrb = new PCGroundRuleBody(RT_DISJ,body,recursive || grb->_recursive);
					newgrb->_body.push_back(ts);
					delete(grb);
					it->second = newgrb;
				}
				else {
					int ts1 = _translator->translate(grb->_bound,(grb->_lower ? '<' : '>'),false,grb->_setnr,grb->_aggtype,grb->_recursive ? TS_RULE : TS_EQ);
					int ts2 = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
					it->second = new PCGroundRuleBody(RT_DISJ,vi,recursive || grb->_recursive);
					delete(grb);
				}
				break;
			}
			default:
				assert(false);
		}
	}
}

void GroundDefinition::addAggRule(int head, int setnr, AggType aggtype, bool lower, double bound, bool recursive) {
	// Check if there exists a rule with the same head
	map<int,GroundRuleBody*>::iterator it = _rules.find(head);

	if(it == _rules.end()) {
		_rules[head] = new AggGroundRuleBody(setnr,aggtype,lower,bound,recursive);
	}
	else if((it->second)->isFalse()) {
		delete(it->second);
		it->second = new AggGroundRuleBody(setnr,aggtype,lower,bound,recursive);
	}
	else if(!(it->second->isTrue())) {
		switch(it->second->_type) {
			case RT_DISJ: {
				PCGroundRuleBody* grb = dynamic_cast<PCGroundRuleBody*>(it->second);
				int ts = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
				grb->_body.push_back(ts);
				grb->_recursive = grb->_recursive || recursive;
				break;
			}
			case RT_CONJ: {
				PCGroundRuleBody* grb = dynamic_cast<PCGroundRuleBody*>(it->second);
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
				if(grb->_body.size() == 1) {
					grb->_type = RT_DISJ;
					grb->_body.push_back(ts2);
				}
				else {
					int ts1 = _translator->translate(grb->_body,true,(grb->_recursive ? TS_RULE : TS_EQ));
					vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
					grb->_type = RT_DISJ;
					grb->_body = vi;
				}
				grb->_recursive = grb->_recursive || recursive;
				break;
			}
			case RT_AGG: {
				AggGroundRuleBody* grb = dynamic_cast<AggGroundRuleBody*>(it->second);
				int ts1 = _translator->translate(grb->_bound,(grb->_lower ? '<' : '>'),false,grb->_setnr,grb->_aggtype,(grb->_recursive ? TS_RULE : TS_EQ));
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,setnr,aggtype,(recursive ? TS_RULE : TS_EQ));
				vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
				it->second = new PCGroundRuleBody(RT_DISJ,vi,recursive || grb->_recursive);
				delete(grb);
				break;
			}
		}
	}
}

string GroundDefinition::to_string() const {
	stringstream s;
	s << "{\n";
	for(map<int,GroundRuleBody*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) {
		s << _translator->printatom(it->first) << " <- ";
		const GroundRuleBody* body = it->second;
		if(body->_type == RT_AGG) {
			const AggGroundRuleBody* grb = dynamic_cast<const AggGroundRuleBody*>(body);
			s << grb->_bound << (grb->_lower ? " =< " : " >= ");
			switch(grb->_aggtype) {
				case AGGCARD: s << "#"; break;
				case AGGSUM: s << "sum"; break;
				case AGGPROD: s << "prod"; break;
				case AGGMIN: s << "min"; break;
				case AGGMAX: s << "max"; break;
			}
			s << grb->_setnr << ".\n";
		}
		else {
			const PCGroundRuleBody* grb = dynamic_cast<const PCGroundRuleBody*>(body);
			char c = grb->_type == RT_CONJ ? '&' : '|';
			if(!grb->_body.empty()) {
				if(grb->_body[0] < 0) s << '~';
				s << _translator->printatom(grb->_body[0]);
				for(unsigned int n = 1; n < grb->_body.size(); ++n) {
					s << ' ' << c << ' ';
					if(grb->_body[n] < 0) s << '~';
					s << _translator->printatom(grb->_body[n]);
				}
			}
			else if(grb->_type == RT_CONJ) s << "true";
			else s << "false";
			s << ".\n";
		}
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
void AbstractGroundTheory::transformForAdd(vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst) {
	unsigned int n = 0;
	if(skipfirst) ++n;
	for(; n < vi.size(); ++n) {
		int atom = abs(vi[n]);
		if(_translator->isTseitin(atom) && _printedtseitins.find(atom) == _printedtseitins.end()) {
			_printedtseitins.insert(atom);
			TsBody* tsbody = _translator->tsbody(atom);
			if(typeid(*tsbody) == typeid(PCTsBody)) {
				PCTsBody* body = dynamic_cast<PCTsBody*>(tsbody);
				if(body->_type == TS_IMPL || body->_type == TS_EQ) {
					if(body->_conj) {
						for(unsigned int m = 0; m < body->_body.size(); ++m) {
							vector<int> cl(2,-atom);
							cl[1] = body->_body[m];
							addClause(cl,true);	
						}
					}
					else {
						vector<int> cl(body->_body.size()+1,-atom);
						for(unsigned int m = 0; m < body->_body.size(); ++m) cl[m+1] = body->_body[m];
						addClause(cl,true);
					}
				}
				if(body->_type == TS_RIMPL || body->_type == TS_EQ) {
					if(body->_conj) {
						vector<int> cl(body->_body.size()+1,atom);
						for(unsigned int m = 0; m < body->_body.size(); ++m) cl[m+1] = -body->_body[m];
						addClause(cl,true);
					}
					else {
						for(unsigned int m = 0; m < body->_body.size(); ++m) {
							vector<int> cl(2,atom);
							cl[1] = -body->_body[m];
							addClause(cl,true);
						}
					}
				}
				if(body->_type == TS_RULE) {
					assert(defnr != _nodef);
					addPCRule(defnr,atom,body);
				}
			}
			else {	// body of the tseitin is an aggregate expression
				assert(typeid(*tsbody) == typeid(AggTsBody));
				AggTsBody* body = dynamic_cast<AggTsBody*>(tsbody);
				if(body->_type == TS_RULE) {
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
	for(map<int,GroundRuleBody*>::iterator it = d._rules.begin(); it != d._rules.end(); ++it) {
		int head = it->first;
		if(_printedtseitins.find(head) == _printedtseitins.end()) {
			GroundRuleBody* grb = it->second;
			if(typeid(*grb) == typeid(PCGroundRuleBody)) {
				PCGroundRuleBody* pcgrb = dynamic_cast<PCGroundRuleBody*>(grb);
				transformForAdd(pcgrb->_body,pcgrb->_type == RT_CONJ ? VIT_CONJ : VIT_DISJ,defnr);
			}
			else {
				assert(typeid(*grb) == typeid(AggGroundRuleBody));
				AggGroundRuleBody* agggrb = dynamic_cast<AggGroundRuleBody*>(grb);
				addSet(agggrb->_setnr,defnr,agggrb->_aggtype != AGGCARD);
			}
		}
	}
}

void GroundTheory::addFixpDef(GroundFixpDef& ) {
	assert(false);
	/* TODO */
}

void GroundTheory::addAggregate(int head, AggTsBody* body) {
	addSet(body->_setnr,_nodef,body->_aggtype != AGGCARD);
	GroundAggregate agg(body->_aggtype,body->_lower,body->_type,head,body->_setnr,body->_bound);
	_aggregates.push_back(agg);
}

void GroundTheory::addSet(int setnr, int defnr, bool) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		TsSet& grs = _translator->groundset(setnr);
		transformForAdd(grs._setlits,VIT_SET,defnr);
		_sets.push_back(GroundSet(setnr,grs._setlits,grs._litweights));
	}
}

void GroundTheory::addPCRule(int defnr, int tseitin, PCTsBody* body) {
	assert(_definitions[defnr]._rules.find(tseitin) == _definitions[defnr]._rules.end());
	transformForAdd(body->_body,body->_conj ? VIT_CONJ : VIT_DISJ, defnr);
	_definitions[defnr].addPCRule(tseitin,body->_body,body->_conj,true);
}

void GroundTheory::addAggRule(int defnr, int tseitin, AggTsBody* body) {
	assert(_definitions[defnr]._rules.find(tseitin) == _definitions[defnr]._rules.end());
	addSet(body->_setnr,defnr,body->_aggtype != AGGCARD);
	_definitions[defnr].addAggRule(tseitin,body->_setnr,body->_aggtype,body->_lower,body->_bound,true);
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
		for(unsigned int m = 0; m < _sets[n]._setlits.size(); ++m) {
			s << _translator->printatom(_sets[n]._setlits[m]);
			s << _sets[n]._litweights[m];
		}
		s << "}\n";
	}
	for(unsigned int n = 0; n < _aggregates.size(); ++n) {
		const GroundAggregate& agg = _aggregates[n];
		s << _translator->printatom(agg._head);
		switch(agg._arrow) {
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
		TsSet& grs = _translator->groundset(setnr);
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

void SolverTheory::addFixpDef(GroundFixpDef& ) {
	// TODO
	assert(false);
}

void SolverTheory::addAggregate(int head, AggTsBody* body) {
	addSet(body->_setnr,_nodef,body->_aggtype != AGGCARD);
	MinisatID::AggSign sg = body->_lower ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	MinisatID::AggType tp;
	switch(body->_aggtype) {
		case AGGCARD: tp = MinisatID::CARD; break;
		case AGGSUM: tp = MinisatID::SUM; break;
		case AGGPROD: tp = MinisatID::PROD; break;
		case AGGMIN: tp = MinisatID::MIN; break;
		case AGGMAX: tp = MinisatID::MAX; break;
	}
	MinisatID::AggSem sem;
	switch(body->_type) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL: sem = MinisatID::COMP; break;
		case TS_RULE: sem = MinisatID::DEF; break;
	}
	MinisatID::Literal headlit(head,false);
	MinisatID::Weight weight(int(body->_bound));		// TODO: remove cast if supported by the solver
	_solver->addAggrExpr(headlit,body->_setnr,weight,sg,tp,sem);
}

void SolverTheory::addDefinition(GroundDefinition& d) {
	int defnr = 1; //FIXME: We should ask the solver to give us the next number
	for(map<int,GroundRuleBody*>::iterator it = d._rules.begin(); it != d._rules.end(); ++it) {
		GroundRuleBody* grb = it->second;
		if(typeid(*grb) == typeid(PCGroundRuleBody)) {
			PCGroundRuleBody* pcgrb = static_cast<PCGroundRuleBody*>(grb);
			addPCRule(defnr,it->first,pcgrb);
		}
		else {
			assert(typeid(*grb) == typeid(AggGroundRuleBody));
			AggGroundRuleBody* agggrb = dynamic_cast<AggGroundRuleBody*>(grb);
			addAggRule(defnr,it->first,agggrb);
		}
		// add head to set of defined atoms
		_defined[_translator->symbol(it->first)].insert(it->first);
	}
}

void SolverTheory::addPCRule(int defnr, int head, PCGroundRuleBody* grb) {
	transformForAdd(grb->_body,(grb->_type == RT_CONJ ? VIT_CONJ : VIT_DISJ),defnr);
	MinisatID::Atom mhead(head);
	vector<MinisatID::Literal> mbody;
	for(unsigned int n = 0; n < grb->_body.size(); ++n) {
		MinisatID::Literal l(abs(grb->_body[n]),grb->_body[n]<0);
		mbody.push_back(l);
	}
	if(grb->_type == RT_CONJ)
		_solver->addConjRule(mhead,mbody);
	else
		_solver->addDisjRule(mhead,mbody);
}

void SolverTheory::addAggRule(int defnr, int head, AggGroundRuleBody* grb) {
	addSet(grb->_setnr,defnr,grb->_aggtype != AGGCARD);
	MinisatID::AggSign sg = grb->_lower ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	MinisatID::AggType tp;
	switch(grb->_aggtype) {
		case AGGCARD: tp = MinisatID::CARD; break;
		case AGGSUM: tp = MinisatID::SUM; break;
		case AGGPROD: tp = MinisatID::PROD; break;
		case AGGMIN: tp = MinisatID::MIN; break;
		case AGGMAX: tp = MinisatID::MAX; break;
	}
	MinisatID::AggSem sem = MinisatID::DEF;
	MinisatID::Literal headlit(head,false);
	MinisatID::Weight weight(int(grb->_bound));		// TODO: remove cast if supported by the solver
	_solver->addAggrExpr(headlit,grb->_setnr,weight,sg,tp,sem);
}

void SolverTheory::addPCRule(int defnr, int head, PCTsBody* tsb) {
	transformForAdd(tsb->_body,(tsb->_conj ? VIT_CONJ : VIT_DISJ),defnr);
	MinisatID::Atom mhead(head);
	vector<MinisatID::Literal> mbody;
	for(unsigned int n = 0; n < tsb->_body.size(); ++n) {
		MinisatID::Literal l(abs(tsb->_body[n]),tsb->_body[n]<0);
		mbody.push_back(l);
	}
	if(tsb->_conj)
		_solver->addConjRule(mhead,mbody);
	else
		_solver->addDisjRule(mhead,mbody);
}

void SolverTheory::addAggRule(int defnr, int head, AggTsBody* body) {
	addSet(body->_setnr,defnr,body->_aggtype != AGGCARD);
	MinisatID::AggSign sg = body->_lower ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	MinisatID::AggType tp;
	switch(body->_aggtype) {
		case AGGCARD: tp = MinisatID::CARD; break;
		case AGGSUM: tp = MinisatID::SUM; break;
		case AGGPROD: tp = MinisatID::PROD; break;
		case AGGMIN: tp = MinisatID::MIN; break;
		case AGGMAX: tp = MinisatID::MAX; break;
	}
	MinisatID::AggSem sem = MinisatID::DEF;
	MinisatID::Literal headlit(head,false);
	MinisatID::Weight weight(int(body->_bound));		// TODO: remove cast if supported by the solver
	_solver->addAggrExpr(headlit,body->_setnr,weight,sg,tp,sem);
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
					tseitin = _translator->translate(1,'>',false,setnr,AGGCARD,TS_IMPL);
				}
				else {
					tseitin = _translator->translate(1,'=',true,setnr,AGGCARD,TS_IMPL);
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
