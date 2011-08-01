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
	GroundDefinition* newdef = new GroundDefinition(_id, _translator);
//	for(ruleit = _rules.begin(); ruleit != _rules.end(); ++ruleit)
		//TODO clone rules...	
	return newdef;
}

void GroundDefinition::recursiveDelete() {
	for(ruleiterator it = begin(); it != end(); ++it){
		delete(*it);
	}
	delete(this);
}

void GroundDefinition::add(GroundRule* rule){
	_rules.push_back(rule);
}

ostream& GroundDefinition::put(ostream& s, unsigned int ) const {
	s << "{\n";
	for(auto it = _rules.begin(); it != _rules.end(); ++it) {
		s << _translator->printAtom((*it)->head()) << " <- ";
		const GroundRule* body = *it;
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
				s << _translator->printAtom(grb->body()[0]);
				for(unsigned int n = 1; n < grb->body().size(); ++n) {
					s << ' ' << c << ' ';
					if(grb->body()[n] < 0) s << '~';
					s << _translator->printAtom(grb->body()[n]);
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
		(*it)->accept(this);
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
		*it = (*it)->accept(this);
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
