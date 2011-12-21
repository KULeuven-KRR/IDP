/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include "commontypes.hpp"

class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class Options;
class UserProcedure;
class Query;
class Term;

#include "parseinfo.hpp"

#include "GlobalData.hpp"

/*****************
 Namespaces
 *****************/

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
	std::map<std::string, AbstractStructure*> _structures; //!< Map a name to the corresponding structure
	std::map<std::string, AbstractTheory*> _theories; //!< Map a name to the corresponding theory
	std::map<std::string, UserProcedure*> _procedures; //!< Map a name+arity to the corresponding procedure
	std::map<std::string, Query*> _queries; //!< Map a name to the corresponding query
	std::map<std::string, Term*> _terms; //!< Map a name to the corresponding term

	ParseInfo _pi; //!< the place where the namespace was parsed

	std::set<std::string> doubleNames() const;

public:
	Namespace(std::string name, Namespace* super, const ParseInfo& pi) :
			_name(name), _superspace(super), _pi(pi) {
		if (super){
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
	bool isTerm(const std::string&) const;
	bool isStructure(const std::string&) const;
	bool isProc(const std::string&) const;
	Namespace* subspace(const std::string&) const;
	Vocabulary* vocabulary(const std::string&) const;
	AbstractTheory* theory(const std::string&) const;
	AbstractStructure* structure(const std::string&) const;
	UserProcedure* procedure(const std::string&) const;
	Query* query(const std::string&) const;
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
	const std::map<std::string, AbstractStructure*>& structures() const {
		return _structures;
	}
	const std::map<std::string, AbstractTheory*>& theories() const {
		return _theories;
	}
	const std::map<std::string, Query*>& queries() const {
		return _queries;
	}
	const std::map<std::string, Term*>& terms() const {
		return _terms;
	}

	// Mutators
	void add(Vocabulary* v);
	void add(Namespace* n);
	void add(AbstractStructure* s);
	void add(AbstractTheory* t);
	void add(UserProcedure* l);
	void add(const std::string& name, Query*);
	void add(const std::string& name, Term*);

	// Output
	std::ostream& putName(std::ostream&) const;
	std::ostream& putLuaName(std::ostream&) const;
};

#endif
