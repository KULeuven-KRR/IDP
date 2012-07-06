/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#ifndef FORMULAQUERY_HPP_
#define FORMULAQUERY_HPP_

/**
 * Represents queries of the form {x1 ... xn: phi}
 */
class Query {
private:
	std::vector<Variable*> _variables; //!< The free variables of the query. The order of the variables is the
									   //!< order in which they were parsed.
	Formula* _query; //!< The actual query.
	ParseInfo _pi; //!< The place where the query was parsed.
	std::string _name; //!< the name of the query
	Vocabulary* _vocabulary; //!< the vocabulary of the query

public:
	Query(std::string name, const std::vector<Variable*>& vars, Formula* q, const ParseInfo& pi)
			: 	_variables(vars),
				_query(q),
				_pi(pi),
				_name(name),
				_vocabulary(NULL) {
	}

	~Query() {
	}

	void vocabulary(Vocabulary* v) {
		_vocabulary = v;
	}

	Vocabulary* vocabulary() const {
		return _vocabulary;
	}

	std::string name() const {
		return _name;
	}

	void recursiveDelete() {
		_query->recursiveDelete(); //also deletes the variables.
	}

	Formula* query() const {
		return _query;
	}
	const std::vector<Variable*>& variables() const {
		return _variables;
	}
	const ParseInfo& pi() const {
		return _pi;
	}

};

#endif /* FORMULAQUERY_HPP_ */
