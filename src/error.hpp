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
void error();
void error(const std::string& message);
void error(const ParseInfo& p);

/** Command line errors **/
void constnotset(const std::string&, const ParseInfo& pi);
void unknoption(const std::string&);
void unknoption(const std::string&, const ParseInfo& pi);
void unknfile(const std::string&);
void constsetexp();
void stringconsexp(const std::string&, const ParseInfo& pi);
void twicestdin(const ParseInfo& pi);
void nbmodelsnegative();

/** File errors **/
void cyclicinclude(const std::string&, const ParseInfo& pi);
void unexistingfile(const std::string&, const ParseInfo& pi);
void unabletoopenfile(const std::string&);

/** Invalid ranges **/
void invalidrange(int n1, int n2, const ParseInfo& pi);
void invalidrange(char c1, char c2, const ParseInfo& pi);

/** Invalid tuples **/
void wrongarity(const ParseInfo& pi);
void wrongpredarity(const std::string& p, const ParseInfo& pi);
void wrongfuncarity(const std::string& f, const ParseInfo& pi);
void incompatiblearity(const std::string& n, const ParseInfo& pi);

/** Function name where predicate is expected, and vice versa **/
void prednameexpected(const ParseInfo& pi);
void funcnameexpected(const ParseInfo& pi);
void funcnotconstr(const std::string& s, const ParseInfo& pi);

void funcnameexpectedinhead(const ParseInfo& pi);

/** Invalid interpretations **/
void multpredinter(const std::string& s, const ParseInfo& pi);
void multfuncinter(const std::string& s, const ParseInfo& pi);
void emptyassign(const std::string& s, const ParseInfo& pi);
void emptyambig(const std::string& s, const ParseInfo& pi);
void multunknpredinter(const std::string& s, const ParseInfo& pi);
void multctpredinter(const std::string& s, const ParseInfo& pi);
void multcfpredinter(const std::string& s, const ParseInfo& pi);
void multunknfuncinter(const std::string& s, const ParseInfo& pi);
void multctfuncinter(const std::string& s, const ParseInfo& pi);
void multcffuncinter(const std::string& s, const ParseInfo& pi);
void threethreepred(const std::string& s, const std::string& str);
void threethreefunc(const std::string& s, const std::string& str);
void onethreepred(const std::string& s, const std::string& str);
void onethreefunc(const std::string& s, const std::string& str);
void expectedutf(const std::string& s, const ParseInfo& pi);
void sortelnotinsort(const std::string& el, const std::string& p, const std::string& s, const std::string& str);
void predelnotinsort(const std::string& el, const std::string& p, const std::string& s, const std::string& str);
void funcelnotinsort(const std::string& el, const std::string& p, const std::string& s, const std::string& str);
void notfunction(const std::string& f, const std::string& str, const std::vector<std::string>& el);
void nottotal(const std::string& f, const std::string& str);
void threevalsort(const std::string& s, const ParseInfo& pi);

/** Multiple incompatible declarations of the same object **/
void multdeclns(const std::string& nsname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdeclvoc(const std::string& vocname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdecltheo(const std::string& thname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdeclquery(const std::string& fname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdeclterm(const std::string& fname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdeclstruct(const std::string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdeclopt(const std::string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
void multdeclproc(const std::string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);

/** Undeclared objects **/
void undeclvoc(const std::string& vocname, const ParseInfo& thisplace);
void undecltheo(const std::string& tname, const ParseInfo& thisplace);
void undeclstruct(const std::string& sname, const ParseInfo& thisplace);
void undeclspace(const std::string& sname, const ParseInfo& thisplace);
void undeclsort(const std::string& sname, const ParseInfo& thisplace);
void undeclpred(const std::string& pname, const ParseInfo& thisplace);
void undeclfunc(const std::string& fname, const ParseInfo& thisplace);
void undeclsymb(const std::string& name, const ParseInfo& thisplace);
void undeclopt(const std::string& name, const ParseInfo& thisplace);

/** Unavailable objects **/
void sortnotintheovoc(const std::string& sname, const std::string& tname, const ParseInfo& thisplace);
void prednotintheovoc(const std::string& pname, const std::string& tname, const ParseInfo& thisplace);
void funcnotintheovoc(const std::string& fname, const std::string& tname, const ParseInfo& thisplace);

void symbnotinstructvoc(const std::string& name, const std::string& sname, const ParseInfo& thisplace);
void sortnotinstructvoc(const std::string& name, const std::string& sname, const ParseInfo& thisplace);
void prednotinstructvoc(const std::string& name, const std::string& sname, const ParseInfo& thisplace);
void funcnotinstructvoc(const std::string& name, const std::string& sname, const ParseInfo& thisplace);

/** Using overlapping symbols **/
void predorfuncsymbol(const std::string& name, const ParseInfo& thisplace);
void overloadedsort(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedpred(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedfunc(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedspace(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedvocab(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedtheory(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedquery(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedterm(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedstructure(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedopt(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
void overloadedproc(const std::string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);

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
void notcommand();
void unkncommand(const std::string& name, const ParseInfo& thisplace);
void unkncommand(const std::string& name);
void unknopt(const std::string& name, ParseInfo* thisplace);
void unkniat(const std::string& name, const ParseInfo& thisplace);
void wrongcommandargs(const std::string& name);
void wrongvaluetype(const std::string& name, ParseInfo* thisplace);
void wrongformat(const std::string& format, ParseInfo* thisplace);
void wrongmodelformat(const std::string& format, ParseInfo* thisplace);
void posintexpected(const std::string& name, ParseInfo* thisplace);
void ambigcommand(const std::string& name);
void wrongvalue(const std::string&, const std::string&, const ParseInfo&);

void indexoverloadedfunc();
void indexoverloadedpred();
void indexoverloadedsort();
void threevalcall();

void vocabexpected(const ParseInfo&);
void theoryexpected(const ParseInfo&);
void structureexpected(const ParseInfo&);
}

namespace Warning {

/** Global warning message **/
void warning(const ParseInfo& p);
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

void possiblyInfiniteGrounding(const std::string& formula, const std::string& internalformula);

void triedAddingSubtypeToVocabulary(const std::string& boundedpredname, const std::string& predname, const std::string& vocname);

}

namespace Info {

/** Information **/
void print(const std::string& s);
}

#endif
