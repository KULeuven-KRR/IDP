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
