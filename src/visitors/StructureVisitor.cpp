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

#include "StructureVisitor.hpp"

#include "structure/StructureComponents.hpp"

void StructureVisitor::visit(const PredTable* pt) {
	pt->internTable()->accept(this);
}

void StructureVisitor::visit(const FuncTable* ft) {
	ft->internTable()->accept(this);
}

void StructureVisitor::visit(const SortTable* st) {
	st->internTable()->accept(this);
}

