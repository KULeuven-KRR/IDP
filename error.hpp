/************************************
	error.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ERROR_H
#define ERROR_H

#include "namespace.hpp"

namespace Error {

	/** Number of errors **/
	unsigned int nr_of_errors();

	/** Global error message **/
	void error();
	void error(ParseInfo* p);

	/** Command line errors **/
	void constnotset(const string&, ParseInfo* pi);
	void unknoption(const string&);
	void unknfile(const string&);
	void constsetexp();
	void stringconsexp(const string&, ParseInfo* pi);
	void twicestdin(ParseInfo* pi);

	/** File errors **/
	void cyclicinclude(const string&, ParseInfo* pi);
	void unexistingfile(const string&, ParseInfo* pi);

	/** Invalid ranges **/
	void invalidrange(int n1, int n2, ParseInfo* pi);
	void invalidrange(char c1, char c2, ParseInfo* pi);

	/** Invalid tuples **/
	void wrongarity(ParseInfo* pi);

	/** Invalid interpretations **/
	void multpredinter(const string& s, ParseInfo* pi);
	void multfuncinter(const string& s, ParseInfo* pi);
	void emptyassign(const string& s, ParseInfo* pi);
	void emptyambig(const string& s, ParseInfo* pi);
	void multunknpredinter(const string& s, ParseInfo* pi);
	void multctpredinter(const string& s, ParseInfo* pi);
	void multcfpredinter(const string& s, ParseInfo* pi);
	void multunknfuncinter(const string& s, ParseInfo* pi);
	void multctfuncinter(const string& s, ParseInfo* pi);
	void multcffuncinter(const string& s, ParseInfo* pi);
	void threethreepred(const string& s, const string& str);
	void threethreefunc(const string& s, const string& str);
	void onethreepred(const string& s, const string& str);
	void onethreefunc(const string& s, const string& str);
	void expectedutf(const string& s, ParseInfo* pi);
	void sortelnotinsort(const string& el, const string& p, const string& s, const string& str);
	void predelnotinsort(const string& el, const string& p, const string& s, const string& str);
	void funcelnotinsort(const string& el, const string& p, const string& s, const string& str);
	void notfunction(const string& f, const string& str, const vector<string>& el);
	void nottotal(const string& f, const string& str);

	/** Multiple incompatible declarations of the same object **/
	void multdeclns(const string& nsname, ParseInfo* thisplace, ParseInfo* prevdeclplace);
	void multdeclvoc(const string& vocname, ParseInfo* thisplace, ParseInfo* prevdeclplace);
	void multdecltheo(const string& thname, ParseInfo* thisplace, ParseInfo* prevdeclplace);
	void multdeclstruct(const string& sname, ParseInfo* thisplace, ParseInfo* prevdeclplace);
	void multdeclsort(const string& sname, ParseInfo* thisplace, ParseInfo* prevdeclplace);
	void multdeclpred(const string& pname, ParseInfo* thisplace, ParseInfo* prevdeclplace);
	void multdeclfunc(const string& fname, ParseInfo* thisplace, ParseInfo* prevdeclplace);

	/** Undeclared objects **/
	void undeclvoc(const string& vocname, ParseInfo* thisplace);
	void undecltheo(const string& tname, ParseInfo* thisplace);
	void undeclstruct(const string& sname, ParseInfo* thisplace);
	void undeclsort(const string& sname, ParseInfo* thisplace);
	void undeclpred(const string& pname, ParseInfo* thisplace);
	void undeclfunc(const string& fname, ParseInfo* thisplace);
	void undeclsymb(const string& name, ParseInfo* thisplace);

	/** Unavailable objects **/
	void sortnotintheovoc(const string& sname, const string& tname, ParseInfo* thisplace);
	void prednotintheovoc(const string& pname, const string& tname, ParseInfo* thisplace);
	void funcnotintheovoc(const string& fname, const string& tname, ParseInfo* thisplace);
	void sortnotintheostruct(const string& sname, const string& strname, ParseInfo* thisplace);
	void notheostruct(ParseInfo* thisplace);

	void symbnotinstructvoc(const string& name, const string& sname, ParseInfo* thisplace);
	void sortnotinstructvoc(const string& name, const string& sname, ParseInfo* thisplace);
	void prednotinstructvoc(const string& name, const string& sname, ParseInfo* thisplace);
	void funcnotinstructvoc(const string& name, const string& sname, ParseInfo* thisplace);

	/** Using overlapping symbols **/
	void doublesortusing(const string& sname, const string& vocname1, const string& vocname2, ParseInfo* thisplace);
	void doublepredusing(const string& pname, const string& vocname1, const string& vocname2, ParseInfo* thisplace);
	void doublefuncusing(const string& fname, const string& vocname1, const string& vocname2, ParseInfo* thisplace);
	void predorfuncsymbol(const string& name, ParseInfo* thisplace);

	/** Type checking **/
	void novarsort(const string&, ParseInfo* thisplace);
	void nopredsort(const string&, ParseInfo* thisplace);
	void nofuncsort(const string&, ParseInfo* thisplace);
	void wrongsort(const string&, const string&, const string&, ParseInfo*);

	/** Unknown options or commands **/
	void unkncommand(const string& name, ParseInfo* thisplace);
	void wrongcommandargs(const string& name, ParseInfo*  thisplace);
	void ambigcommand(const string& name, ParseInfo* thisplace);
}

namespace Warning {

	/** Global warning message **/
	void warning(ParseInfo* p);

	/** Ambiguous statements **/
	void varcouldbeconst(const string&, ParseInfo* thisplace);

	/** Free variables **/
	void freevars(const string& fv, ParseInfo* thisplace);

	/** Unexpeded type derivation **/
	void derivevarsort(const string& varname, const string& sortname, ParseInfo* thisplace);

	/** Reading from stdin **/
	void readingfromstdin();

}

namespace Info {

	/** Information **/
	void print(const string& s);
}

#endif
