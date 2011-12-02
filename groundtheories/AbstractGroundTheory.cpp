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
