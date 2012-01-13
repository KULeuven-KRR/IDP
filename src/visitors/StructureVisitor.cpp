/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "StructureVisitor.hpp"

#include "structure.hpp"

void StructureVisitor::visit(const PredTable* pt) {
	pt->internTable()->accept(this);
}

void StructureVisitor::visit(const FuncTable* ft) {
	ft->internTable()->accept(this);
}

void StructureVisitor::visit(const SortTable* st) {
	st->internTable()->accept(this);
}

