/************************************
	StructureVisitor.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "visitors/StructureVisitor.hpp"

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


