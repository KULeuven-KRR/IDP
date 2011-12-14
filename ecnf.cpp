/************************************
 ecnf.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "ecnf.hpp"

#include <iostream>
#include <sstream>

#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/grounders/LazyQuantGrounder.hpp"

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

PCGroundRule::PCGroundRule(int head, PCTsBody* body, bool rec) :
		GroundRule(head, body->conj() ? RT_CONJ : RT_DISJ, rec), _body(body->body()) {
}
AggGroundRule::AggGroundRule(int head, AggTsBody* body, bool rec) :
		GroundRule(head, RT_AGG, rec), _setnr(body->setnr()), _aggtype(body->aggtype()), _lower(body->lower()), _bound(body->bound()) {
}

GroundDefinition* GroundDefinition::clone() const {
	throw notyetimplemented("Cloning grounddefinitions is not implemented.");
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

void GroundDefinition::addTrueRule(int head) {
	addPCRule(head, vector<int>(0), true, false);
}

void GroundDefinition::addFalseRule(int head) {
	addPCRule(head, vector<int>(0), false, false);
}

void GroundDefinition::addPCRule(int head, const vector<int>& body, bool conj, bool recursive) {
	// Search for a rule with the same head
	map<int, GroundRule*>::iterator it = _rules.find(head);
	if (it == _rules.cend()) { // There is not yet a rule with the same head
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
				for (unsigned int n = 0; n < body.size(); ++n) {
					grb->body().push_back(body[n]);
				}
			} else {
				int ts = _translator->translate(body, conj, (recursive ? TsType::RULE : TsType::EQ)); //TODO TSType ok?  Not TSType::Rule?
				grb->body().push_back(ts);
			}
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RT_CONJ: {
			PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
			if (grb->body().size() == 1 && ((!conj) || body.size() == 1)){
				grb->type(RT_DISJ);
				for (unsigned int n = 0; n < body.size(); ++n)
					grb->body().push_back(body[n]);
			}
			else if ((!conj) || body.size() == 1) {
				int ts = _translator->translate(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				grb->type(RT_DISJ);
				grb->body(body);
				grb->body().push_back(ts);
			} else {
				int ts1 = _translator->translate(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				int ts2 = _translator->translate(body, conj, (recursive ? TsType::RULE : TsType::EQ));
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
			CompType comp = (grb->lower() ? CompType::LEQ : CompType::GEQ);
			if ((!conj) || body.size() == 1) {
				int ts = _translator->translate(grb->bound(), comp, grb->aggtype(), grb->setnr(), (grb->recursive() ? TsType::RULE : TsType::EQ));
				PCGroundRule* newgrb = new PCGroundRule(head, RT_DISJ, body, (recursive || grb->recursive()));
				newgrb->body().push_back(ts);
				delete (grb);
				it->second = newgrb;
			} else {
				int ts1 = _translator->translate(grb->bound(), comp, grb->aggtype(), grb->setnr(), (grb->recursive() ? TsType::RULE : TsType::EQ));
				int ts2 = _translator->translate(body, conj, (recursive ? TsType::RULE : TsType::EQ));
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
	map<int, GroundRule*>::iterator it = _rules.find(head);

	if (it == _rules.cend()) {
		_rules[head] = new AggGroundRule(head, setnr, aggtype, lower, bound, recursive);
	} else if ((it->second)->isFalse()) {
		delete (it->second);
		it->second = new AggGroundRule(head, setnr, aggtype, lower, bound, recursive);
	} else if (!(it->second->isTrue())) {
		switch (it->second->type()) {
		case RT_DISJ: {
			PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
			int ts = _translator->translate(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			grb->body().push_back(ts);
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RT_CONJ: {
			PCGroundRule* grb = dynamic_cast<PCGroundRule*>(it->second);
			int ts2 = _translator->translate(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			if (grb->body().size() == 1) {
				grb->type(RT_DISJ);
				grb->body().push_back(ts2);
			} else {
				int ts1 = _translator->translate(grb->body(), true, (grb->recursive() ? TsType::RULE : TsType::EQ));
				vector<int> vi(2);
				vi[0] = ts1;
				vi[1] = ts2;
				grb->type(RT_DISJ);
				grb->body(vi);
			}
			grb->recursive(grb->recursive() || recursive);
			break;
		}
		case RT_AGG: {
			AggGroundRule* grb = dynamic_cast<AggGroundRule*>(it->second);
			int ts1 = _translator->translate(grb->bound(), (grb->lower() ? CompType::LEQ : CompType::GEQ), grb->aggtype(), grb->setnr(),
					(grb->recursive() ? TsType::RULE : TsType::EQ));
			int ts2 = _translator->translate(bound, (lower ? CompType::LEQ : CompType::GEQ), aggtype, setnr, (recursive ? TsType::RULE : TsType::EQ));
			vector<int> vi(2);
			vi[0] = ts1;
			vi[1] = ts2;
			it->second = new PCGroundRule(head, RT_DISJ, vi, (recursive || grb->recursive()));
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
		if (body->type() == RT_AGG) {
			const AggGroundRule* grb = dynamic_cast<const AggGroundRule*>(body);
			s << grb->bound() << (grb->lower() ? " =< " : " >= ");
			s << grb->aggtype() << grb->setnr() << ".\n";
		} else {
			const PCGroundRule* grb = dynamic_cast<const PCGroundRule*>(body);
			char c = grb->type() == RT_CONJ ? '&' : '|';
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
			} else if (grb->type() == RT_CONJ) {
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

bool TsBody::operator<(const TsBody& body) const {
	if (typeid(*this).before(typeid(body))) {
				return true;
			} else if (typeid(body).before(typeid(*this))) {
						return false;
					} else if (type() < body.type()) {
						return true;
					} else {
						return false;
					}
				}

				bool AggTsBody::operator==(const TsBody& body) const {
					if (not (*this == body)) {
						return false;
					}
					auto rhs = dynamic_cast<const AggTsBody&>(body);
					return bound() == rhs.bound() && setnr() == rhs.setnr() && lower() == rhs.lower() && aggtype() == rhs.aggtype();
				}
				bool AggTsBody::operator<(const TsBody& body) const {
					if (TsBody::operator<(body)) {
						return true;
					} else if (TsBody::operator>(body)) {
						return false;
					}
					auto rhs = dynamic_cast<const AggTsBody&>(body);
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
					auto rhs = dynamic_cast<const PCTsBody&>(other);
					return body() == rhs.body() && conj() == rhs.conj();
				}
				bool PCTsBody::operator<(const TsBody& other) const {
					if (TsBody::operator<(other)) {
						return true;
					} else if (TsBody::operator>(other)) {
						return false;
					}
					auto rhs = dynamic_cast<const PCTsBody&>(other);
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

				bool CPTsBody::operator==(const TsBody& body) const {
					if (not TsBody::operator==(body)) {
						return false;
					}
					auto rhs = dynamic_cast<const CPTsBody&>(body);
					return comp() == rhs.comp() && left() == rhs.left() && right() == rhs.right();
				}
				bool CPTsBody::operator<(const TsBody& body) const {
					if (TsBody::operator<(body)) {
						return true;
					} else if (TsBody::operator>(body)) {
						return false;
					}
					auto rhs = dynamic_cast<const CPTsBody&>(body);
					if (comp() < rhs.comp()) {
						return true;
					} else if (comp() > rhs.comp()) {
						return false;
					}
					if (left() < rhs.left()) {
						return true;
					} else if (left() > rhs.left()) {
						return false;
					}
					if (right() < rhs.right()) {
						return true;
					}
					return false;
				}
				/*bool LazyTsBody::operator==(const TsBody& body) const {
				 if (not TsBody::operator==(body)) {
				 return false;
				 }
				 auto rhs = dynamic_cast<const LazyTsBody&>(body);
				 return id_ == rhs.id_ && grounder_ == rhs.grounder_ && (*inst) == (*rhs.inst);
				 }*/
				/*bool LazyTsBody::operator<(const TsBody& body) const {
				 if (TsBody::operator<(body)) {
				 return true;
				 } else if (TsBody::operator>(body)) {
				 return false;
				 }
				 auto rhs = dynamic_cast<const LazyTsBody&>(body);
				 if (id_ < rhs.id_) {
				 return true;
				 } else if (id_ > rhs.id_) {
				 return false;
				 }
				 if (grounder_ < rhs.grounder_) {
				 return true;
				 } else if (grounder_ > rhs.grounder_) {
				 return false;
				 }
				 if ((*inst) == (*rhs.inst)) {
				 return true;
				 }
				 return false;
				 }*/

				bool CPTerm::operator==(const CPTerm& body) const {
					return typeid(*this) == typeid(body);
				}

				bool CPTerm::operator<(const CPTerm& body) const {
					return typeid(*this).before(typeid(body));
						}

						bool CPVarTerm::operator==(const CPTerm& body) const {
							if (not CPTerm::operator==(body)) {
								return false;
							}
							auto rhs = dynamic_cast<const CPVarTerm&>(body);
							return _varid == rhs._varid;
						}
						bool CPVarTerm::operator<(const CPTerm& body) const {
							if (CPTerm::operator<(body)) {
								return true;
							} else if (CPTerm::operator>(body)) {
								return false;
							}
							auto rhs = dynamic_cast<const CPVarTerm&>(body);
							if (_varid < rhs._varid) {
								return true;
							}
							return false;
						}
						bool CPSumTerm::operator==(const CPTerm& body) const {
							if (not CPTerm::operator==(body)) {
								return false;
							}
							auto rhs = dynamic_cast<const CPSumTerm&>(body);
							return _varids == rhs._varids;
						}
						bool CPSumTerm::operator<(const CPTerm& body) const {
							if (CPTerm::operator<(body)) {
								return true;
							} else if (CPTerm::operator>(body)) {
								return false;
							}
							auto rhs = dynamic_cast<const CPSumTerm&>(body);
							if (_varids < rhs._varids) {
								return true;
							}
							return false;
						}
						bool CPWSumTerm::operator==(const CPTerm& body) const {
							if (not CPTerm::operator==(body)) {
								return false;
							}
							auto rhs = dynamic_cast<const CPWSumTerm&>(body);
							return _varids == rhs._varids;
						}
						bool CPWSumTerm::operator<(const CPTerm& body) const {
							if (CPTerm::operator<(body)) {
								return true;
							} else if (CPTerm::operator>(body)) {
								return false;
							}
							auto rhs = dynamic_cast<const CPWSumTerm&>(body);
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
							grounder_->notifyTheoryOccurence(inst);
						}
