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

#include "structure/MainStructureComponents.hpp"
#include "commontypes.hpp"

class Structure;
class Definition;
class PrologProgram;
class PrologTerm;
class PFSymbol;
class PredForm;
class XSBToIDPTranslator;

class XSBInterface {
private:
	PrologProgram* _pp;
	Structure* _structure;
	XSBToIDPTranslator* _translator;

	XSBInterface();
	void loadOpenSymbols();
	void loadInterpretation(PFSymbol*);
	void sendToXSB(std::string);
	void commandCall(const std::string& command);
	void handleResult(int xsb_status);
	PrologTerm* symbol2term(const PFSymbol*);

public:
	static XSBInterface* instance();
	void setStructure(Structure* structure);
	void reset();
	void exit();
	void loadDefinition(Definition*);
	SortedElementTable queryDefinition(PFSymbol*, TruthValue tv = TruthValue::True);
	bool query(PFSymbol*, ElementTuple);
	PrologTerm* atom2term(PredForm* pf);
	PrologTerm* atom2term(PFSymbol* symbol, ElementTuple el);
};
