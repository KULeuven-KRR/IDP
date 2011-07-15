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
	AbstractTheory("",ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator(str)) { }

AbstractGroundTheory::AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str) : 
	AbstractTheory("",voc,ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator(str)) { }

AbstractGroundTheory::~AbstractGroundTheory() {
	delete(_structure);
	delete(_translator);
	delete(_termtranslator);
}

void AbstractGroundTheory::recursiveDelete() {
	delete(this);
}

/**
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
				assert(typeid(*tsbody) == typeid(CPTsBody));
				CPTsBody* body = dynamic_cast<CPTsBody*>(tsbody);
				if(body->type() == TS_RULE) {
					assert(false);
					//TODO Does this ever happen?
				}
				else {
					addCPReification(atom,body);
				}
			}
		}
	}
}

CPTerm* AbstractGroundTheory::foldCPTerm(CPTerm* cpterm) {
	if(_foldedterms.find(cpterm) == _foldedterms.end()) {
		_foldedterms.insert(cpterm);
		if(typeid(*cpterm) == typeid(CPVarTerm)) {
			CPVarTerm* varterm = static_cast<CPVarTerm*>(cpterm);
			if(not _termtranslator->function(varterm->_varid)) {
				CPTsBody* cprelation = _termtranslator->cprelation(varterm->_varid);
				CPTerm* left = foldCPTerm(cprelation->left());
				if((typeid(*left) == typeid(CPSumTerm) || typeid(*left) == typeid(CPWSumTerm)) && cprelation->comp() == CT_EQ) {
					assert(cprelation->right()._isvarid && cprelation->right()._varid == varterm->_varid);
					return left;
				}
			}
		}
		else if(typeid(*cpterm) == typeid(CPSumTerm)) {
			CPSumTerm* sumterm = static_cast<CPSumTerm*>(cpterm);
			vector<VarId> newvarids;
			for(vector<VarId>::const_iterator it = sumterm->_varids.begin(); it != sumterm->_varids.end(); ++it) {
				if(not _termtranslator->function(*it)) {
					CPTsBody* cprelation = _termtranslator->cprelation(*it);
					CPTerm* left = foldCPTerm(cprelation->left());
					if(typeid(*left) == typeid(CPSumTerm) && cprelation->comp() == CT_EQ) {
						CPSumTerm* subterm = static_cast<CPSumTerm*>(left);
						assert(cprelation->right()._isvarid && cprelation->right()._varid == *it);
						newvarids.insert(newvarids.end(),subterm->_varids.begin(),subterm->_varids.end());
					}
					//TODO Need to do something special in other cases?
					else newvarids.push_back(*it);
				}
				else newvarids.push_back(*it);
			}
			sumterm->_varids = newvarids;
		}
		else if(typeid(*cpterm) == typeid(CPWSumTerm)) {
			//CPWSumTerm* wsumterm = static_cast<CPWSumTerm*>(cpterm);
			//TODO
		}
	}
	return cpterm;
}

//CPTerm* AbstractGroundTheory::foldCPTerm(CPVarTerm* cpterm) {
//	if(not _termtranslator->function(cpterm->_varid)) {
//		CPTsBody* cprelation = _termtranslator->cprelation(cpterm->_varid);
//		CPTerm* left = foldCPTerm(cprelation->left());
//		if((typeid(*left) == typeid(CPSumTerm) || typeid(*left) == typeid(CPWSumTerm)) && cprelation->comp() == CT_EQ) {
//			assert(cprelation->right()._isvarid && cprelation->right()._varid == cpterm->_varid);
//			return left;
//		}
//	}
//	return cpterm;
//}
//
//CPTerm* AbstractGroundTheory::foldCPTerm(CPSumTerm* cpterm) {
//	//TODO
//	vector<VarId> newvarids;
//	for(vector<VarId>::const_iterator it = cpterm->_varids.begin(); it != cpterm->_varids.end(); ++it) {
//		if(not _termtranslator->function(*it)) {
//			CPTsBody* cprelation = _termtranslator->cprelation(*it);
//			CPTerm* left = foldCPTerm(cprelation->left());
//			if(typeid(*left) == typeid(CPSumTerm) && cprelation->comp() == CT_EQ) {
//				CPSumTerm* subterm = static_cast<CPSumTerm*>(left);
//				assert(cprelation->right()._isvarid && cprelation->right()._varid == *it);
//				newvarids.insert(newvarids.end(),subterm->_varids.begin(),subterm->_varids.end());
//			}
//			//TODO Need to do something special in other cases?
//			else newvarids.push_back(*it);
//		}
//	}
//	cpterm->_varids = newvarids;
//	return cpterm;
//}
//
//CPTerm* AbstractGroundTheory::foldCPTerm(CPWSumTerm*) {
//	//TODO Do something similar to CPSumTerm case.
//}


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
	//TODO
}

void GroundTheory::addAggregate(int head, AggTsBody* body) {
	addSet(body->setnr(),ID_FOR_UNDEFINED,(body->aggtype() != AGG_CARD));
	_aggregates.push_back(new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound()));
}

void GroundTheory::addCPReification(int tseitin, CPTsBody* body) {
	//TODO also add variables (in a separate container?)
	
	CPTsBody* foldedbody = new CPTsBody(body->type(),foldCPTerm(body->left()),body->comp(),body->right());
	//FIXME possible leaks!!

	_cpreifications.push_back(new CPReification(tseitin,foldedbody));
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
		s << _translator->printAtom(agg->head()) << ' ' << agg->arrow() << ' ' << agg->bound();
		s << (agg->lower() ? " =< " : " >= ");
		s << agg->type() << '(' << agg->setnr() << ")." << endl;
	}
	//TODO: repeat above for fixpoint definitions
	for(vector<CPReification*>::const_iterator it = _cpreifications.begin(); it != _cpreifications.end(); ++it) {
		CPReification* cpr = *it;
		s << _translator->printAtom(cpr->_head) << ' ' << cpr->_body->type() << ' ';
		CPTerm* left = cpr->_body->left();
		if(typeid(*left) == typeid(CPSumTerm)) {
			CPSumTerm* cpt = dynamic_cast<CPSumTerm*>(left);
			s << "sum[ ";
			for(vector<unsigned int>::const_iterator vit = cpt->_varids.begin(); vit != cpt->_varids.end(); ++vit) {
				s << _termtranslator->printTerm(*vit);
				if(vit != cpt->_varids.end()-1) s << "; ";
			}
			s << " ]";
		}
		else if(typeid(*left) == typeid(CPWSumTerm)) {
			CPWSumTerm* cpt = dynamic_cast<CPWSumTerm*>(left);
			vector<unsigned int>::const_iterator vit;
			vector<int>::const_iterator wit;
			s << "wsum[ ";
			for(vit = cpt->_varids.begin(), wit = cpt->_weights.begin(); vit != cpt->_varids.end() && wit != cpt->_weights.end(); ++vit, ++wit) {
				s << '(' << _termtranslator->printTerm(*vit) << '=' << *wit << ')';
				if(vit != cpt->_varids.end()-1) s << "; ";
			}
			s << " ]";
		}
		else {
			assert(typeid(*left) == typeid(CPVarTerm));
			CPVarTerm* cpt = dynamic_cast<CPVarTerm*>(left);
			s << _termtranslator->printTerm(cpt->_varid);
		}
		s << ' ' << cpr->_body->comp() << ' ';
		CPBound right = cpr->_body->right();
		if(right._isvarid) s << _termtranslator->printTerm(right._varid);
		else s << right._bound;
		s << '.' << endl;
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

SolverTheory::SolverTheory(SATSolver* solver, AbstractStructure* str, int verbosity) :
			AbstractGroundTheory(str), _solver(solver), _verbosity(verbosity) {
}
SolverTheory::SolverTheory(Vocabulary* voc, SATSolver* solver, AbstractStructure* str, int verbosity) :
			AbstractGroundTheory(voc,str), _solver(solver), _verbosity(verbosity) {
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
	if(_verbosity > 0) clog << "clause ";
	for(unsigned int n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
		if(_verbosity > 0) clog << (cl[n] > 0 ? "" : "~") << _translator->printAtom(cl[n]) << ' ';
	}
	if(_verbosity > 0) clog << endl;
	getSolver().add(clause);
}

void SolverTheory::addSet(int setnr, int defnr, bool weighted) {
	if(_printedsets.find(setnr) == _printedsets.end()) {
		_printedsets.insert(setnr);
		TsSet& tsset = getTranslator().groundset(setnr);
		transformForAdd(tsset.literals(),VIT_SET,defnr);
		if(!weighted){
		if(_verbosity > 0) clog << "set ";
			MinisatID::Set set;
			set.setID = setnr;
			if(_verbosity > 0) clog << setnr;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				set.literals.push_back(createLiteral(tsset.literal(n)));
				if(_verbosity > 0) clog << (tsset.literal(n) > 0 ? "" : "~") << _translator->printAtom(tsset.literal(n)) << ' ';
			}
			if(_verbosity > 0) clog << endl;
			getSolver().add(set);
		}
		else {
			if(_verbosity > 0) clog << "wset ";
			MinisatID::WSet set;
			set.setID = setnr;
			if(_verbosity > 0) clog << setnr;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				set.literals.push_back(createLiteral(tsset.literal(n)));
				set.weights.push_back(createWeight(tsset.weight(n)));
				if(_verbosity > 0) clog << (tsset.literal(n) > 0 ? "" : "~") << _translator->printAtom(tsset.literal(n)) << "=" << tsset.weight(n) << ' ';
			}
			getSolver().add(set);
			if(_verbosity > 0) clog << endl;
		}
	}
}

void SolverTheory::addFixpDef(GroundFixpDef*) {
	// TODO
	assert(false);
}

void SolverTheory::addAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound) {
	addSet(setnr,definitionID,(aggtype != AGG_CARD));
	if(_verbosity > 0) clog << "aggregate: " << _translator->printAtom(head) << ' ' << (sem == TS_RULE ? "<- " : "<=> ");
	MinisatID::Aggregate agg;
	agg.sign = lowerbound ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	agg.setID = setnr;
	switch (aggtype) {
		case AGG_CARD:
			agg.type = MinisatID::CARD;
			if(_verbosity > 0) clog << "card ";
			break;
		case AGG_SUM:
			agg.type = MinisatID::SUM;
			if(_verbosity > 0) clog << "sum ";
			break;
		case AGG_PROD:
			agg.type = MinisatID::PROD;
			if(_verbosity > 0) clog << "prod ";
			break;
		case AGG_MIN:
			agg.type = MinisatID::MIN;
			if(_verbosity > 0) clog << "min ";
			break;
		case AGG_MAX:
			if(_verbosity > 0) clog << "max ";
			agg.type = MinisatID::MAX;
			break;
	}
	if(_verbosity > 0) clog << setnr << ' ';
	switch(sem) {
		case TS_EQ: case TS_IMPL: case TS_RIMPL: 
			agg.sem = MinisatID::COMP;
			break;
		case TS_RULE:
			agg.sem = MinisatID::DEF;
			break;
	}
	if(_verbosity > 0) clog << (lowerbound ? " >= " : " =< ") << bound << endl; 
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

#ifdef CPSUPPORT
void addWeightedSum(const MinisatID::Atom& head, const vector<VarId>& varids, const vector<int> weights, const int& bound, MinisatID::EqType rel, SATSolver& solver){
	MinisatID::CPSumWeighted sentence;
	sentence.head = head;
	sentence.varIDs = varids;
	sentence.weights = weights;
	sentence.bound = bound;
	sentence.rel = rel;
	solver.add(sentence);
}
#endif

void SolverTheory::addCPReification(int tseitin, CPTsBody* body) {
#ifndef CPSUPPORT
	//TODO cleanly catch this
	cerr <<"Interrupted because writing cp-constraints to the solver, which are not compiled in.\n";
	throw exception();
#else
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
	CPTerm* left = foldCPTerm(body->left());
	CPBound right = body->right();
	if(typeid(*left) == typeid(CPVarTerm)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		addCPVariable(term->_varid);
		if(right._isvarid) {
			addCPVariable(right._varid);
			MinisatID::CPBinaryRelVar sentence;
			sentence.head = createAtom(tseitin);
			sentence.lhsvarID = term->_varid;
			sentence.rhsvarID = right._varid;
			sentence.rel = comp;
			getSolver().add(sentence);
		} else {
			MinisatID::CPBinaryRel sentence;
			sentence.head = createAtom(tseitin);
			sentence.varID = term->_varid;
			sentence.bound = right._bound;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
	} else if(typeid(*left) == typeid(CPSumTerm)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		addCPVariables(term->_varids);
		if(right._isvarid) {
			addCPVariable(right._varid);
			vector<VarId> varids = term->_varids;
			vector<int> weights;
			weights.resize(1, term->_varids.size());

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			addWeightedSum(createAtom(tseitin), varids, weights, bound, comp, getSolver());
		} else {
			vector<int> weights;
			weights.resize(1, term->_varids.size());
			addWeightedSum(createAtom(tseitin), term->_varids, weights, right._bound, comp, getSolver());
		}
	} else {
		assert(typeid(*left) == typeid(CPWSumTerm));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		addCPVariables(term->_varids);
		if(right._isvarid) {
			addCPVariable(right._varid);
			vector<VarId> varids = term->_varids;
			vector<int> weights = term->_weights;

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			addWeightedSum(createAtom(tseitin), varids, weights, bound, comp, getSolver());
		} else {
			addWeightedSum(createAtom(tseitin), term->_varids, term->_weights, right._bound, comp, getSolver());
		}
	}
#endif //CPSUPPORT
}

#ifdef CPSUPPORT
void SolverTheory::addCPVariables(const vector<VarId>& varids) {
	for(vector<VarId>::const_iterator it = varids.begin(); it != varids.end(); ++it) {
		addCPVariable(*it);
	}
}

void SolverTheory::addCPVariable(const VarId& varid) {
	if(_addedvarids.find(varid) == _addedvarids.end()) {
		_addedvarids.insert(varid);
		Function* function = _termtranslator->function(varid);
		if(_verbosity > 0) { 
			clog << "Adding domain for var_" << varid;
			if(function) clog << " (function " << *function << ")";
	   		clog << ": ";
		}
		SortTable* domain = _termtranslator->domain(varid);
		assert(domain);
		assert(domain->approxfinite()); 
		if(domain->isRange()) {
			// the domain is a complete range from minvalue to maxvalue.
			MinisatID::CPIntVarRange cpvar;
			cpvar.varID = varid;
			cpvar.minvalue = domain->first()->value()._int;
			cpvar.maxvalue = domain->last()->value()._int;
			if(_verbosity > 0) clog << "[" << cpvar.minvalue << "," << cpvar.maxvalue << "]";
			getSolver().add(cpvar);
		}
		else {
			// the domain is not a complete range.
			MinisatID::CPIntVarEnum cpvar;
			cpvar.varID = varid;
			if(_verbosity > 0) clog << "{ ";
			for(SortIterator it = domain->sortbegin(); it.hasNext(); ++it) {
				int value = (*it)->value()._int;
				cpvar.values.push_back(value);
				if(_verbosity > 0) clog << value << "; ";
			}
			if(_verbosity > 0) clog << "}";
			getSolver().add(cpvar);
		}
		if(_verbosity > 0) clog << endl;
	}
}
#endif //CPSUPPORT

void SolverTheory::addPCRule(int defnr, int head, vector<int> body, bool conjunctive){
	transformForAdd(body,(conjunctive ? VIT_CONJ : VIT_DISJ),defnr);
	MinisatID::Rule rule;
	rule.head = createAtom(head);
	if(_verbosity > 0) clog << (rule.conjunctive ? "conjunctive" : "disjunctive") << "rule " << _translator->printAtom(head) << " <- ";
	for(unsigned int n = 0; n < body.size(); ++n) {
		rule.body.push_back(createLiteral(body[n]));
		if(_verbosity > 0) clog << (body[n] > 0 ? "" : "~") << _translator->printAtom(body[n]) << ' ';
	}
	rule.conjunctive = conjunctive;
	rule.definitionID = defnr;
	getSolver().add(rule);
	if(_verbosity > 0) clog << endl;
}

void SolverTheory::addPCRule(int defnr, int head, PCGroundRuleBody* grb) {
	addPCRule(defnr,head,grb->body(),(grb->type() == RT_CONJ));
}

void SolverTheory::addPCRule(int defnr, int head, PCTsBody* tsb) {
	addPCRule(defnr,head,tsb->body(),tsb->conj());
}

/**
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
				if(de(it->first,input) && !sets.empty()) {
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

int AbstractGroundTheory::getFreeTseitin(){
	int result = _translator->nextNumber(); 
	_printedtseitins.insert(result);
	return result;
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
