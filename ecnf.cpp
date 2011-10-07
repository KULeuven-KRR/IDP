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

PCGroundRule::PCGroundRule(int head, PCTsBody* body, bool rec)
		:GroundRule(head, body->conj()?RT_CONJ:RT_DISJ,rec), _body(body->body()) { }
AggGroundRule::AggGroundRule(int head, AggTsBody* body, bool rec)
		:GroundRule(head, RT_AGG,rec), _setnr(body->setnr()), _aggtype(body->aggtype()), _lower(body->lower()), _bound(body->bound()) { }

GroundDefinition* GroundDefinition::clone() const {
	assert(false); //TODO
	GroundDefinition* newdef = new GroundDefinition(_id, _translator);
//	for(ruleit = _rules.begin(); ruleit != _rules.end(); ++ruleit)
		//TODO clone rules...	
	return newdef;
}

void GroundDefinition::recursiveDelete() {
	for(ruleiterator it = begin(); it != end(); ++it){
		delete((*it).second);
	}
	delete(this);
}

void GroundDefinition::addTrueRule(int head) {
	addPCRule(head, vector<int>(0), true, false);
}

void GroundDefinition::addFalseRule(int head) {
	addPCRule(head, vector<int>(0), false, false);
}

// FIXME check that all heads are correct!
void GroundDefinition::addPCRule(int head, const vector<int>& body, bool conj, bool recursive) {
	// Search for a rule with the same head
	map<int, GroundRule*>::iterator it = _rules.find(head);

	if (it == _rules.end()) { // There is not yet a rule with the same head
		_rules[head] = new PCGroundRule(head, (conj ? RT_CONJ : RT_DISJ), body, recursive);
	} else if ((it->second)->isFalse()) { // The existing rule is false
		PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
		grb->type(conj ? RT_CONJ : RT_DISJ);
		grb->head(head);
		grb->body(body);
		grb->recursive(recursive);
	} else if (body.empty()) { // We are adding a rule with a true or false body
		if (conj) {
			delete (it->second);
			it->second = new PCGroundRule(head, RT_CONJ, body, false);
		}
	} else if (!(it->second)->isTrue()) { // There is a rule with the same head, and it is not true or false
		switch (it->second->type()) {
			case RT_DISJ: {
				PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
				if ((!conj) || body.size() == 1) {
					for (unsigned int n = 0; n < body.size(); ++n){
						grb->body().push_back(body[n]);
					}
				} else if (grb->body().size() == 1) {
					grb->type(RT_CONJ);
					for (unsigned int n = 0; n < body.size(); ++n){
						grb->body().push_back(body[n]);
					}
				} else {
					int ts = _translator->translate(body, conj, (recursive ? TS_RULE : TS_EQ));
					grb->body().push_back(ts);
				}
				grb->recursive(grb->recursive() || recursive);
				break;
			}
			case RT_CONJ: {
				PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
				if (grb->body().size() == 1) {
					grb->type(conj ? RT_CONJ : RT_DISJ);
					for (unsigned int n = 0; n < body.size(); ++n)
						grb->body().push_back(body[n]);
				}
				if ((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb->body(), true, (grb->recursive() ? TS_RULE : TS_EQ));
					grb->type(RT_DISJ);
					grb->body(body);
					grb->body().push_back(ts);
				} else {
					int ts1 = _translator->translate(grb->body(), true, (grb->recursive() ? TS_RULE : TS_EQ));
					int ts2 = _translator->translate(body, conj, (recursive ? TS_RULE : TS_EQ));
					grb->type(RT_DISJ);
					vector<int> vi(2);
					vi[0] = ts1;
					vi[1] = ts2;
					grb->body() = vi;
				}
				grb->recursive(grb->recursive() || recursive);
				break;
			}
			case RT_AGG: {
				AggGroundRule* grb = dynamic_cast<AggGroundRule*>(it->second);
				char comp = (grb->lower() ? '<' : '>');
				if ((!conj) || body.size() == 1) {
					int ts = _translator->translate(grb->bound(), comp, false, grb->aggtype(), grb->setnr(),
							(grb->recursive() ? TS_RULE : TS_EQ));
					PCGroundRule* newgrb = new PCGroundRule(head, RT_DISJ, body, (recursive || grb->recursive()));
					newgrb->body().push_back(ts);
					delete (grb);
					it->second = newgrb;
				} else {
					int ts1 = _translator->translate(grb->bound(), comp, false, grb->aggtype(), grb->setnr(),
							(grb->recursive() ? TS_RULE : TS_EQ));
					int ts2 = _translator->translate(body, conj, (recursive ? TS_RULE : TS_EQ));
					vector<int> vi(2);
					vi[0] = ts1;
					vi[1] = ts2;
					it->second = new PCGroundRule(head, RT_DISJ, vi, (recursive || grb->recursive()));
					delete (grb);
				}
				break;
			}
		}
	}
}

void GroundDefinition::addAggRule(int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) {
	// Check if there exists a rule with the same head
	map<int,GroundRule*>::iterator it = _rules.find(head);

	if(it == _rules.end()) {
		_rules[head] = new AggGroundRule(head, setnr,aggtype,lower,bound,recursive);
	}
	else if((it->second)->isFalse()) {
		delete(it->second);
		it->second = new AggGroundRule(head, setnr,aggtype,lower,bound,recursive);
	}
	else if(!(it->second->isTrue())) {
		switch(it->second->type()) {
			case RT_DISJ: {
				PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
				int ts = _translator->translate(bound,(lower ? '<' : '>'),false,aggtype,setnr,(recursive ? TS_RULE : TS_EQ));
				grb->body().push_back(ts);
				grb->recursive(grb->recursive() || recursive);
				break;
			}
			case RT_CONJ: {
				PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,aggtype,setnr,(recursive ? TS_RULE : TS_EQ));
				if(grb->body().size() == 1) {
					grb->type(RT_DISJ);
					grb->body().push_back(ts2);
				}
				else {
					int ts1 = _translator->translate(grb->body(),true,(grb->recursive() ? TS_RULE : TS_EQ));
					vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
					grb->type(RT_DISJ);
					grb->body(vi);
				}
				grb->recursive(grb->recursive() || recursive);
				break;
			}
			case RT_AGG: {
				AggGroundRule* grb = dynamic_cast<AggGroundRule*>(it->second);
				int ts1 = _translator->translate(grb->bound(),(grb->lower()? '<' : '>'),false,grb->aggtype(),grb->setnr(),(grb->recursive() ? TS_RULE : TS_EQ));
				int ts2 = _translator->translate(bound,(lower ? '<' : '>'),false,aggtype,setnr,(recursive ? TS_RULE : TS_EQ));
				vector<int> vi(2); vi[0] = ts1; vi[1] = ts2;
				it->second = new PCGroundRule(head, RT_DISJ,vi,(recursive || grb->recursive()));
				delete(grb);
				break;
			}
		}
	}
}

ostream& GroundDefinition::put(ostream& s, unsigned int ) const {
	bool longnames = false; // TODO longnames?
	s << "{\n";
	for(auto it = _rules.begin(); it != _rules.end(); ++it) {
		s << _translator->printAtom((*it).second->head(), longnames) << " <- ";
		auto body = (*it).second;
		if(body->type() == RT_AGG) {
			const AggGroundRule* grb = dynamic_cast<const AggGroundRule*>(body);
			s << grb->bound() << (grb->lower() ? " =< " : " >= ");
			switch(grb->aggtype()) {
				case AGG_CARD: s << "#"; break;
				case AGG_SUM: s << "sum"; break;
				case AGG_PROD: s << "prod"; break;
				case AGG_MIN: s << "min"; break;
				case AGG_MAX: s << "max"; break;
			}
			s << grb->setnr() << ".\n";
		}
		else {
			const PCGroundRule* grb = dynamic_cast<const PCGroundRule*>(body);
			char c = grb->type() == RT_CONJ ? '&' : '|';
			if(!grb->body().empty()) {
				if(grb->body()[0] < 0) s << '~';
				s << _translator->printAtom(grb->body()[0], longnames);
				for(unsigned int n = 1; n < grb->body().size(); ++n) {
					s << ' ' << c << ' ';
					if(grb->body()[n] < 0) s << '~';
					s << _translator->printAtom(grb->body()[n], longnames);
				}
			}
			else if(grb->type() == RT_CONJ) s << "true";
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
	for(auto it = d->begin(); it != d->end(); ++it){
		(*it).second->accept(this);
	}
}

void TheoryVisitor::visit(const AggGroundRule*) {
	// TODO
}

void TheoryVisitor::visit(const PCGroundRule*) {
	// TODO
}

void TheoryVisitor::visit(const GroundSet*) {
	// TODO
}

void TheoryVisitor::visit(const GroundAggregate*) {
	// TODO
}

GroundDefinition* TheoryMutatingVisitor::visit(GroundDefinition* d) {
	for(auto it = d->begin(); it != d->end(); ++it){
		(*it).second=(*it).second->accept(this);
	}
	return d;
}

GroundRule* TheoryMutatingVisitor::visit(AggGroundRule* r) {
	// TODO
	return r;
}

GroundRule* TheoryMutatingVisitor::visit(PCGroundRule* r) {
	// TODO
	return r;
}
