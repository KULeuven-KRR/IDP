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

#include "UniqueNames.hpp"

class Structure;
class Theory;
class Definition;
class Rule;
class PFSymbol;
class Options;
class Sort;

namespace BootstrappingUtils {
	/**
	 * Extracts metadata concerning definitions from theory.
	 * Format of the metadata: according to the file definition.idp, in data/shara/std,
	 * vocabulary used is basicinfo from that file.
	 */
	Structure* getDefinitionInfo(const Theory* theo, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames, UniqueNames<const Definition*>& uniqueDefNames);
	Structure* getDefinitionInfo(const Theory* theo, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames, UniqueNames<Definition*>& uniqueDefNames);
	Structure* getDefinitionInfo(const Definition* d, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames, UniqueNames<const Definition*>& uniqueDefNames);
	Structure* getDefinitionInfo(Definition* d, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames, UniqueNames<Definition*>& uniqueDefNames);

	/**
	 * Sets the options for running bootstrapping applications. Returns backup of old options.
	 * When calling this method, you are responsible for reverting the options that are now set.
	 */
	Options* setBootstrappingOptions();

	struct MetaRepr {
		Vocabulary* origvoc;
		Structure* metastructure;
		UniqueStringNames<PFSymbol*> symbols;
	};

	MetaRepr toMeta(Theory* theory);
	Theory* fromMeta(MetaRepr meta, Predicate* setOfSentences);
}
