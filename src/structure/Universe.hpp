/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef UNIVERSE_HPP_
#define UNIVERSE_HPP_

#include <vector>
#include "TableSize.hpp"
#include "DomainElement.hpp"

class SortTable;

class Universe {
private:
	std::vector<SortTable*> _tables;
public:
	Universe() {
	}
	Universe(const std::vector<SortTable*>& tables)
			: _tables(tables) {
	}
	Universe(const Universe& univ)
			: _tables(univ.tables()) {
	}
	~Universe() {
	}

	const std::vector<SortTable*>& tables() const {
		return _tables;
	}
	void addTable(SortTable* table) {
		_tables.push_back(table);
	}
	unsigned int arity() const {
		return _tables.size();
	}

	bool empty() const;
	bool finite() const;
	bool approxEmpty() const;
	bool approxFinite() const;
	bool contains(const ElementTuple&) const;
	tablesize size() const;
};

#endif /* UNIVERSE_HPP_ */
