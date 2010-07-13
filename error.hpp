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

	/** Invalid ranges **/
	void invalidrange(int n1, int n2, const ParseInfo& pi);
	void invalidrange(char c1, char c2, const ParseInfo& pi);

	/** Invalid tuples **/
	void wrongarity(const ParseInfo& pi);

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

	/** Multiple incompatible declarations of the same object **/
	void multdeclns(const string& nsname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclvoc(const string& vocname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdecltheo(const string& thname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclstruct(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclsort(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclpred(const string& pname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);
	void multdeclfunc(const string& fname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace);

	/** Undeclared objects **/
	void undeclvoc(const string& vocname, const ParseInfo& thisplace);
	void undecltheo(const string& tname, const ParseInfo& thisplace);
	void undeclstruct(const string& sname, const ParseInfo& thisplace);
	void undeclsort(const string& sname, const ParseInfo& thisplace);
	void undeclpred(const string& pname, const ParseInfo& thisplace);
	void undeclfunc(const string& fname, const ParseInfo& thisplace);
	void undeclsymb(const string& name, const ParseInfo& thisplace);

	/** Unavailable objects **/
	void sortnotintheovoc(const string& sname, const string& tname, const ParseInfo& thisplace);
	void prednotintheovoc(const string& pname, const string& tname, const ParseInfo& thisplace);
	void funcnotintheovoc(const string& fname, const string& tname, const ParseInfo& thisplace);

	void symbnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);
	void sortnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);
	void prednotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);
	void funcnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace);

	/** Using overlapping symbols **/
	void doublesortusing(const string& sname, const string& vocname1, const string& vocname2, const ParseInfo& thisplace);
	void doublepredusing(const string& pname, const string& vocname1, const string& vocname2, const ParseInfo& thisplace);
	void doublefuncusing(const string& fname, const string& vocname1, const string& vocname2, const ParseInfo& thisplace);
	void predorfuncsymbol(const string& name, const ParseInfo& thisplace);

	/** Type checking **/
	void novarsort(const string&, const ParseInfo& thisplace);
	void nopredsort(const string&, const ParseInfo& thisplace);
	void nofuncsort(const string&, const ParseInfo& thisplace);
	void wrongsort(const string&, const string&, const string&, const ParseInfo&);

	/** Unknown options or commands **/
	void unkncommand(const string& name, const ParseInfo& thisplace);
	void wrongcommandargs(const string& name, const ParseInfo&  thisplace);
	void ambigcommand(const string& name, const ParseInfo& thisplace);
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

	/** Reading from stdin **/
	void readingfromstdin();

}

namespace Info {

	/** Information **/
	void print(const string& s);
}

#endif
