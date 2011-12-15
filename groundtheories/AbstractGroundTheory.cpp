/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "AbstractGroundTheory.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

AbstractGroundTheory::AbstractGroundTheory(AbstractStructure* str) :
		AbstractTheory("", ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator(str)) {
}

AbstractGroundTheory::AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str) :
		AbstractTheory("", voc, ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator(str)) {
}

AbstractGroundTheory::~AbstractGroundTheory() {
	delete (_structure);
	delete (_translator);
	delete (_termtranslator);
}

void AbstractGroundTheory::addEmptyClause() {
	add(GroundClause { });
}
void AbstractGroundTheory::addUnitClause(Lit l) {
	add(GroundClause { l });
}
AbstractGroundTheory* AbstractGroundTheory::clone() const {
	Assert(false);
	return NULL;/* TODO */
}
