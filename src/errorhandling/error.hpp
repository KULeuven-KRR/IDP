/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <vector>

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
void incompatiblearity(const std::string& n, const ParseInfo& pi);

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
	Namespace, Vocabulary, Theory, Structure, Query, Term, Procedure, Predicate, Function, Symbol, Sort
};

/** Multiple incompatible declarations of the same object **/
void declaredEarlier(ComponentType type, const std::string& name, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);

/** Undeclared objects **/
void notDeclared(ComponentType type, const std::string& name, const ParseInfo& thisplace);

/** Unavailable objects **/
void notInVocabularyOf(ComponentType type, ComponentType parentType, const std::string& sname, const std::string& tname, const ParseInfo& thisplace);

/** Using overlapping symbols **/
void overloaded(ComponentType type, const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);

/** Sort hierarchy errors **/
void notsubsort(const std::string&, const std::string&, const ParseInfo& pi);
void cyclichierarchy(const std::string&, const std::string&, const ParseInfo& pi);

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

void emptySort(const std::string& sortname);

}

#endif
