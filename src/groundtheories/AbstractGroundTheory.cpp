/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "AbstractGroundTheory.hpp"
#include "IncludeComponents.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

AbstractGroundTheory::AbstractGroundTheory(AbstractStructure const * const str)
		: AbstractTheory("", ParseInfo()), _structure(NULL), _translator(NULL) {
	if(str!=NULL){
		_structure = str->clone();
	}
	_translator = new GroundTranslator(_structure);
}

AbstractGroundTheory::AbstractGroundTheory(Vocabulary* voc, AbstractStructure const * const str)
		: AbstractTheory("", voc, ParseInfo()), _structure(NULL), _translator(NULL) {
	if(str!=NULL){
		_structure = str->clone();
	}
	_translator = new GroundTranslator(_structure);
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
