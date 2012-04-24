/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "AbstractStructure.hpp"
#include "printers/idpprinter.hpp"

void AbstractStructure::put(std::ostream& s) const {
	auto p = IDPPrinter<std::ostream>(s);
	p.startTheory();
	p.visit(this);
	p.endTheory();
}
