/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "IncludeComponents.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/lazygrounders/LazyInst.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

using namespace std;

/*************************
 Ground definitions
 *************************/

IMPLACCEPTBOTH(PCGroundRule, GroundRule)
IMPLACCEPTBOTH(AggGroundRule, GroundRule)
IMPLACCEPTBOTH(GroundDefinition, AbstractDefinition)

IMPLACCEPTNONMUTATING(CPVarTerm)
IMPLACCEPTNONMUTATING(CPSetTerm)

IMPLACCEPTNONMUTATING(GroundSet)
IMPLACCEPTNONMUTATING(GroundAggregate)
IMPLACCEPTNONMUTATING(CPReification)

PCGroundRule::PCGroundRule(Lit head, PCTsBody* body, bool rec)
		: 	GroundRule(head, body->conj() ? RuleType::CONJ : RuleType::DISJ, rec),
			_body(body->body()) {
}
AggGroundRule::AggGroundRule(Lit head, AggTsBody* body, bool rec)
		: 	GroundRule(head, RuleType::AGG, rec),
		  	_bound(body->bound()),
			_lower(body->lower()),
			_setnr(body->setnr()),
			_aggtype(body->aggtype()) {
}

bool GroundDefinition::hasRule(Atom head) const {
	return contains(rules(), head);
}

GroundDefinition* GroundDefinition::clone() const {
	throw notyetimplemented("Cloning grounddefinitions");
	auto newdef = new GroundDefinition(_id, _translator);
//	for(ruleit = _rules.cbegin(); ruleit != _rules.cend(); ++ruleit)
	//TODO clone rules...
	return newdef;
}

void GroundDefinition::recursiveDelete() {
	for (auto rule : rules()) {
		delete (rule.second);
	}
	delete (this);
}

void GroundDefinition::addPCRule(Lit head, const litlist& body, bool conj, bool recursive) {
	// Search for a rule with the same head
	auto it = _rules.find(head);
	if (it == _rules.cend()) { // There is not yet a rule with the same head
		_rules[head] = new PCGroundRule(head, (conj ? RuleType::CONJ : RuleType::DISJ), body, recursive);
	} else if ((it->second)->isFalse()) { // The existing rule is false
		auto grb = dynamic_cast<PCGroundRule*>(it->second);
		grb->type(conj ? RuleType::CONJ : RuleType::DISJ);
		grb->head(head);
		grb->body(body);
		grb->recursive(recursive);
	} else if (body.empty()) { // We are adding a rule with a true or false body
		if (conj) {
			delete (it->second);
			it->second = new PCGroundRule(head, RuleType::CONJ, body, false);
		}
	} else if (not (it->second)->isTrue()) { // There is a rule with the same head, and it is not true or false
		switch (it->second->type()) {
		case RuleType::DISJ: {
			auto grb = dynamic_cast<PCGroundRule*>(it->second);
			if ((not conj) || body.size() == 1) {
				for (size_t n = 0; n < body.size(); ++n) {
					grb->body().push_back(body[n]);
				}
			} else {
				Lit ts = _translator->reify(body, conj, (recursive ? TsType::RULE : TsType::EQ));
				grb->body().push_back(ts);
			}
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RuleType::CONJ: {
			auto grb = dynamic_cast<PCGroundRule*>(it->second);
			if (grb->body().size() == 1 && ((not conj) || body.size() == 1)) {
				grb->type(RuleType::DISJ);
				for (size_t n = 0; n < body.size(); ++n) {
					grb->body().push_back(body[n]);
				}
			} else if ((not conj) || body.size() == 1) {
				Lit ts = _translator->reify(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				grb->type(RuleType::DISJ);
				grb->body(body);
				grb->body().push_back(ts);
			} else {
				Lit ts1 = _translator->reify(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				Lit ts2 = _translator->reify(body, conj, (recursive ? TsType::RULE : TsType::EQ));
				grb->type(RuleType::DISJ);
				grb->body() = {ts1, ts2};
			}
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RuleType::AGG: {
			auto grb = dynamic_cast<AggGroundRule*>(it->second);
			CompType comp = (grb->lower() ? CompType::LEQ : CompType::GEQ);
			if ((not conj) || body.size() == 1) {
				Lit ts = _translator->reify(grb->bound(), comp, grb->aggtype(), grb->setnr(), (grb->recursive() ? TsType::RULE : TsType::EQ));
				auto newgrb = new PCGroundRule(head, RuleType::DISJ, body, (recursive || grb->recursive()));
				newgrb->body().push_back(ts);
				delete (grb);
				it->second = newgrb;
			} else {
				Lit ts1 = _translator->reify(grb->bound(), comp, grb->aggtype(), grb->setnr(), (grb->recursive() ? TsType::RULE : TsType::EQ));
				Lit ts2 = _translator->reify(body, conj, (recursive ? TsType::RULE : TsType::EQ));
				it->second = new PCGroundRule(head, RuleType::DISJ, { ts1, ts2 }, (recursive || grb->recursive()));
				delete (grb);
			}
			break;
		}
		}
	}
}

void GroundDefinition::addAggRule(Lit head, SetId setnr, AggFunction aggtype, bool lower, double bound, bool recursive) {
	// Check if there exists a rule with the same head
	map<int, GroundRule*>::iterator it = _rules.find(head);

	if (it == _rules.cend()) {
		_rules[head] = new AggGroundRule(head, bound, lower, setnr, aggtype, recursive);
	} else if ((it->second)->isFalse()) {
		delete (it->second);
		it->second = new AggGroundRule(head, bound, lower, setnr, aggtype, recursive);
	} else if (not (it->second->isTrue())) {
		switch (it->second->type()) {
		case RuleType::DISJ: {
			auto grb = dynamic_cast<PCGroundRule*>(it->second);
			Lit ts = _translator->reify(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			grb->body().push_back(ts);
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RuleType::CONJ: {
			auto grb = dynamic_cast<PCGroundRule*>(it->second);
			Lit ts2 = _translator->reify(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			if (grb->body().size() == 1) {
				grb->type(RuleType::DISJ);
				grb->body().push_back(ts2);
			} else {
				Lit ts1 = _translator->reify(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				grb->type(RuleType::DISJ);
				grb->body( { ts1, ts2 });
			}
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RuleType::AGG: {
			auto grb = dynamic_cast<AggGroundRule*>(it->second);
			Lit ts1 = _translator->reify(grb->bound(), (grb->lower() ? CompType::LEQ : CompType::GEQ), grb->aggtype(), grb->setnr(),
					(grb->recursive() ? TsType::RULE : TsType::EQ));
			Lit ts2 = _translator->reify(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			it->second = new PCGroundRule(head, RuleType::DISJ, { ts1, ts2 }, (recursive || grb->recursive()));
			delete (grb);
			break;
		}
		}
	}
}

ostream& GroundDefinition::put(ostream& s) const {
	s << "{\n";
	for (auto it = _rules.cbegin(); it != _rules.cend(); ++it) {
		s << _translator->printLit((*it).second->head()) << " <- ";
		auto body = (*it).second;
		if (body->type() == RuleType::AGG) {
			auto grb = dynamic_cast<const AggGroundRule*>(body);
			s << grb->bound() << (grb->lower() ? " =< " : " >= ");
			s << grb->aggtype() << grb->setnr() << ".\n";
		} else {
			auto grb = dynamic_cast<const PCGroundRule*>(body);
			char c = grb->type() == RuleType::CONJ ? '&' : '|';
			if (not grb->body().empty()) {
				if (grb->body()[0] < 0) {
					s << '~';
				}
				s << _translator->printLit(grb->body()[0]);
				for (size_t n = 1; n < grb->body().size(); ++n) {
					s << ' ' << c << ' ';
					if (grb->body()[n] < 0) {
						s << '~';
					}
					s << _translator->printLit(grb->body()[n]);
				}
			} else if (grb->type() == RuleType::CONJ) {
				s << "true";
			} else {
				s << "false";
			}
			s << ".\n";
		}
	}
	s << "}\n";
	return s;
}

CPReification::~CPReification() {
	//delete (_body); TODO (might not be necessary)
}

std::ostream& GroundTerm::put(std::ostream& s) const {
	if (isVariable) {
		s << "var_" << _varid;
	} else {
		s << print(_domelement);
	}
	return s;
}

bool operator==(const GroundTerm& a, const GroundTerm& b) {
	if (a.isVariable == b.isVariable) {
		return a.isVariable ? (a._varid == b._varid) : (a._domelement == b._domelement);
	}
	return false;
}

bool operator<(const GroundTerm& a, const GroundTerm& b) {
	if (a.isVariable == b.isVariable) {
		return a.isVariable ? (a._varid < b._varid) : (a._domelement < b._domelement);
	}
	// GroundTerms with a domain element come before GroundTerms with a CP variable identifier.
	return (a.isVariable < b.isVariable);
}

bool TsBody::operator==(const TsBody& body) const {
	if (typeid(*this) != typeid(body)) {
		return false;
	}
	return type() == body.type();
}

bool AggTsBody::operator==(const TsBody& body) const {
	if (not (*this == body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const AggTsBody&>(body);
	return bound() == rhs.bound() && setnr() == rhs.setnr() && lower() == rhs.lower() && aggtype() == rhs.aggtype();
}

bool AggTsBody::operator<(const TsBody& body) const {
	if (TsBody::operator<(body)) {
		return true;
	} else if (TsBody::operator>(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const AggTsBody&>(body);
	if (bound() < rhs.bound()) {
		return true;
	} else if (bound() > rhs.bound()) {
		return false;
	}
	if (lower() < rhs.lower()) {
		return true;
	} else if (lower() > rhs.lower()) {
		return false;
	}
	if (lower() < rhs.lower()) {
		return true;
	} else if (lower() > rhs.lower()) {
		return false;
	}
	if (aggtype() < rhs.aggtype()) {
		return true;
	}
	return false;
}

bool PCTsBody::operator==(const TsBody& other) const {
	if (not TsBody::operator==(other)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const PCTsBody&>(other);
	return body() == rhs.body() && conj() == rhs.conj();
}

bool PCTsBody::operator<(const TsBody& other) const {
	if (TsBody::operator<(other)) {
		return true;
	} else if (not TsBody::operator==(other)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const PCTsBody&>(other);
	if (conj() < rhs.conj()) {
		return true;
	} else if (conj() > rhs.conj()) {
		return false;
	}
	if (body() < rhs.body()) {
		return true;
	}
	return false;
}

bool DenotingTsBody::operator==(const TsBody& other) const {
	if (not TsBody::operator==(other)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const DenotingTsBody&>(other);
	return getVarId() == rhs.getVarId();
}

bool DenotingTsBody::operator<(const TsBody& other) const {
	if (TsBody::operator<(other)) {
		return true;
	} else if (not TsBody::operator==(other)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const DenotingTsBody&>(other);
	return getVarId() < rhs.getVarId();
}

bool compEqThroughNeg(CompType left, CompType right) {
	switch (left) {
	case CompType::EQ:
		if (right == left || right == CompType::NEQ) {
			return true;
		}
		break;
	case CompType::NEQ:
		if (right == left || right == CompType::EQ) {
			return true;
		}
		break;
	case CompType::LEQ:
		if (right == left || right == CompType::GT) {
			return true;
		}
		break;
	case CompType::GEQ:
		if (right == left || right == CompType::LT) {
			return true;
		}
		break;
	case CompType::LT:
		if (right == left || right == CompType::GEQ) {
			return true;
		}
		break;
	case CompType::GT:
		if (right == left || right == CompType::LEQ) {
			return true;
		}
		break;
	}
	return false;
}

bool CPTsBody::operator==(const TsBody& body) const {
	if (not TsBody::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPTsBody&>(body);
	return compEqThroughNeg(comp(), rhs.comp()) && left() == rhs.left() && right() == rhs.right();
}

bool CPTsBody::operator<(const TsBody& body) const {
	if (TsBody::operator<(body)) {
		return true;
	} else if (not TsBody::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPTsBody&>(body);
	if (not compEqThroughNeg(comp(), rhs.comp())) {
		if (comp() < rhs.comp()) {
			return true;
		} else if (comp() > rhs.comp()) {
			return false;
		}
	}
	if (*left() < *rhs.left()) {
		return true;
	} else if (*left() > *rhs.left()) {
		return false;
	}
	return right() < rhs.right();
}

bool CPTerm::operator==(const CPTerm& body) const {
	return typeid(*this) == typeid(body);
}
bool CPTerm::operator>(const CPTerm& rhs) const {
	return not (operator ==(rhs) || operator <(rhs));
}

bool CPVarTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPVarTerm&>(body);
	return _varid == rhs._varid;
}

bool CPVarTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPVarTerm&>(body);
	if (_varid < rhs._varid) {
		return true;
	}
	return false;
}

bool CPSetTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPSetTerm&>(body);
	return _type==rhs._type && _conditions == rhs._conditions && _varids == rhs._varids && _weights==rhs._weights;
}

bool CPSetTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPSetTerm&>(body);
	if(_type<rhs._type){
		return true;
	}else if(_type>rhs._type){
		return false;
	}
	if (_conditions < rhs._conditions) {
		return true;
	} else if (_conditions > rhs._conditions) {
		return false;
	}
	if (_varids < rhs._varids) {
		return true;
	} else if (_varids > rhs._varids) {
		return false;
	}
	if (_weights < rhs._weights) {
		return true;
	}
	return false;
}

bool CPBound::operator==(const CPBound& rhs) const {
	if (_isvarid != rhs._isvarid) {
		return false;
	}
	if (_isvarid) {
		return _varid == rhs._varid;
	} else {
		return _bound == rhs._bound;
	}
}

bool CPBound::operator<(const CPBound& rhs) const {
	if (_isvarid < rhs._isvarid) {
		return true;
	} else if (_isvarid > rhs._isvarid) {
		return false;
	}
	if (_isvarid) {
		return _varid < rhs._varid;
	} else {
		return _bound < rhs._bound;
	}
}

void LazyTsBody::notifyTheoryOccurence() {
	inst->notifyTheoryOccurrence(_type);
}

void TsBody::put(std::ostream& stream) const {
	stream << print(_type);
}
void CPTsBody::put(std::ostream& stream) const {
	stream << print(type()) << " " << print(left()) << print(comp()) << print(right());
}
void CPVarTerm::put(std::ostream& stream) const {
	stream << "var" << varid();
}
void CPBound::put(std::ostream& stream) const {
	stream << (_isvarid ? "var" : "");
	if(_isvarid){
		stream <<_varid;
	}else{
		 stream <<_bound;
	}
}

void CPSetTerm::put(std::ostream& stream) const {
	stream << print(type()) << " of ";
	if (type() == AggFunction::PROD) {
		stream <<weights()[0] <<"*";
	}
	for (uint i = 0; i < varids().size(); ++i) {
		if (type() == AggFunction::SUM) {
			stream << "(" <<conditions()[i] <<", " << weights()[i] << " * var" << varids()[i] << ") + ";
		}else if (type() == AggFunction::PROD) {
			stream << "(" <<conditions()[i] <<", var" << varids()[i] << ") * ";
		}
	}
}

// TODO eclipse indentation problems below this line...
bool TsBody::operator>(const TsBody& rhs) const {
	return not (operator ==(rhs) || operator <(rhs));
}
bool TsBody::operator<(const TsBody& body) const {
//	cerr << "Comparing " << typeid(*this).name() << " with " << typeid(body).name() << "\n";
	if (typeid(*this).before(typeid(body))) {
				return true;
			} else if (typeid(body).before(typeid(*this))) {
						return false;
					}
					return type() < body.type();
				}

				bool CPTerm::operator<(const CPTerm& body) const {
					return typeid(*this).before(typeid(body));
						}
