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

#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include "commontypes.hpp"

class Vocabulary;
class Structure;
class AbstractTheory;
class Options;
class UserProcedure;
class Query;
class Term;
class FOBDD;

#include "parseinfo.hpp"
#include "GlobalData.hpp"

/**************
 * Namespaces
 **************/

/**
 * Class to represent namespaces
 */
class Namespace {
private:
	std::string _name; //!< The name of the namespace
	Namespace* _superspace; //!< The parent in the namespace hierarchy
							//!< Null-pointer if top of the hierarchy
	std::map<std::string, Namespace*> _subspaces; //!< Map a name to the corresponding subspace
	std::map<std::string, Vocabulary*> _vocabularies; //!< Map a name to the corresponding vocabulary
	std::map<std::string, Structure*> _structures; //!< Map a name to the corresponding structure
	std::map<std::string, AbstractTheory*> _theories; //!< Map a name to the corresponding theory
	std::map<std::string, UserProcedure*> _procedures; //!< Map a name+arity to the corresponding procedure
	std::map<std::string, Query*> _queries; //!< Map a name to the corresponding query
	std::map<std::string, Term*> _terms; //!< Map a name to the corresponding term
	std::map<std::string, const FOBDD*> _fobdds; //!< Map a name to the corresponding fobdd

	ParseInfo _pi; //!< the place where the namespace was parsed

	std::set<std::string> doubleNames() const;

public:
	Namespace(std::string name, Namespace* super, const ParseInfo& pi)
			: 	_name(name),
				_superspace(super),
				_pi(pi) {
		if (super) {
			super->add(this);
		}
	}

	~Namespace();

	static Namespace* createGlobal();

	const std::string& name() const {
		return _name;
	}
	Namespace* super() const {
		return _superspace;
	}
	bool hasParent(Namespace* ns) const {
		if (super() == ns) {
			return true;
		}
		if (super() == NULL) {
			return false;
		}
		return super()->hasParent(ns);
	}
	const ParseInfo& pi() const {
		return _pi;
	}
	bool isGlobal() const {
		return this == GlobalData::instance()->getGlobalNamespace();
	}
	bool isSubspace(const std::string&) const;
	bool isVocab(const std::string&) const;
	bool isTheory(const std::string&) const;
	bool isQuery(const std::string&) const;
	bool isFOBDD(const std::string&) const;
	bool isTerm(const std::string&) const;
	bool isStructure(const std::string&) const;
	bool isProc(const std::string&) const;
	Namespace* subspace(const std::string&) const;
	Vocabulary* vocabulary(const std::string&) const;
	AbstractTheory* theory(const std::string&) const;
	Structure* structure(const std::string&) const;
	UserProcedure* procedure(const std::string&) const;
	Query* query(const std::string&) const;
	const FOBDD* fobdd(const std::string&) const;
	Term* term(const std::string&) const;

	const std::map<std::string, UserProcedure*>& procedures() const {
		return _procedures;
	}
	const std::map<std::string, Namespace*>& subspaces() const {
		return _subspaces;
	}
	const std::map<std::string, Vocabulary*>& vocabularies() const {
		return _vocabularies;
	}
	const std::map<std::string, Structure*>& structures() const {
		return _structures;
	}
	const std::map<std::string, AbstractTheory*>& theories() const {
		return _theories;
	}
	const std::map<std::string, Query*>& queries() const {
		return _queries;
	}
	const std::map<std::string, const FOBDD*>& fobdds() const {
		return _fobdds;
	}
	const std::map<std::string, Term*>& terms() const {
		return _terms;
	}

	// TODO NOTE: update when adding more blocks!
	std::vector<std::string> getComponentNamesExceptSpaces() const;

//	enum class ComponentType{ VOC, THEORY, STRUCT, NS, QUERY, TERM, PROC};
//
//	class SpecificationComponent{
//		std::string _name;
//		ComponentType _type;
//		Namespace* _namespace;
//		AbstractTheory* _theory;
//		Structure* _structure;
//		Vocabulary* _vocabulary;
//		Query* _query;
//		Term* _term;
//		UserProcedure* _procedure;
//
//		std::ostream& putLuaName(std::ostream& stream) const{
//			switch(_type){
//			case ComponentType::VOC:
//				_vocabulary->putLuaName(stream);
//				break;
//			case ComponentType::THEORY:
//				_theory->putLuaName(stream);
//				break;
//			case ComponentType::STRUCT:
//				_structure->putLuaName(stream);
//				break;
//			case ComponentType::NS:
//				_namespace->putLuaName(stream);
//				break;
//			case ComponentType::QUERY:
//				_query->putLuaName(stream);
//				break;
//			case ComponentType::TERM:
//				_term->putLuaName(stream);
//				break;
//			case ComponentType::PROC:
//				_procedure->putLuaName(stream);
//				break;
//			}
//		}
//	};

	// Mutators
	void add(Vocabulary* v);
	void add(Namespace* n);
	void add(Structure* s);
	void add(AbstractTheory* t);
	void add(UserProcedure* l);
	void add(const std::string& name, Query*);
	void add(const std::string& name, const FOBDD*);
	void add(const std::string& name, Term*);

	// Output
	std::ostream& putName(std::ostream&) const;
	std::ostream& putLuaName(std::ostream&) const;
};

#endif
