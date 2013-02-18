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

#pragma once

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
	Universe(const std::vector<SortTable*>& tables);
	Universe(const Universe& univ);

	~Universe() {
	}

	const std::vector<SortTable*>& tables() const {
		return _tables;
	}
	void addTable(SortTable* table);
	unsigned int arity() const {
		return _tables.size();
	}

	bool empty() const;
	bool finite() const;
	bool approxEmpty() const;
	bool approxFinite() const;
	bool contains(const ElementTuple&) const;
	tablesize size() const;

	void put(std::ostream& stream) const;
};
