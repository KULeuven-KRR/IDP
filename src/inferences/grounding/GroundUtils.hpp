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

#include "common.hpp"
#include "GroundingContext.hpp"
#include "utils/ListUtils.hpp"

class Variable;
class DomElemContainer;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;
class TsBody;

typedef std::vector<Variable*> varlist;
typedef std::map<Variable*, const DomElemContainer*> var2dommap;

typedef double Weight;
typedef std::vector<Lit> litlist;
typedef std::vector<Weight> weightlist;
typedef std::vector<int> intweightlist;

typedef std::pair<ElementTuple, Lit> Tuple2Atom;
typedef std::pair<Lit, TsBody*> tspair;

#define _true translator()->trueLit()
#define _false translator()->falseLit()

template<typename DomElemList, typename DomInstList>
void overwriteVars(DomElemList& originst, const DomInstList& freevarinst) {
	for (auto var2domelem = freevarinst.cbegin(); var2domelem < freevarinst.cend(); ++var2domelem) {
		originst.push_back(var2domelem->first->get());
		(*var2domelem->first) = var2domelem->second;
	}
}

template<typename DomElemList, typename DomInstList>
void restoreOrigVars(DomElemList& originst, const DomInstList& freevarinst) {
	for (size_t i = 0; i < freevarinst.size(); ++i) {
		(*freevarinst[i].first) = originst[i];
	}
}

class Structure;
class GenerateBDDAccordingToBounds;

// Always non-owning, can always be NULL!
struct StructureInfo{
	Structure* concrstructure;
	std::shared_ptr<GenerateBDDAccordingToBounds> symstructure;
};

bool useLazyGrounding();

bool useUFSAndOnlyIfSem();
