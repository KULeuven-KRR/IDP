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

#include "AbstractGroundTheory.hpp"
#include "IncludeComponents.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

AbstractGroundTheory::AbstractGroundTheory(StructureInfo structure)
		: AbstractTheory("", ParseInfo()), _structure(NULL), _translator(NULL) {
	if(structure.concrstructure!=NULL){
		_structure = structure.concrstructure->clone();
	}
	_translator = new GroundTranslator(structure, this);
}

AbstractGroundTheory::AbstractGroundTheory(Vocabulary* voc, StructureInfo structure)
		: AbstractTheory("", voc, ParseInfo()), _structure(NULL), _translator(NULL) {
	if(structure.concrstructure!=NULL){
		_structure = structure.concrstructure->clone();
	}
	_translator = new GroundTranslator(structure, this);
}

void AbstractGroundTheory::initializeTheory(){
	_translator->initialize();
}

AbstractGroundTheory::~AbstractGroundTheory() {
	delete (_structure);
	delete (_translator);
}

void AbstractGroundTheory::addEmptyClause() {
	add(GroundClause { });
}
void AbstractGroundTheory::addUnitClause(Lit l) {
	add(GroundClause { l });
}
AbstractGroundTheory* AbstractGroundTheory::clone() const {
	throw notyetimplemented("Cloning ground theories.\n");
}
