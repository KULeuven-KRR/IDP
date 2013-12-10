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

#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <vector>
#include <set>
#include "common.hpp"

class ParseInfo;

namespace Error {

/** Number of errors **/
unsigned int nr_of_errors();

/** Global error message **/
void error(const std::string& message);
void error(const std::string& message, const ParseInfo& p);

/** Command line errors **/
void unknoption(const std::string&);
void unknoption(const std::string&, const ParseInfo& pi);
void constsetexp();
void stringconsexp(const std::string&, const ParseInfo& pi);
void twicestdin(const ParseInfo& pi);

/** File errors **/
void cyclicinclude(const std::string&, const ParseInfo& pi);
void unexistingfile(const std::string&);
void unexistingfile(const std::string&, const ParseInfo& pi);

/** Invalid ranges **/
void invalidrange(int n1, int n2, const ParseInfo& pi);
void invalidrange(char c1, char c2, const ParseInfo& pi);

/** Invalid tuples **/
void wrongarity(const ParseInfo& pi);
void incompatiblearity(const std::string& n, int symbolarity, int tablearity, const ParseInfo& pi);

/** Function name where predicate is expected, and vice versa **/
void prednameexpected(const ParseInfo& pi);
void funcnameexpected(const ParseInfo& pi);

/** Invalid interpretations **/
void expectedutf(const std::string& s, const ParseInfo& pi);
void sortelnotinsort(const std::string& el, const std::string& p, const std::string& s, const std::string& str);
void predelnotinsort(const std::string& el, const std::string& p, const std::string& s, const std::string& str);
void funcelnotinsort(const std::string& el, const std::string& p, const std::string& s, const std::string& str);
void notfunction(const std::string& f, const std::string& str, const std::vector<std::string>& el);
void nottotal(const std::string& f, const std::string& str);
void threevalsort(const std::string& s, const ParseInfo& pi);

enum class ComponentType {
	Namespace, Vocabulary, Theory, Structure, Query, Term, Procedure, Predicate, Function, Symbol, Sort, Variable, FOBDD
};

std::ostream& operator<<(std::ostream& stream, ComponentType t);

/** Multiple incompatible declarations of the same object **/
void declaredEarlier(ComponentType type, const std::string& name, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);

/** Undeclared objects **/
void notDeclared(ComponentType type, const std::string& name, const ParseInfo& thisplace);

/** Unavailable objects **/
void notInVocabularyOf(ComponentType type, ComponentType parentType, const std::string& sname, const std::string& tname, const ParseInfo& thisplace);

/** Some types have a fixed interpretation that can not be changed in a structure **/
void fixedInterTypeReinterpretedInStructure(ComponentType type, const std::string& name, const ParseInfo& pi);

/** Constructed types can not be a supersort **/
void constructedTypeAsSubtype(ComponentType type, const std::string& name, const ParseInfo& pi);

/** Using overlapping symbols **/
template<class List>
void overloaded(ComponentType type, const std::string& name, const List& possiblelocations, const ParseInfo& pi) {
	if(possiblelocations.size()==0){
		notDeclared(type, name, pi);
		return;
	}
	std::stringstream ss;
	ss << "The " << type << " " << name << " used here should be disambiguated as it might refer to :\n";
	for (auto info : possiblelocations) {
		ss << "\tThe " << type << " at " << toString(info) << "\n";
	}
	error(ss.str(), pi);
}

/** Sort hierarchy errors **/
void notsubsort(const std::string&, const std::string&, const ParseInfo& pi);
void cyclichierarchy(const std::string&, const std::string&, const ParseInfo& pi);

/** Free variables **/
void freevars(const std::string& fv, const ParseInfo& thisplace);

/** Type checking **/
void novarsort(const std::string&, const ParseInfo& thisplace);
void nopredsort(const std::string&, const ParseInfo& thisplace);
void nofuncsort(const std::string&, const ParseInfo& thisplace);
void nodomsort(const std::string&, const ParseInfo& thisplace);
void wrongsort(const std::string&, const std::string&, const std::string&, const ParseInfo&);

/** Unknown options or commands **/
void unknopt(const std::string& name, ParseInfo* thisplace);
void ambigcommand(const std::string& name);
void wrongvalue(const std::string& option, const std::string& value, const ParseInfo&);

void expected(ComponentType type, const ParseInfo&);

/** Errors concerning progression and LTC **/
namespace LTC {
/**All LTC errors have in common that they require the program to STOP after an error is encountered instead of simply notifying the global data*/
void error(const std::string& message);
void error(const std::string& message, const ParseInfo& p);
void defineStaticInTermsOfDynamic(const ParseInfo&);
void timeStratificationViolated(const ParseInfo&);
void containsStartAndNext(const ParseInfo&);
void containsStartAndOther(const ParseInfo&);
void invalidTimeTerm(const ParseInfo&);
void multipleTimeVars(const std::string& var1, const std::string& var2, const ParseInfo&);
void wrongTimeQuantification(const std::string& var, const ParseInfo&);
void nonTopLevelTimeVar(const std::string& var, const ParseInfo&);
void unexpectedTimeTerm(const std::string& term, const ParseInfo&);
void invarContainsStart(const ParseInfo&);
void invarContainsNext(const ParseInfo&);
void invarContainsDefinitions(const ParseInfo&);
void invarIsStatic(const ParseInfo&);
void invarVocIsNotTheoVoc();
void strucVocIsNotTheoVoc();
void notInitialised();
void progressOverWrongVocabulary(const std::string& expectedVoc, const std::string& realVoc);

}
}

namespace Warning {

/** Number of warnings **/
unsigned int nr_of_warnings();

/** Global warning message **/
void warning(const std::string& message, const ParseInfo& p);
void warning(const std::string& message);

/** Ambiguous statements **/
void varcouldbeconst(const std::string&, const ParseInfo& thisplace);

/** Free variables **/
void freevars(const std::string& fv, const ParseInfo& thisplace);

/** Unused variables **/
void unusedvar(const std::string& uv, const ParseInfo& pi);

/** Unexpeded type derivation **/
void derivevarsort(const std::string& varname, const std::string& sortname, const ParseInfo& thisplace);

/** Introduced variable **/
void introducedvar(const std::string& varname, const std::string& sortname, const std::string& term);

/** Ambiguous occurrence of a partial term **/
void ambigpartialterm(const std::string& term, const ParseInfo& thisplace);

/** Autocompletion **/
void addingeltosort(const std::string& elname, const std::string& sortname, const std::string& strname);

/** Reading from stdin **/
void readingfromstdin();

/** Wrong cumulative chance (due to rounding errors?) **/
void cumulchance(double c);

void possiblyInfiniteGrounding(const std::string& formula);

void triedAddingSubtypeToVocabulary(const std::string& boundedpredname, const std::string& predname, const std::string& vocname);

void emptySort(const std::string& sortname, const std::string& structurename);

/** Warn that ASP queries do not have a specific handling */
void aspQueriesAreParsedAsFacts();

/** Domain element can be interpreted as constructor **/
void constructorDisambiguationInStructure(const std::string& name);

}

#endif
