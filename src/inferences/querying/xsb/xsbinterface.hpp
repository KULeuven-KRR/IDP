#pragma once

#include <list>
#include <set>

#include "compiler.hpp"
#include "theory/theory.hpp"
#include "structure/StructureComponents.hpp"

class XSBInterface {
private:
	PrologProgram* _pp;
	AbstractStructure* _structure;
	void loadOpenSymbols();
	void loadInterpretation(PFSymbol*);
	void sendToXSB(std::string, bool);
	XSBInterface();
public:
	static XSBInterface* instance();
	void setStructure(AbstractStructure* structure);
	void reset();
	void exit();
	void loadDefinition(Definition*);
	SortedElementTable queryDefinition(PFSymbol*);
	bool query(PFSymbol*, ElementTuple);
};

ElementTuple atom2tuple(PredForm* pf, AbstractStructure* s);
