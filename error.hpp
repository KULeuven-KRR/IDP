/************************************
	error.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ERROR_HPP
#define ERROR_HPP

#include "namespace.hpp"

namespace Error {

	/** Number of errors **/
	unsigned int nr_of_errors();

	/** Global error message **/
	void error();
	void error(const ParseInfo& p);

	/** Command line errors **/
	void constnotset(const string&, const ParseInfo& pi);
	void unknoption(const string&);
	void unknfile(const string&);
	void constsetexp();
	void stringconsexp(const string&, const ParseInfo& pi);
	void twicestdin(const ParseInfo& pi);
	void nrmodelsnegative();

	/** File errors **/
	void cyclicinclude(const string&, const ParseInfo& pi);
	void unexistingfile(const string&, const ParseInfo& pi);
	void unabletoopenfile(const string&);

	/** Invalid ranges **/
	void invalidrange(int n1, int n2, const ParseInfo& pi);
	void invalidrange(char c1, char c2, const ParseInfo& pi);

	/** Invalid tuples **/
	void wrongarity(const ParseInfo& pi);
	void wrongpredarity(const string& p, const ParseInfo& pi);
	void wrongfuncarity(const string& f, const ParseInfo& pi);
	void incompatiblearity(const string& n, const ParseInfo& pi);

	/** Function name where predicate is expected, and vice versa **/
	void prednameexpected(const ParseInfo& pi);
	void funcnameexpected(const ParseInfo& pi);
	void funcnotconstr(const string& s, const ParseInfo& pi);

	/** Invalid interpretations **/
	void multpredinter(const string& s, const ParseInfo& pi);
	void multfuncinter(const string& s, const ParseInfo& pi);
	void emptyassign(const string& s, const ParseInfo& pi);
	void emptyambig(const string& s, const ParseInfo& pi);
	void multunknpredinter(const string& s, const ParseInfo& pi);
	void multctpredinter(const string& s, const ParseInfo& pi);
	void multcfpredinter(const string& s, const ParseInfo& pi);
	void multunknfuncinter(const string& s, const ParseInfo& pi);
	void multctfuncinter(const string& s, const ParseInfo& pi);
	void multcffuncinter(const string& s, const ParseInfo& pi);
	void threethreepred(const string& s, const string& str);
	void threethreefunc(const string& s, const string& str);
	void onethreepred(const string& s, const string& str);
	void onethreefunc(const string& s, const string& str);
	void expectedutf(const string& s, const ParseInfo& pi);
	void sortelnotinsort(const string& el, const string& p, const string& s, const string& str);
	void predelnotinsort(const string& el, const string& p, const string& s, const string& str);
	void funcelnotinsort(const string& el, const string& p, const string& s, const string& str);
	void notfunction(const string& f, const string& str, const vector<string>& el);
	void nottotal(const string& f, const string& str);
	void threevalsort(const string& s, const ParseInfo& pi);

	/** Multiple incompatible declarations of the same object **/
	void multdeclns(const string& nsname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclvoc(const string& vocname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdecltheo(const string& thname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclstruct(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclopt(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclproc(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);

	/** Undeclared objects **/
	void undeclvoc(const string& vocname, const ParseInfo& thisplace);
	void undecltheo(const string& tname, const ParseInfo& thisplace);
	void undeclstruct(const string& sname, const ParseInfo& thisplace);
	void undeclspace(const string& sname, const ParseInfo& thisplace);
	void undeclsort(const string& sname, const ParseInfo& thisplace);
	void undeclpred(const string& pname, const ParseInfo& thisplace);
	void undeclfunc(const string& fname, const ParseInfo& thisplace);
	void undeclsymb(const string& name, const ParseInfo& thisplace);
	void undeclopt(const string& name, const ParseInfo& thisplace);

	/** Unavailable objects **/
	void sortnotintheovoc(const string& sname, const string& tname, const ParseInfo& thisplace);
	void prednotintheovoc(const string& pname, const string& tname, const ParseInfo& thisplace);
	void funcnotintheovoc(const string& fname, const string& tname, const ParseInfo& thisplace);

	void symbnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);
	void sortnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);
	void prednotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);
	void funcnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);

	/** Using overlapping symbols **/
	void predorfuncsymbol(const string& name, const ParseInfo& thisplace);
	void overloadedsort(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedpred(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedspace(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedvocab(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedtheory(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedstructure(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedopt(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);
	void overloadedproc(const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& thisplace);

	/** Sort hierarchy errors **/
	void notsubsort(const string&, const string&, const ParseInfo& pi);
	void cyclichierarchy(const string&, const string&, const ParseInfo& pi);

	/** Type checking **/
	void novarsort(const string&, const ParseInfo& thisplace);
	void nopredsort(const string&, const ParseInfo& thisplace);
	void nofuncsort(const string&, const ParseInfo& thisplace);
	void nodomsort(const string&, const ParseInfo& thisplace);
	void wrongsort(const string&, const string&, const string&, const ParseInfo&);

	/** Unknown options or commands **/
	void notcommand();
	void unkncommand(const string& name, const ParseInfo& thisplace);
	void unkncommand(const string& name);
	void unknopt(const string& name, ParseInfo* thisplace);
	void unkniat(const string& name, const ParseInfo& thisplace);
	void wrongcommandargs(const string& name);
	void wrongvaluetype(const string& name, ParseInfo* thisplace);
	void wrongformat(const string& format, ParseInfo* thisplace);
	void wrongmodelformat(const string& format, ParseInfo* thisplace);
	void posintexpected(const string& name, ParseInfo* thisplace);
	void ambigcommand(const string& name);
}

namespace Warning {

	/** Global warning message **/
	void warning(const ParseInfo& p);

	/** Ambiguous statements **/
	void varcouldbeconst(const string&, const ParseInfo& thisplace);

	/** Free variables **/
	void freevars(const string& fv, const ParseInfo& thisplace);

	/** Unexpeded type derivation **/
	void derivevarsort(const string& varname, const string& sortname, const ParseInfo& thisplace);

	/** Autocompletion **/
	void addingeltosort(const string& elname, const string& sortname,const string& strname);

	/** Reading from stdin **/
	void readingfromstdin();

}

namespace Info {

	/** Information **/
	void print(const string& s);
}

#endif
