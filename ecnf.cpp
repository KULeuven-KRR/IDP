/************************************
	ecnf.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ecnf.hpp"

#include <iostream>
#include <sstream>

#include "vocabulary.hpp"
#include "structure.hpp"
#include "ground.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

using namespace std;

/*************************
	Ground definitions
*************************/

GroundDefinition* GroundDefinition::clone() const {
	assert(false); //TODO
	GroundDefinition* newdef = new GroundDefinition(_translator);
//	for(ruleit = _rules.begin(); ruleit != _rules.end(); ++ruleit)
		//TODO clone rules...	
	return newdef;
}

void GroundDefinition::recursiveDelete() {
	for(ruleiterator it = begin(); it != end(); ++it)
		delete(it->second);
	delete(this);
}

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
					for(unsigned int n = 0; n < body.size(); ++n)
						grb->_body.push_back(body[n]);
				}
				else if(grb->_body.size() == 1) {
					grb->_type = RT_CONJ;
					for(unsigned int n = 0; n < body.size(); ++n)
						grb->_body.push_back(body[n]);
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
					for(unsigned int n = 0; n < body.size(); ++n) 
						grb->_body.push_back(body[n]);
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
				char comp = (grb->_lower ? '<' : '>');
				if((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb->_bound,comp,false,grb->_aggtype,grb->_setnr,(grb->_recursive ? TS_RULE : TS_EQ));
					PCGroundRuleBody* newgrb = new PCGroundRuleBody(RT_DISJ,body,(recursive || grb->_recursive));
					newgrb->_body.push_back(ts);
					delete(grb);
					it->second = newgrb;
				}
				else {
					int ts1 = _translator->translate(grb->_bound,comp,false,grb->_aggtype,grb->_setnr,(grb->_recursive ? TS_RULE : TS_EQ));
					int ts2 = _translator->translate(body,conj,(recursive ? TS_RULE : TS_EQ));
					vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
					it->second = new PCGroundRuleBody(RT_DISJ,vi,(recursive || grb->_recursive));
					delete(grb);
				}
				break;
			}
			default:
				assert(false);
		}
	}
}

void GroundDefinition::addAggRule(int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) {
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
				int ts = _translator->translate(bound,(lower ? '<' : '>'),false,aggtype,setnr,(recursive ? TS_RULE : TS_EQ));
				grb->_body.push_back(ts);
				grb->_recursive = grb->_recursive || recursive;
				break;
			}
			case RT_CONJ: {
				PCGroundRuleBody* grb = dynamic_cast<PCGroundRuleBody*>(it->second);
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,aggtype,setnr,(recursive ? TS_RULE : TS_EQ));
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
				int ts1 = _translator->translate(grb->_bound,(grb->_lower ? '<' : '>'),false,grb->_aggtype,grb->_setnr,(grb->_recursive ? TS_RULE : TS_EQ));
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,aggtype,setnr,(recursive ? TS_RULE : TS_EQ));
				vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
				it->second = new PCGroundRuleBody(RT_DISJ,vi,(recursive || grb->_recursive));
				delete(grb);
				break;
			}
		}
	}
}

ostream& GroundDefinition::put(ostream& s, unsigned int ) const {
	s << "{\n";
	for(map<int,GroundRuleBody*>::const_iterator it = _rules.begin(); it != _rules.end(); ++it) {
		s << _translator->printAtom(it->first) << " <- ";
		const GroundRuleBody* body = it->second;
		if(body->_type == RT_AGG) {
			const AggGroundRuleBody* grb = dynamic_cast<const AggGroundRuleBody*>(body);
			s << grb->_bound << (grb->_lower ? " =< " : " >= ");
			switch(grb->_aggtype) {
				case AGG_CARD: s << "#"; break;
				case AGG_SUM: s << "sum"; break;
				case AGG_PROD: s << "prod"; break;
				case AGG_MIN: s << "min"; break;
				case AGG_MAX: s << "max"; break;
			}
			s << grb->_setnr << ".\n";
		}
		else {
			const PCGroundRuleBody* grb = dynamic_cast<const PCGroundRuleBody*>(body);
			char c = grb->_type == RT_CONJ ? '&' : '|';
			if(!grb->_body.empty()) {
				if(grb->_body[0] < 0) s << '~';
				s << _translator->printAtom(grb->_body[0]);
				for(unsigned int n = 1; n < grb->_body.size(); ++n) {
					s << ' ' << c << ' ';
					if(grb->_body[n] < 0) s << '~';
					s << _translator->printAtom(grb->_body[n]);
				}
			}
			else if(grb->_type == RT_CONJ) s << "true";
			else s << "false";
			s << ".\n";
		}
	}
	s << "}\n";
	return s;
}

string GroundDefinition::to_string(unsigned int) const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}

/*******************************
	Abstract Ground theories
*******************************/

const int ID_FOR_UNDEFINED = -1;

AbstractGroundTheory::AbstractGroundTheory(AbstractStructure* str) : 
	AbstractTheory("",ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator()) { }

AbstractGroundTheory::AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str) : 
	AbstractTheory("",voc,ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator()) { }

AbstractGroundTheory::~AbstractGroundTheory() {
	delete(_structure);
	delete(_translator);
	delete(_termtranslator);
}

void AbstractGroundTheory::recursiveDelete() {
	delete(this);
}

/*
 * AbstractGroundTheory::transformForAdd(vector<int>& vi, VIType vit, int defnr, bool skipfirst)
 * DESCRIPTION
 *		Adds defining rules for tseitin literals in a given vector of literals to the ground theory.
 *		This method may apply unfolding which changes the given vector of literals.
 * PARAMETERS
 *		vi			- given vector of literals
 *		vit			- indicates whether vi represents a disjunction, conjunction or set of literals
 *		defnr		- number of the definition vi belongs to. Is NODEF when vi does not belong to a definition
 *		skipfirst	- if true, the defining rule for the first literal is not added to the ground theory
 * TODO
 *		implement unfolding
 */
void AbstractGroundTheory::transformForAdd(const vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst) {
	unsigned int n = 0;
	if(skipfirst) ++n;
	for(; n < vi.size(); ++n) {
		int atom = abs(vi[n]);
		if(_translator->isTseitin(atom) && _printedtseitins.find(atom) == _printedtseitins.end()) {
			_printedtseitins.insert(atom);
			TsBody* tsbody = _translator->tsbody(atom);
			if(typeid(*tsbody) == typeid(PCTsBody)) {
				PCTsBody* body = dynamic_cast<PCTsBody*>(tsbody);
				if(body->type() == TS_IMPL || body->type() == TS_EQ) {
					if(body->conj()) {
						for(unsigned int m = 0; m < body->size(); ++m) {
							vector<int> cl(2,-atom);
							cl[1] = body->literal(m);
							addClause(cl,true);	
						}
					}
					else {
						vector<int> cl(body->size()+1,-atom);
						for(unsigned int m = 0; m < body->size(); ++m)
							cl[m+1] = body->literal(m);
						addClause(cl,true);
					}
				}
				if(body->type() == TS_RIMPL || body->type() == TS_EQ) {
					if(body->conj()) {
						vector<int> cl(body->size()+1,atom);
						for(unsigned int m = 0; m < body->size(); ++m)
							cl[m+1] = -body->literal(m);
						addClause(cl,true);
					}
					else {
						for(unsigned int m = 0; m < body->size(); ++m) {
							vector<int> cl(2,atom);
							cl[1] = -body->literal(m);
							addClause(cl,true);
						}
					}
				}
				if(body->type() == TS_RULE) {
					assert(defnr != ID_FOR_UNDEFINED);
					addPCRule(defnr,atom,body);
				}
			}
			else if(typeid(*tsbody) == typeid(AggTsBody)) {
				AggTsBody* body = dynamic_cast<AggTsBody*>(tsbody);
				if(body->type() == TS_RULE) {
					assert(defnr != ID_FOR_UNDEFINED);
					addAggRule(defnr,atom,body);
				}
				else {
					addAggregate(atom,body);
				}
			}
			else {
#ifdef TEMP_CP
				assert(typeid(*tsbody) == typeid(CPTsBody));
				CPTsBody* body = dynamic_cast<CPTsBody*>(tsbody);
				if(body->type() == TS_RULE) {
					assert(false);
					//TODO
				}
				else {
					addCPReification(atom,body);
				}
#else
				assert(false);
#endif
			}
		}
	}
}


/*******************************
	Internal ground theories
*******************************/

void GroundTheory::recursiveDelete() {
	for(vector<GroundDefinition*>::iterator defit = _definitions.begin(); defit != _definitions.end(); ++defit) {
		(*defit)->recursiveDelete();
	}
	for(vector<GroundAggregate*>::iterator aggit = _aggregates.begin(); aggit != _aggregates.end(); ++aggit) {
		delete(*aggit);
	}
	for(vector<GroundSet*>::iterator setit = _sets.begin(); setit != _sets.end(); ++setit) {
		delete(*setit);
	}
	//for(vector<GroundFixpDef*>::iterator fdefit = _fixpdefs.begin(); fdefit != _fixpdefs.end(); ++fdefit) {
	//	(*defit)->recursiveDelete();
	//	delete(*defit);
	//}
	for(vector<CPReification*>::iterator cprit = _cpreifications.begin(); cprit != _cpreifications.end(); ++cprit) {
		delete(*cprit);
	}
}

void GroundTheory::addClause(GroundClause& cl, bool skipfirst) {
	transformForAdd(cl,VIT_DISJ,ID_FOR_UNDEFINED,skipfirst);
	_clauses.push_back(cl);
}

void GroundTheory::addDefinition(GroundDefinition* d) {
	unsigned int defnr = _definitions.size();
	_definitions.push_back(d);
	for(map<int,GroundRuleBody*>::iterator it = d->begin(); it != d->end(); ++it) {
		int head = it->first;
		if(_printedtseitins.find(head) == _printedtseitins.end()) {
			GroundRuleBody* grb = it->second;
			if(typeid(*grb) == typeid(PCGroundRuleBody)) {
				PCGroundRuleBody* pcgrb = dynamic_cast<PCGroundRuleBody*>(grb);
				transformForAdd(pcgrb->body(),(pcgrb->type() == RT_CONJ ? VIT_CONJ : VIT_DISJ),defnr);
			}
			else {
				assert(typeid(*grb) == typeid(AggGroundRuleBody));
				AggGroundRuleBody* agggrb = dynamic_cast<AggGroundRuleBody*>(grb);
				addSet(agggrb->setnr(),defnr,(agggrb->aggtype() != AGG_CARD));
			}
		}
	}
}

void GroundTheory::addFixpDef(GroundFixpDef*) {
	assert(false);
	/* TODO */
}

void GroundTheory::addAggregate(int head, AggTsBody* body) {
	addSet(body->setnr(),ID_FOR_UNDEFINED,(body->aggtype() != AGG_CARD));
	_aggregates.push_back(new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound()));
}

void GroundTheory::addCPReification(int tseitin, CPTsBody* body) {
	_cpreifications.push_back(new CPReification(tseitin,body));
}

void GroundTheory::addSet(int setnr, int defnr, bool weighted) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		TsSet& tsset = _translator->groundset(setnr);
		transformForAdd(tsset.literals(),VIT_SET,defnr);
		vector<double> weights;
		if(weighted) weights = tsset.weights();
		_sets.push_back(new GroundSet(setnr,tsset.literals(),weights));
	}
}

void GroundTheory::addPCRule(int defnr, int tseitin, PCTsBody* body) {
	assert(_definitions[defnr]->rule(tseitin) == _definitions[defnr]->end());
	transformForAdd(body->body(),(body->conj() ? VIT_CONJ : VIT_DISJ), defnr);
	_definitions[defnr]->addPCRule(tseitin,body->body(),body->conj(),true);
}

void GroundTheory::addAggRule(int defnr, int tseitin, AggTsBody* body) {
	assert(_definitions[defnr]->rule(tseitin) == _definitions[defnr]->end());
	addSet(body->setnr(),defnr,(body->aggtype() != AGG_CARD));
	_definitions[defnr]->addAggRule(tseitin,body->setnr(),body->aggtype(),body->lower(),body->bound(),true);
}

ostream& GroundTheory::put(ostream& s, unsigned int) const {
	for(unsigned int n = 0; n < _clauses.size(); ++n) {
		if(_clauses[n].empty()) {
			s << "false";
		}
		else {
			for(unsigned int m = 0; m < _clauses[n].size(); ++m) {
				if(_clauses[n][m] < 0) s << '~';
				s << _translator->printAtom(_clauses[n][m]);
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
		s << _definitions[n]->to_string();
	}
	for(unsigned int n = 0; n < _sets.size(); ++n) {
		s << "Set nr. " << _sets[n]->setnr() << " = [ ";
		for(unsigned int m = 0; m < _sets[n]->size(); ++m) {
			s << "(" << _translator->printAtom(_sets[n]->literal(m));
			s << " = " << _sets[n]->weight(m) << ")";
			if(m < _sets[n]->size()-1) s << "; ";
		}
		s << "].\n";
	}
	for(unsigned int n = 0; n < _aggregates.size(); ++n) {
		const GroundAggregate* agg = _aggregates[n];
		s << _translator->printAtom(agg->head());
		switch(agg->arrow()) {
			case TS_RULE: 	s << " <- "; break;
			case TS_IMPL: 	s << " => "; break;
			case TS_RIMPL: 	s << " <= "; break;
			case TS_EQ: 	s << " <=> "; break;
			default: assert(false);
		}
		s << agg->bound();
		s << (agg->lower() ? " =< " : " >= ");
		switch(agg->type()) {
			case AGG_CARD: 	s << "card("; break;
			case AGG_SUM: 	s << "sum("; break;
			case AGG_PROD: 	s << "prod("; break;
			case AGG_MIN: 	s << "min("; break;
			case AGG_MAX: 	s << "max("; break;
			default: assert(false);
		}
		s << agg->setnr() << ").\n";
	}
	//TODO: repeat above for fixpoint definitions
	for(vector<CPReification*>::const_iterator it = _cpreifications.begin(); it != _cpreifications.end(); ++it) {
		CPReification* cpr = *it;
		s << _translator->printAtom(cpr->_head);
		switch(cpr->_body->type()) {
			case TS_RULE: 	s << " <- "; break;
			case TS_IMPL: 	s << " => "; break;
			case TS_RIMPL: 	s << " <= "; break;
			case TS_EQ: 	s << " <=> "; break;
			default: assert(false);
		}
		CPTerm* left = cpr->_body->left();
		if(typeid(*left) == typeid(CPSumTerm)) {
			CPSumTerm* cpt = dynamic_cast<CPSumTerm*>(left);
			s << "sum[ ";
			for(vector<unsigned int>::const_iterator vit = cpt->_varids.begin(); vit != cpt->_varids.end(); ++vit) {
				s << _termtranslator->printTerm(*vit);
				if(*vit != cpt->_varids.back()) s << "; ";
			}
			s << " ]";
		}
		else if(typeid(*left) == typeid(CPWSumTerm)) {
			CPWSumTerm* cpt = dynamic_cast<CPWSumTerm*>(left);
			vector<unsigned int>::const_iterator vit;
			vector<int>::const_iterator wit;
			s << "wsum[ ";
			for(vit = cpt->_varids.begin(), wit = cpt->_weights.begin(); vit != cpt->_varids.end() && wit != cpt->_weights.end(); ++vit, ++wit) {
				s << "(" << _termtranslator->printTerm(*vit) << "=" << *wit << ")";
				if(*vit != cpt->_varids.back()) s << "; ";
			}
			s << " ]";
		}
		else {
			assert(typeid(*left) == typeid(CPVarTerm));
			CPVarTerm* cpt = dynamic_cast<CPVarTerm*>(left);
			s << _termtranslator->printTerm(cpt->_varid);
		}
		switch(cpr->_body->comp()) {
			case CT_EQ:		s << " = "; break;
			case CT_NEQ:	s << " ~= "; break;
			case CT_LEQ:	s << " =< "; break;
			case CT_GEQ:	s << " >= "; break;
			case CT_LT:		s << " < "; break;
			case CT_GT:		s << " > "; break;
			default: assert(false);
		}
		CPBound right = cpr->_body->right();
		if(right._isvarid) s << _termtranslator->printTerm(right._value._varid);
		else s << right._value._bound;
		s << ".\n";
	}
	return s;
}

string GroundTheory::to_string() const {
	stringstream s;
	put(s);
	return s.str();
}

//Formula* GroundTheory::sentence(unsigned int n) const{
//	if(n < _clauses.size()) return _clauses[n];
//	else return _aggregates[n-_clauses.size()]
//}


/**********************
	Solver theories
**********************/

SolverTheory::SolverTheory(SATSolver* solver,AbstractStructure* str) :
			AbstractGroundTheory(str), _solver(solver) {
}
SolverTheory::SolverTheory(Vocabulary* voc, SATSolver* solver, AbstractStructure* str) :
			AbstractGroundTheory(voc,str), _solver(solver) {
}

inline MinisatID::Atom createAtom(int lit){
	return MinisatID::Atom(abs(lit));
}

inline MinisatID::Literal createLiteral(int lit){
	return MinisatID::Literal(abs(lit),lit<0);
}

inline MinisatID::Weight createWeight(double weight){
#warning "Dangerous cast from double to int in adding rules to the solver"
	return MinisatID::Weight(int(weight));	// TODO: remove cast if supported by the solver
}

void SolverTheory::addClause(GroundClause& cl, bool skipfirst) {
	transformForAdd(cl,VIT_DISJ,ID_FOR_UNDEFINED,skipfirst);
	MinisatID::Disjunction clause;
	for(unsigned int n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
	}
	getSolver().add(clause);
}

void SolverTheory::addSet(int setnr, int defnr, bool weighted) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		TsSet& tsset = getTranslator().groundset(setnr);
		transformForAdd(tsset.literals(),VIT_SET,defnr);
		if(!weighted){
			MinisatID::Set set;
			set.setID = setnr;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				set.literals.push_back(createLiteral(tsset.literal(n)));
			}
			getSolver().add(set);
		}
		else {
			MinisatID::WSet set;
			set.setID = setnr;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				set.literals.push_back(createLiteral(tsset.literal(n)));
				set.weights.push_back(createWeight(tsset.weight(n)));
			}
			getSolver().add(set);
		}
	}
}

void SolverTheory::addFixpDef(GroundFixpDef*) {
	// TODO
	assert(false);
}

void SolverTheory::addAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound) {
	addSet(setnr,definitionID,(aggtype != AGG_CARD));
	MinisatID::Aggregate agg;
	agg.sign = lowerbound ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	agg.setID = setnr;
	switch (aggtype) {
		case AGG_CARD:
			agg.type = MinisatID::CARD;
			break;
		case AGG_SUM:
			agg.type = MinisatID::SUM;
			break;
		case AGG_PROD:
			agg.type = MinisatID::PROD;
			break;
		case AGG_MIN:
			agg.type = MinisatID::MIN;
			break;
		case AGG_MAX:
			agg.type = MinisatID::MAX;
			break;
	}
	switch(sem) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL: 
			agg.sem = MinisatID::COMP;
			break;
		case TS_RULE:
			agg.sem = MinisatID::DEF;
			break;
	}
	agg.defID = definitionID;
	agg.head = createAtom(head);
	agg.bound = createWeight(bound);
	getSolver().add(agg);
}

void SolverTheory::addAggregate(int head, AggTsBody* body) {
	assert(body->type() != TS_RULE);
	addAggregate(ID_FOR_UNDEFINED,head,body->lower(),body->setnr(),body->aggtype(),body->type(),body->bound());
}

void SolverTheory::addAggRule(int defnr, int head, AggGroundRuleBody* body) {
	addAggregate(defnr,head,body->lower(),body->setnr(),body->aggtype(),TS_RULE,body->bound());
}

void SolverTheory::addAggRule(int defnr, int head, AggTsBody* body) {
	assert(body->type() == TS_RULE);
	addAggregate(defnr,head,body->lower(),body->setnr(),body->aggtype(),body->type(),body->bound());
}

void SolverTheory::addDefinition(GroundDefinition* d) {
	int defnr = 1; //FIXME: We should ask the solver to give us the next number
	for(map<int,GroundRuleBody*>::iterator it = d->begin(); it != d->end(); ++it) {
		int head = it->first;
		GroundRuleBody* grb = it->second;
		// Pass the rule to the definition in the solver
		if(typeid(*grb) == typeid(PCGroundRuleBody)) {
			PCGroundRuleBody* pcgrb = static_cast<PCGroundRuleBody*>(grb);
			addPCRule(defnr,head,pcgrb);
		}
		else {
			assert(typeid(*grb) == typeid(AggGroundRuleBody));
			AggGroundRuleBody* agggrb = dynamic_cast<AggGroundRuleBody*>(grb);
			addAggRule(defnr,head,agggrb);
		}
		// add head to set of defined atoms
		_defined[getTranslator().symbol(head)].insert(head);
	}
}

#ifdef TEMP_CP
void SolverTheory::addCPReification(int tseitin, CPTsBody* body) {
	MinisatID::EqType comp;
	switch(body->comp()) {
		case CT_EQ:		comp = MinisatID::MEQ; break; 
		case CT_NEQ:	comp = MinisatID::MNEQ; break; 
		case CT_LEQ:	comp = MinisatID::MLEQ; break; 
		case CT_GEQ:	comp = MinisatID::MGEQ; break; 
		case CT_LT:		comp = MinisatID::ML; break; 
		case CT_GT:		comp = MinisatID::MG; break;
		default: assert(false);
	} 
	CPTerm* left = body->left();
	CPBound right = body->right();
	if(typeid(*left) == typeid(CPVarTerm)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		addCPVariable(term->_varid);
		if(right._isvarid) {
			addCPVariable(right._value._varid);
			MinisatID::CPBinaryRelVar sentence;
			sentence.head = createAtom(tseitin);
			sentence.lhsvarID = term->_varid;
			sentence.rhsvarID = right._value._varid;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
		else {
			MinisatID::CPBinaryRel sentence;
			sentence.head = createAtom(tseitin);
			sentence.varID = term->_varid;
			sentence.bound = right._value._bound;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
	}
	else if(typeid(*left) == typeid(CPSumTerm)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		addCPVariables(term->_varids);
		if(right._isvarid) {
			addCPVariable(right._value._varid);
			MinisatID::CPSumWithVar sentence;
			sentence.head = createAtom(tseitin);
			sentence.varIDs = term->_varids;
			sentence.rhsvarID = right._value._varid;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
		else {
			MinisatID::CPSum sentence;
			sentence.head = createAtom(tseitin);
			sentence.varIDs = term->_varids;
			sentence.bound = right._value._bound;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
	}
	else {
		assert(typeid(*left) == typeid(CPWSumTerm));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		addCPVariables(term->_varids);
		if(right._isvarid) {
			addCPVariable(right._value._varid);
			MinisatID::CPSumWeightedWithVar sentence;
			sentence.head = createAtom(tseitin);
			sentence.varIDs = term->_varids;
			sentence.weights = term->_weights;
			sentence.rhsvarID = right._value._varid;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
		else {
			MinisatID::CPSumWeighted sentence;
			sentence.head = createAtom(tseitin);
			sentence.varIDs = term->_varids;
			sentence.weights = term->_weights;
			sentence.bound = right._value._bound;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
	}
}

void SolverTheory::addCPVariables(const vector<unsigned int>& varids) {
	for(vector<unsigned int>::const_iterator it = varids.begin(); it != varids.end(); ++it)
		addCPVariable(*it);
}

void SolverTheory::addCPVariable(unsigned int varid) {
	if(_addedvarids.find(varid) == _addedvarids.end()) {
		_addedvarids.insert(varid);
		Function* function = _termtranslator->function(varid);
//cerr << "func = " << function->name();
		SortTable* domain = _structure->inter(function->outsort());
		assert(domain->approxfinite()); 
		const DomainElement* first = domain->first(); 
		int minvalue = first->value()._int;
		const DomainElement* last = domain->last();	
		int maxvalue = last->value()._int;
		assert(maxvalue > minvalue);
		if(domain->isRange()) {
//cerr << " domain = [" << minvalue << "," << maxvalue << "]" << endl;
			// the domain is a complete range from minvalue to maxvalue.
			MinisatID::CPIntVarRange cpvar;
			cpvar.varID = varid;
			cpvar.minvalue = minvalue;
			cpvar.maxvalue = maxvalue;
			getSolver().add(cpvar);
		}
		else {
			// the domain is not a complete range.
			MinisatID::CPIntVarEnum cpvar;
			cpvar.varID = varid;
//cerr << " domain = { ";
			
			for(SortIterator it = domain->sortbegin(); it.hasNext(); ++it) {
				int value = (*it)->value()._int;
//cerr << value << "; ";
				cpvar.values.push_back(value);
			}
//cerr << " }" << endl;
			getSolver().add(cpvar);
		}
	}
}
#endif

void SolverTheory::addPCRule(int defnr, int head, vector<int> body, bool conjunctive){
	transformForAdd(body,(conjunctive ? VIT_CONJ : VIT_DISJ),defnr);
	MinisatID::Rule rule;
	rule.head = createAtom(head);
	for(unsigned int n = 0; n < body.size(); ++n) {
		rule.body.push_back(createLiteral(body[n]));
	}
	rule.conjunctive = conjunctive;
	rule.definitionID = defnr;
	getSolver().add(rule);
}

void SolverTheory::addPCRule(int defnr, int head, PCGroundRuleBody* grb) {
	addPCRule(defnr,head,grb->body(),(grb->type() == RT_CONJ));
}

void SolverTheory::addPCRule(int defnr, int head, PCTsBody* tsb) {
	addPCRule(defnr,head,tsb->body(),tsb->conj());
}

/*
 *		Adds constraints to the theory that state that each of the functions that occur in the theory is indeed a function.
 *		This method should be called before running the SAT solver and after grounding.
 */
void AbstractGroundTheory::addFuncConstraints() {
	for(unsigned int n = 0; n < getTranslator().nrOffsets(); ++n) {
		PFSymbol* pfs = getTranslator().getSymbol(n);
		const map<vector<const DomainElement*>,int,StrictWeakTupleOrdering>& tuples = getTranslator().getTuples(n);
		if((typeid(*pfs) == typeid(Function))  && !(tuples.empty())) {
			Function* f = dynamic_cast<Function*>(pfs);
			StrictWeakNTupleEquality de(f->arity());
			StrictWeakNTupleOrdering ds(f->arity());

			const PredTable* ct = _structure->inter(f)->graphinter()->ct();
			const PredTable* pt = _structure->inter(f)->graphinter()->pt();
			SortTable* st = _structure->inter(f->outsort());

			ElementTuple input(f->arity(),0);
			TableIterator tit = ct->begin();
			SortIterator sit = st->sortbegin();
			vector<vector<int> > sets;
			vector<bool> weak;
			for(map<vector<const DomainElement*>,int,StrictWeakTupleOrdering>::const_iterator it = tuples.begin(); it != tuples.end(); ) {
				if(de(it->first,input)) {
					sets.back().push_back(it->second);
					while(*sit != it->first.back()) {
						ElementTuple temp = input; temp.push_back(*sit);
						if(pt->contains(temp)) {
							weak.back() = true;
							break;
						}
						++sit;
					}
					++it;
					if(sit.hasNext()) ++sit;
				}
				else {
					if(!sets.empty() && sit.hasNext()) weak.back() = true;
					if(tit.hasNext()) {
						const ElementTuple& tuple = *tit;
						if(de(tuple,it->first)) {
							do { 
								if(it->first != tuple) addUnitClause(-(it->second));
								++it; 
							} while(it != tuples.end() && de(tuple,it->first));
							continue;
						}
						else if(ds(tuple,it->first)) {
							do { ++tit; } while(tit.hasNext() && ds(*tit,it->first));
							continue;
						}
					}
					sets.push_back(vector<int>(0));
					weak.push_back(false);
					input = it->first; input.pop_back();
					sit = st->sortbegin();
				}
			}
			for(unsigned int s = 0; s < sets.size(); ++s) {
				vector<double> lw(sets[s].size(),1);
				vector<double> tw(0);
				int setnr = getTranslator().translateSet(sets[s],lw,tw);
				int tseitin;
				if(f->partial() || !(st->finite()) || weak[s]) {
					tseitin = getTranslator().translate(1,'>',false,AGG_CARD,setnr,TS_IMPL);
				}
				else {
					tseitin = getTranslator().translate(1,'=',true,AGG_CARD,setnr,TS_IMPL);
				}
				addUnitClause(tseitin);
			}
		}
	}
}

void SolverTheory::addFalseDefineds() {
	for(unsigned int n = 0; n < getTranslator().nrOffsets(); ++n) {
		PFSymbol* s = getTranslator().getSymbol(n);
		map<PFSymbol*,set<int> >::const_iterator it = _defined.find(s);
		if(it != _defined.end()) {
			const map<vector<const DomainElement*>,int,StrictWeakTupleOrdering>& tuples = getTranslator().getTuples(n);
			for(map<vector<const DomainElement*>,int>::const_iterator jt = tuples.begin(); jt != tuples.end(); ++jt) {
				if(it->second.find(jt->second) == it->second.end()) addUnitClause(-jt->second);
			}
		}
	}
}

/**************
	Visitor
**************/

void TheoryVisitor::visit(const GroundDefinition* d) {
	for(map<int,GroundRuleBody*>::const_iterator it = d->begin(); it != d->end(); ++it) 
		it->second->accept(this);
}

void TheoryVisitor::visit(const AggGroundRuleBody*) {
	// TODO
}

void TheoryVisitor::visit(const PCGroundRuleBody*) {
	// TODO
}

void TheoryVisitor::visit(const GroundSet*) {
	// TODO
}

void TheoryVisitor::visit(const GroundAggregate*) {
	// TODO
}

GroundDefinition* TheoryMutatingVisitor::visit(GroundDefinition* d) {
	for(map<int,GroundRuleBody*>::iterator it = d->begin(); it != d->end(); ++it) 
		it->second = it->second->accept(this);
	return d;
}

GroundRuleBody* TheoryMutatingVisitor::visit(AggGroundRuleBody* r) {
	// TODO
	return r;
}

GroundRuleBody* TheoryMutatingVisitor::visit(PCGroundRuleBody* r) {
	// TODO
	return r;
}
