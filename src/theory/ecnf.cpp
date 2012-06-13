/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IncludeComponents.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/grounders/LazyFormulaGrounders.hpp"

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
IMPLACCEPTNONMUTATING(CPSumTerm)
IMPLACCEPTNONMUTATING(CPWSumTerm)

IMPLACCEPTNONMUTATING(GroundSet)
IMPLACCEPTNONMUTATING(GroundAggregate)
IMPLACCEPTNONMUTATING(CPReification)

PCGroundRule::PCGroundRule(Lit head, PCTsBody* body, bool rec)
		: 	GroundRule(head, body->conj() ? RuleType::CONJ : RuleType::DISJ, rec),
			_body(body->body()) {
}
AggGroundRule::AggGroundRule(Lit head, AggTsBody* body, bool rec)
		: 	GroundRule(head, RuleType::AGG, rec),
			_setnr(body->setnr()),
			_aggtype(body->aggtype()),
			_lower(body->lower()),
			_bound(body->bound()) {
}

GroundDefinition* GroundDefinition::clone() const {
	throw notyetimplemented("Cloning grounddefinitions");
	GroundDefinition* newdef = new GroundDefinition(_id, _translator);
//	for(ruleit = _rules.cbegin(); ruleit != _rules.cend(); ++ruleit)
	//TODO clone rules...
	return newdef;
}

void GroundDefinition::recursiveDelete() {
	for (ruleiterator it = begin(); it != end(); ++it) {
		delete ((*it).second);
	}
	delete (this);
}

void GroundDefinition::addTrueRule(Lit head) {
	addPCRule(head, litlist(0), true, false);
}

void GroundDefinition::addFalseRule(Lit head) {
	addPCRule(head, litlist(0), false, false);
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
				Lit ts = _translator->translate(body, conj, (recursive ? TsType::RULE : TsType::EQ));
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
				Lit ts = _translator->translate(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				grb->type(RuleType::DISJ);
				grb->body(body);
				grb->body().push_back(ts);
			} else {
				Lit ts1 = _translator->translate(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				Lit ts2 = _translator->translate(body, conj, (recursive ? TsType::RULE : TsType::EQ));
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
				Lit ts = _translator->translate(grb->bound(), comp, grb->aggtype(), grb->setnr(), (grb->recursive() ? TsType::RULE : TsType::EQ));
				auto newgrb = new PCGroundRule(head, RuleType::DISJ, body, (recursive || grb->recursive()));
				newgrb->body().push_back(ts);
				delete (grb);
				it->second = newgrb;
			} else {
				Lit ts1 = _translator->translate(grb->bound(), comp, grb->aggtype(), grb->setnr(), (grb->recursive() ? TsType::RULE : TsType::EQ));
				Lit ts2 = _translator->translate(body, conj, (recursive ? TsType::RULE : TsType::EQ));
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
		_rules[head] = new AggGroundRule(head, setnr, aggtype, lower, bound, recursive);
	} else if ((it->second)->isFalse()) {
		delete (it->second);
		it->second = new AggGroundRule(head, setnr, aggtype, lower, bound, recursive);
	} else if (not (it->second->isTrue())) {
		switch (it->second->type()) {
		case RuleType::DISJ: {
			auto grb = dynamic_cast<PCGroundRule*>(it->second);
			Lit ts = _translator->translate(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			grb->body().push_back(ts);
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RuleType::CONJ: {
			auto grb = dynamic_cast<PCGroundRule*>(it->second);
			Lit ts2 = _translator->translate(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			if (grb->body().size() == 1) {
				grb->type(RuleType::DISJ);
				grb->body().push_back(ts2);
			} else {
				Lit ts1 = _translator->translate(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				grb->type(RuleType::DISJ);
				grb->body( { ts1, ts2 });
			}
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RuleType::AGG: {
			auto grb = dynamic_cast<AggGroundRule*>(it->second);
			Lit ts1 = _translator->translate(grb->bound(), (grb->lower() ? CompType::LEQ : CompType::GEQ), grb->aggtype(), grb->setnr(),
					(grb->recursive() ? TsType::RULE : TsType::EQ));
			Lit ts2 = _translator->translate(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
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
		s << toString(_domelement);
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

CPTsBody::~CPTsBody() {
	delete (_left);
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

bool CPSumTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPSumTerm&>(body);
	return _varids == rhs._varids;
}

bool CPSumTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPSumTerm&>(body);
	if (_varids < rhs._varids) {
		return true;
	}
	return false;
}

bool CPWSumTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPWSumTerm&>(body);
	return _varids == rhs._varids;
}

bool CPWSumTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (not CPTerm::operator==(body)) {
		return false;
	}
	const auto& rhs = dynamic_cast<const CPWSumTerm&>(body);
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

void LazyTsBody::notifyTheoryOccurence(Lit tseitin) {
	inst->grounder->notifyTheoryOccurrence(tseitin, inst, _type);
}

void TsBody::put(std::ostream& stream) const {
	stream << toString(_type);
}
void CPTsBody::put(std::ostream& stream) const {
	stream << toString(type()) << " " << toString(left()) << toString(comp()) << toString(right());
}
void CPVarTerm::put(std::ostream& stream) const {
	stream << "var" << varid();
}
void CPBound::put(std::ostream& stream) const {
	stream << (_isvarid ? "var" : "") << (_isvarid ? _varid : _bound);
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
