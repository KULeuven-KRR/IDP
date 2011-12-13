#include <iostream>
#include "parseinfo.hpp"
#include "error.hpp"
#include "GlobalData.hpp"
using namespace std;

namespace Error {
unsigned int nr_of_errors() {
	return GlobalData::instance()->getErrorCount();
}

/** Global error messages **/

void error() {
	GlobalData::instance()->notifyOfError();
	clog << "ERROR: ";
}

void error(const std::string& message) {
	GlobalData::instance()->notifyOfError();
	clog << "ERROR: " << message;
}

void error(const ParseInfo& p) {
	GlobalData::instance()->notifyOfError();
	clog << "ERROR at line " << p.line() << ", column " << p.col();
	if (p.file()) {
		clog << " of file " << *(p.file());
	}
	clog << ": ";
}

/** Command line errors **/

void constnotset(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Constant '" << s << "' should be set at the command line." << "\n";
}

void unknoption(const string& s) {
	error();
	clog << "'" << s << "' is an unknown option." << "\n";
}

void unknoption(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "'" << s << "' is an unknown option." << "\n";
}

void wrongvalue(const string& optname, const string& val, const ParseInfo& pi) {
	error(pi);
	clog << "'" << val << "' is not a valid value for option '" << optname << "'\n";
}

void unknfile(const string& s) {
	error();
	clog << "'" << s << "' is not a valid file name or not readable." << "\n";
}

void constsetexp() {
	error();
	clog << "Constant assignment expected after '-c'." << "\n";
}

void stringconsexp(const string& c, const ParseInfo& pi) {
	error(pi);
	clog << "Command line constant " << c << " should be a string constant.\n";
}

void twicestdin(const ParseInfo& pi) {
	error(pi);
	clog << "stdin can be parsed only once.\n";
}

void nrmodelsnegative() {
	error();
	clog << "Expected a non-negative integer after '-n'" << "\n";
}

/** File errors **/

void cyclicinclude(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "File " << s << " includes itself.\n";
}

void unexistingfile(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Could not open file " << s << ".\n";
}

void unabletoopenfile(const string& s) {
	error();
	clog << "Could not open file " << s << ".\n";
}

/** Invalid ranges **/
void invalidrange(int n1, int n2, const ParseInfo& pi) {
	error(pi);
	clog << n1 << ".." << n2 << " is an invalid range." << "\n";
}

void invalidrange(char c1, char c2, const ParseInfo& pi) {
	error(pi);
	clog << "'" << c1 << ".." << c2 << "' is an invalid range." << "\n";
}

/** Invalid tuples **/
void wrongarity(const ParseInfo& pi) {
	error(pi);
	clog << "The tuples in this table have different lengths" << "\n";
}

void wrongpredarity(const string& p, const ParseInfo& pi) {
	error(pi);
	clog << "Wrong number of terms given to predicate " << p << ".\n";
}

void wrongfuncarity(const string& f, const ParseInfo& pi) {
	error(pi);
	clog << "Wrong number of terms given to function " << f << ".\n";
}

void incompatiblearity(const string& n, const ParseInfo& pi) {
	error(pi);
	clog << "The arity of symbol " << n << " is different from the arity of the table assigned to it.\n";
}

void prednameexpected(const ParseInfo& pi) {
	error(pi);
	clog << "Expected a name of a predicate, instead of a function name.\n";
}

void funcnameexpectedinhead(const ParseInfo& pi) {
	error(pi);
	clog << "The head of a rule can only be a predicate, function = term or term = function.\n";
}

void funcnameexpected(const ParseInfo& pi) {
	error(pi);
	clog << "Expected a name of a function, instead of a predicate name.\n";
}

void funcnotconstr(const string& f, const ParseInfo& pi) {
	error(pi);
	clog << "Function " << f << " is not a constructor.\n";
}

/** Invalid interpretations **/
void expectedutf(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Unexpected '" << s << "', expected 'u', 'ct' or 'cf'\n";
}

void multpredinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for predicate " << s << ".\n";
}

void multfuncinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for function " << s << ".\n";
}

void emptyassign(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "I cannot assign the empty interpretation to symbol " << s << " because all symbols with that name do already have an interpretation."
			<< "\n";
}

void emptyambig(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "I cannot unambiguously assign the empty interpretation to symbol " << s
			<< " because there is more than one symbol with that name and without an interpretation." << "\n";
}

void multunknpredinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for predicate " << s << "[u].\n";
}

void multunknfuncinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for function " << s << "[u].\n";
}

void multctpredinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for predicate " << s << "[ct].\n";
}

void multcfpredinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for predicate " << s << "[cf].\n";
}

void multctfuncinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for function " << s << "[ct].\n";
}

void multcffuncinter(const string& s, const ParseInfo& pi) {
	error(pi);
	clog << "Multiple interpretations for function " << s << "[cf].\n";
}

void threethreepred(const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", an interpretation for predicate " << s << "[ct], " << s << "[cf], and " << s << "[u] is given. \n";
}

void threethreefunc(const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", an interpretation for function " << s << "[ct], " << s << "[cf], and " << s << "[u] is given. \n";
}

void onethreepred(const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", an interpretation for predicate " << s << "[u] is given, but there is no interpretation for " << s
			<< "[ct] or " << s << "[cf].\n";
}

void onethreefunc(const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", an interpretation for function " << s << "[u] is given, but there is no interpretation for " << s
			<< "[ct] or " << s << "[cf].\n";
}

void sortelnotinsort(const string& el, const string& p, const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", element " << el << " occurs in the domain of sort " << p
			<< " but does not belong to the interpretation of sort " << s << ".\n";
}

void predelnotinsort(const string& el, const string& p, const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", element " << el << " occurs in the interpretation of predicate " << p
			<< " but does not belong to the interpretation of sort " << s << ".\n";
}

void funcelnotinsort(const string& el, const string& p, const string& s, const string& str) {
	error();
	clog << "In structure " << str << ", element " << el << " occurs in the interpretation of function " << p
			<< " but does not belong to the interpretation of sort " << s << ".\n";
}

void notfunction(const string& f, const string& str, const vector<string>& el) {
	error();
	clog << "Tuple (";
	if (el.size()) {
		clog << el[0];
		for (unsigned int n = 1; n < el.size(); ++n) {
			clog << "," << el[n];
		}
	}
	clog << ") has more than one image in the interpretation of function " << f << " in structure " << str << ".\n";
}

void nottotal(const string& f, const string& str) {
	error();
	clog << "The interpretation of function " << f << " in structure " << str << " is non-total.\n";
}

void threevalsort(const string&, const ParseInfo& pi) {
	error(pi);
	clog << "Not allowed to assign a three-valued interpretation to a sort.\n";
}

/** Multiple incompatible declarations of the same object **/

void multdeclns(const string& nsname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Namespace " << nsname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

void multdeclvoc(const string& vocname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Vocabulary " << vocname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

void multdecltheo(const string& thname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Theory " << thname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

void multdeclquery(const string& fname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Query " << fname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

void multdeclterm(const string& tname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Term " << tname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) clog << " of file " << *(prevdeclplace.file());
	clog << "." << "\n";
}

void multdeclstruct(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Structure " << sname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

void multdeclopt(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Options " << sname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

void multdeclproc(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
	error(thisplace);
	clog << "Procedure " << sname << " is already declared in this scope" << ", namely at line " << prevdeclplace.line() << ", column "
			<< prevdeclplace.col();
	if (prevdeclplace.file()) {
		clog << " of file " << *(prevdeclplace.file());
	}
	clog << "." << "\n";
}

/** Undeclared objects **/

void undeclvoc(const string& vocname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Vocabulary " << vocname << " is not declared in this scope." << "\n";
}

void undeclopt(const string& optname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Option " << optname << " is not declared in this scope." << "\n";
}

void undecltheo(const string& tname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Theory " << tname << " is not declared in this scope." << "\n";
}

void undeclstruct(const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Structure " << sname << " is not declared in this scope." << "\n";
}

void undeclspace(const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Namespace " << sname << " is not declared in this scope." << "\n";
}

void undeclsort(const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Sort " << sname << " is not declared in this scope." << "\n";
}

void undeclpred(const string& pname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Predicate " << pname << " is not declared in this scope." << "\n";
}

void undeclfunc(const string& fname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Function " << fname << " is not declared in this scope." << "\n";
}

void undeclsymb(const string& name, const ParseInfo& pi) {
	error(pi);
	clog << "Predicate or function " << name << " is not declared in this scope." << "\n";
}

/** Unavailable objects **/

void sortnotintheovoc(const string& sname, const string& tname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Sort " << sname << " is not in the vocabulary of theory " << tname << "." << "\n";
}

void prednotintheovoc(const string& pname, const string& tname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Predicate " << pname << " is not in the vocabulary of theory " << tname << "." << "\n";
}

void funcnotintheovoc(const string& fname, const string& tname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Function " << fname << " is not in the vocabulary of theory " << tname << "." << "\n";
}

void symbnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Symbol " << name << " does not belong to the vocabulary of structrure " << sname << "." << "\n";
}

void sortnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Sort " << name << " does not belong to the vocabulary of structrure " << sname << "." << "\n";
}

void prednotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Predicate " << name << " does not belong to the vocabulary of structrure " << sname << "." << "\n";
}

void funcnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Function " << name << " does not belong to the vocabulary of structrure " << sname << "." << "\n";
}

/** Using overlapping symbols **/
void predorfuncsymbol(const string& name, const ParseInfo& pi) {
	error(pi);
	clog << name << " could be the predicate " << name << "/1 or the function " << name << "/0 at this place.\n";
}

void overloadedsort(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The sort " << name << " used here could be the sort declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the sort declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedfunc(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The function " << name << " used here could be the predicate declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the function declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedpred(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The predicate " << name << " used here could be the predicate declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the predicate declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedspace(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The namespace " << name << " used here could be the namespace declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the namespace declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedstructure(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The structure " << name << " used here could be the structure declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the structure declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedopt(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The options " << name << " used here could be the options declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the options declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedproc(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The procedure " << name << " used here could be the options declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the procedure declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedquery(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The query " << name << " used here could be the query declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the query declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedterm(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The term " << name << " used here could be the term declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) clog << " of file " << p1.file();
	clog << " or the term declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) clog << " of file " << p2.file();
	clog << ".\n";
}

void overloadedtheory(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The theory " << name << " used here could be the theory declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the theory declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

void overloadedvocab(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "The vocabulary " << name << " used here could be the vocabulary declared at " << "line " << p1.line() << ", column " << p1.col();
	if (p1.file()) {
		clog << " of file " << p1.file();
	}
	clog << " or the vocabulary declared at " << "line " << p2.line() << ", column " << p2.col();
	if (p2.file()) {
		clog << " of file " << p2.file();
	}
	clog << ".\n";
}

/** Sort hierarchy errors **/

void notsubsort(const string& c, const string& p, const ParseInfo& pi) {
	error(pi);
	clog << "Sort " << c << " is not a subsort of sort " << p << ".\n";
}

void cyclichierarchy(const string& c, const string& p, const ParseInfo& pi) {
	error(pi);
	clog << "Cycle introduced between sort " << c << " and sort " << p << ".\n";
}

/** Type checking **/

void novarsort(const string& name, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Could not derive the sort of variable " << name << ".\n";
}

void nodomsort(const string& name, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Could not derive the sort of domain element " << name << ".\n";
}

void nopredsort(const string& name, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Could not derive the sorts of predicate " << name << ".\n";
}

void nofuncsort(const string& name, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Could not derive the sorts of function " << name << ".\n";
}

void wrongsort(const string& termname, const string& termsort, const string& possort, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Term " << termname << " has sort " << termsort << " but occurs in a position with sort " << possort << ".\n";
}

/** Unknown commands or options **/

void notcommand() {
	error();
	clog << "Attempt to call a non-function value." << "\n";
}

void unkncommand(const string& name, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Procedure " << name << " does not exist." << "\n";
}

void unkncommand(const string& name) {
	error();
	clog << "Procedure " << name << " does not exist." << "\n";
}

void unknopt(const string& name, ParseInfo* thisplace) {
	thisplace ? error(*thisplace) : error();
	clog << "Options " << name << " does not exist." << "\n";
}

void unkniat(const string& name, const ParseInfo& thisplace) {
	error(thisplace);
	clog << "Argument type " << name << " does not exist." << "\n";
}

void wrongvaluetype(const string& name, ParseInfo* thisplace) {
	thisplace ? error(*thisplace) : error();
	clog << "The value given to option " << name << " is of the wrong type.\n";
}

void wrongformat(const string& format, ParseInfo* thisplace) {
	thisplace ? error(*thisplace) : error();
	clog << format << " is not a valid output language.\n";
}

void wrongmodelformat(const string& format, ParseInfo* thisplace) {
	thisplace ? error(*thisplace) : error();
	clog << format << " is not a valid model format.\n";
}

void posintexpected(const string& name, ParseInfo* thisplace) {
	thisplace ? error(*thisplace) : error();
	clog << "The value given to option " << name << " should be a positive integer.\n";
}

void ambigcommand(const string& name) {
	error();
	clog << "Ambiguous call to overloaded procedure " << name << ".\n";
}

void indexoverloadedfunc() {
	error();
	clog << "Indexing a structure with an overloaded function.\n";
}

void indexoverloadedpred() {
	error();
	clog << "Indexing a structure with an overloaded predicate.\n";
}

void indexoverloadedsort() {
	error();
	clog << "Indexing a structure with an overloaded sort.\n";
}

void threevalcall() {
	error();
	clog << "Calling a three-valued function.\n";
}

void structureexpected(const ParseInfo& pi) {
	error(pi);
	clog << "Expected a structure.\n";
}

void theoryexpected(const ParseInfo& pi) {
	error(pi);
	clog << "Expected a theory.\n";
}

void vocabexpected(const ParseInfo& pi) {
	error(pi);
	clog << "Expected a vocabulary.\n";
}

}

namespace Warning {

/** Data **/
unsigned int warningcounter = 0;

/** Global error messages **/

void warning() {
	warningcounter++;
	clog << "WARNING: ";
}

void warning(const ParseInfo& p) {
	warningcounter++;
	clog << "WARNING at line " << p.line() << ", column " << p.col();
	if (p.file()) {
		clog << " of file " << *(p.file());
	}
	clog << ": ";
}

void cumulchance(double c) {
	warning();
	clog << "Chance of " << c << " is impossible. Using chance 1 instead\n";
}

void possiblyInfiniteGrounding(const std::string& formula, const std::string& internalformula) {
	clog << "Warning: infinite grounding of formula " << formula << "\n";
	clog << "\t Internal representation: " << internalformula << "\n";
}

void triedAddingSubtypeToVocabulary(const std::string& boundedpredname, const std::string& predname, const std::string& vocname) {
	clog << "Warning: tried to add " << boundedpredname << " to " <<vocname <<", instead " <<predname <<" was added to that vocabulary.\n";
}

/** Ambiguous partial term **/
void ambigpartialterm(const string& term, const ParseInfo& thisplace) {
	warning(thisplace);
	clog << "Term " << term << " may lead to an ambiguous meaning of the formula where it occurs.\n";
}

/** Ambiguous statements **/
void varcouldbeconst(const string& name, const ParseInfo& thisplace) {
	warning(thisplace);
	clog << "'" << name << "' could be a variable or a constant. GidL assumes it is a variable.\n";
}

/** Free variables **/
void freevars(const string& fv, const ParseInfo& thisplace) {
	warning(thisplace);
	if (fv.size() > 1) {
		clog << "Variables" << fv << " are not quantified.\n";
	} else {
		clog << "Variable" << fv[0] << " is not quantified.\n";
	}
}

/** Unexpeded type derivation **/
void derivevarsort(const string& varname, const string& sortname, const ParseInfo& thisplace) {
	warning(thisplace);
	clog << "Derived sort " << sortname << " for variable " << varname << ".\n";
}

/** Autocompletion **/
void addingeltosort(const string& elname, const string& sortname, const string& structname) {
	warning();
	clog << "Adding element " << elname << " to the interpretation of sort " << sortname << " in structure " << structname << ".\n";
}

/** Reading from stdin **/
void readingfromstdin() {
	clog << "(Reading from stdin)\n";
}
}

namespace Info {

/** Information **/
void print(const string& s) {
	clog << s << "\n";
}
}
