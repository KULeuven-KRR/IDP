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

#include "AbstractStructure.hpp"
#include "printers/idpprinter.hpp"

AbstractStructure::~AbstractStructure() {
	changeVocabulary(NULL);
}

void AbstractStructure::put(std::ostream& s) const {
	auto p = IDPPrinter<std::ostream>(s);
	p.startTheory();
	p.visit(this);
	p.endTheory();
}

void AbstractStructure::changeVocabulary(Vocabulary* v) {
	if (v != _vocabulary) {
		if (_vocabulary != NULL) {
			_vocabulary->removeStructure(this);
		}
		_vocabulary = v;
		if (_vocabulary != NULL) {
			_vocabulary->addStructure(this);
		}
	}
}
