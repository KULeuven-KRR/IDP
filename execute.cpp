/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <set>
#include <iostream>
#include "lua.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "namespace.hpp"
#include "execute.hpp"
#include "error.hpp"
#include "print.hpp"
#include "external/MonitorInterface.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
using namespace std;

extern void parsefile(const string&);

typedef MinisatID::WrappedPCSolver SATSolver;

/******************************
	User defined procedures
******************************/

int UserProcedure::_compilenumber = 0;

void UserProcedure::compile(lua_State* L) {
	if(!iscompiled()) {
		// Compose function header, body, and return statement
		stringstream ss;
		ss << "local function " << _name << "(";
		if(!_argnames.empty()) {
			vector<string>::iterator it = _argnames.begin();
			ss << *it; ++it;
			for(; it != _argnames.end(); ++it) ss << ',' << *it;
		}		
		ss << ')' << _code.str() << " end\n";
		ss << "return " << _name << "(...)\n";

		// Compile 
		int err = luaL_loadstring(L,ss.str().c_str());
		if(err) {
			Error::error(_pi);
			cerr << string(lua_tostring(L,-1)) << endl;
			lua_pop(L,1);
		}
		else {
			_registryindex = "idp_compiled_procedure_" + itos(UserProcedure::_compilenumber);
			++UserProcedure::_compilenumber;
			lua_setfield(L,LUA_REGISTRYINDEX,_registryindex.c_str());
		}
	}
}

/**************************
	Internal procedures
**************************/

/**
 * Types of arguments given to, or results produced by internal procedures
 */
enum ArgType {

	// Vocabulary
	AT_SORT,			//!< a sort
	AT_PREDICATE,		//!< a predicate symbol
	AT_FUNCTION,		//!< a function symbol
	AT_SYMBOL,			//!< a symbol of a vocabulary
	AT_VOCABULARY,		//!< a vocabulary

	// Structure
	AT_COMPOUND,		//!< a compound domain element
	AT_TUPLE,			//!< a tuple in a predicate or function table
	AT_DOMAIN,			//!< a domain of a sort
	AT_PREDTABLE,		//!< a predicate table
	AT_PREDINTER,		//!< a predicate interpretation
	AT_FUNCINTER,		//!< a function interpretation
	AT_STRUCTURE,		//!< a structure
	AT_TABLEITERATOR,	//!< a predicate table iterator
	AT_DOMAINITERATOR,	//!< a domain iterator

	// Theory
	AT_THEORY,			//!< a theory

	// Options
	AT_OPTIONS,			//!< an options block

	// Namespace
	AT_NAMESPACE,		//!< a namespace

	// Lua types
	AT_NIL,				//!< a nil value
	AT_INT,				//!< an integer
	AT_DOUBLE,			//!< a double
	AT_BOOLEAN,			//!< a boolean
	AT_STRING,			//!< a string
	AT_TABLE,			//!< a table
	AT_PROCEDURE,		//!< a procedure
   
	AT_OVERLOADED,		//!< an overloaded object

	// Additional return values
	AT_MULT,			//!< multiple arguments	(only used as return value)
	AT_REGISTRY			//!< a value stored in the registry of the lua state
};

/**
 * Objects to overload sorts, predicate, and function symbols
 */
class OverloadedSymbol {
	private:
		set<Sort*>		_sorts;
		set<Function*>	_funcs;
		set<Predicate*>	_preds;
	public:
		void insert(Sort* s)		{ _sorts.insert(s);	}
		void insert(Function* f)	{ _funcs.insert(f);	}
		void insert(Predicate* p)	{ _preds.insert(p);	}

		set<Sort*>*	sorts()			{ return &_sorts;	}
		set<Predicate*>* preds()	{ return &_preds;	}
		set<Function*>* funcs()		{ return &_funcs;	}

		vector<ArgType>	types() {
			vector<ArgType> result;
			if(!_sorts.empty()) result.push_back(AT_SORT);
			if(!_preds.empty()) result.push_back(AT_PREDICATE);
			if(!_funcs.empty()) result.push_back(AT_FUNCTION);
			return result;
		}
};

/**
 * Objects to overload members of namespaces and vocabularies
 */
class OverloadedObject {
	private:
		Namespace*			_namespace;
		Vocabulary*			_vocabulary;
		AbstractTheory*		_theory;
		AbstractStructure*	_structure;
		Options*			_options;
		UserProcedure*		_procedure;

		set<Predicate*>		_predicate;
		set<Function*>		_function;
		set<Sort*>			_sort;

	public:
		// Constructor
		OverloadedObject() : 
			_namespace(0), _vocabulary(0), _theory(0), _structure(0), _options(0), _procedure(0) { }

		void insert(Namespace* n)			{ _namespace = n;	}
		void insert(Vocabulary* n)			{ _vocabulary = n;	}
		void insert(AbstractTheory* n)		{ _theory = n;		}
		void insert(AbstractStructure* n)	{ _structure = n;	}
		void insert(Options* n)				{ _options = n;		}
		void insert(UserProcedure* n)		{ _procedure = n;	}

		AbstractStructure*	structure()		const { return _structure;	}
		AbstractTheory*		theory()		const { return _theory;		}
		Options*			options()		const { return _options;	}
		Namespace*			space()			const { return _namespace;	}
		Vocabulary*			vocabulary()	const { return _vocabulary;	}

		vector<ArgType> types() {
			vector<ArgType> result;
			if(_namespace) result.push_back(AT_NAMESPACE);
			if(_vocabulary) result.push_back(AT_VOCABULARY);
			if(_theory) result.push_back(AT_THEORY);
			if(_structure) result.push_back(AT_STRUCTURE);
			if(_options) result.push_back(AT_OPTIONS);
			if(_procedure) result.push_back(AT_PROCEDURE);
			if(!_predicate.empty()) result.push_back(AT_PREDICATE);
			if(!_function.empty()) result.push_back(AT_FUNCTION);
			if(!_sort.empty()) result.push_back(AT_SORT);
			return result;
		}
};

struct InternalArgument {
	ArgType		_type;
	union {
		set<Sort*>*						_sort;
		set<Predicate*>*				_predicate;
		set<Function*>*					_function;
		Vocabulary*						_vocabulary;

		const Compound*					_compound;
		ElementTuple*					_tuple;
		SortTable*						_domain;
		const PredTable*				_predtable;
		PredInter*						_predinter;
		FuncInter*						_funcinter;
		AbstractStructure*				_structure;
		TableIterator*					_tableiterator;
		SortIterator*					_sortiterator;

		AbstractTheory*					_theory;
		Options*						_options;
		Namespace*						_namespace;

		int								_int;
		double							_double;
		bool							_boolean;
		string*							_string;
		vector<InternalArgument>*		_table;

		OverloadedSymbol*				_symbol;
		OverloadedObject*				_overloaded;
	} _value;

	// Constructors
	InternalArgument() { }
	InternalArgument(Vocabulary* v)						: _type(AT_VOCABULARY)	{ _value._vocabulary = v;	}
	InternalArgument(PredInter* p)						: _type(AT_PREDINTER)	{ _value._predinter = p;	}
	InternalArgument(FuncInter* f)						: _type(AT_FUNCINTER)	{ _value._funcinter = f;	}
	InternalArgument(AbstractStructure* s)				: _type(AT_STRUCTURE)	{ _value._structure = s;	}
	InternalArgument(AbstractTheory* t)					: _type(AT_THEORY)		{ _value._theory = t;		}
	InternalArgument(Options* o)						: _type(AT_OPTIONS)		{ _value._options = o;		}
	InternalArgument(Namespace* n)						: _type(AT_NAMESPACE)	{ _value._namespace = n;	}
	InternalArgument(int i)								: _type(AT_INT)			{ _value._int = i;			}
	InternalArgument(double d)							: _type(AT_DOUBLE)		{ _value._double = d;		}
	InternalArgument(string* s)							: _type(AT_STRING)		{ _value._string = s;		}
	InternalArgument(vector<InternalArgument>* t)		: _type(AT_TABLE)		{ _value._table = t;		}
	InternalArgument(set<Sort*>* s)						: _type(AT_SORT)		{ _value._sort = s;			} 
	InternalArgument(set<Predicate*>* p)				: _type(AT_PREDICATE)	{ _value._predicate = p;	} 
	InternalArgument(set<Function*>* f)					: _type(AT_FUNCTION)	{ _value._function = f;		} 
	InternalArgument(OverloadedSymbol* s)				: _type(AT_SYMBOL)		{ _value._symbol = s;		}
	InternalArgument(const PredTable* t)				: _type(AT_PREDTABLE)	{ _value._predtable = t;	}
	InternalArgument(SortTable* t)						: _type(AT_DOMAIN)		{ _value._domain = t;		}
	InternalArgument(const Compound* c)					: _type(AT_COMPOUND)	{ _value._compound = c;		}
	InternalArgument(OverloadedObject* o)				: _type(AT_OVERLOADED)	{ _value._overloaded = o;	}

	InternalArgument(int arg, lua_State* L);
	InternalArgument(const DomainElement*);

	// Inspectors
	set<Sort*>*	sort() const { 
		if(_type == AT_SORT) return _value._sort;
		else if(_type == AT_SYMBOL) {
			return _value._symbol->sorts();
		}
		else {
			assert(false);
			return 0;
		}
	}

	AbstractTheory* theory() const {
		if(_type == AT_THEORY) return _value._theory;
		else if(_type == AT_OVERLOADED) return _value._overloaded->theory();
		else {
			assert(false);
			return 0;
		}
	}

	AbstractStructure* structure() const {
		if(_type == AT_STRUCTURE) return _value._structure;
		else if(_type == AT_OVERLOADED) return _value._overloaded->structure();
		else {
			assert(false);
			return 0;
		}
	}

	Options* options() const {
		if(_type == AT_OPTIONS) return _value._options;
		else if(_type == AT_OVERLOADED) return _value._overloaded->options();
		else {
			assert(false);
			return 0;
		}
	}

	Namespace* space() const {
		if(_type == AT_NAMESPACE) return _value._namespace;
		else if(_type == AT_OVERLOADED) return _value._overloaded->space();
		else {
			assert(false);
			return 0;
		}
	}

	Vocabulary* vocabulary() const {
		if(_type == AT_VOCABULARY) return _value._vocabulary;
		else if(_type == AT_OVERLOADED) return _value._overloaded->vocabulary();
		else {
			assert(false);
			return 0;
		}
	}

};

int ArgProcNumber = 0;						//!< Number to create unique registry indexes
static const char* _typefield = "type";		//!< Field index containing the type of userdata

/**
 * Get a domain element from the lua stack
 */
const DomainElement* convertToElement(int arg, lua_State* L) {
	switch(lua_type(L,arg)) {
		case LUA_TNIL: 
			return 0;
		case LUA_TSTRING: 
			return DomainElementFactory::instance()->create(StringPointer(lua_tostring(L,arg)));
		case LUA_TNUMBER:
			return DomainElementFactory::instance()->create(lua_tonumber(L,arg));
		case LUA_TUSERDATA:
		{
			lua_getmetatable(L,arg);
			lua_getfield(L,-1,_typefield);
			ArgType type = (ArgType)lua_tointeger(L,-1); assert(type != AT_NIL);
			lua_pop(L,2);
			switch(type) {
				case AT_COMPOUND:
					return DomainElementFactory::instance()->create(*(Compound**)lua_touserdata(L,arg));
				default:
					return 0;
			}
		}
		default:
			return 0;
	}
}

/**
 * Constructor to convert a domain element to an InternalArgument
 */
InternalArgument::InternalArgument(const DomainElement* el) {
	switch(el->type()) {
		case DET_INT:
			_type = AT_INT;
			_value._int = el->value()._int;
			break;
		case DET_DOUBLE:
			_type = AT_DOUBLE;
			_value._double = el->value()._double;
			break;
		case DET_STRING:
			_type = AT_STRING;
			_value._string = StringPointer(*(el->value()._string));
			break;
		case DET_COMPOUND:
			_type = AT_COMPOUND;
			_value._compound = el->value()._compound;
			break;
		default:
			assert(false);
	}
}

/**
 * Constructor to convert an element on the lua stack to an InternalArgument
 */
InternalArgument::InternalArgument(int arg, lua_State* L) {
	switch(lua_type(L,arg)) {
		case LUA_TNIL: 
			_type = AT_NIL; 
			break;
		case LUA_TBOOLEAN: 
			_type = AT_BOOLEAN; 
			_value._boolean = lua_toboolean(L,arg); 
			break;
		case LUA_TSTRING: 
			_type = AT_STRING; 
			_value._string = StringPointer(lua_tostring(L,arg)); 
			break;
		case LUA_TTABLE: 
			_type = AT_TABLE; 
			_value._table = new vector<InternalArgument>();
			lua_pushnil(L);
			while(lua_next(L,arg) != 0) {
				_value._table->push_back(InternalArgument(-1,L));
				lua_pop(L,1);
			}
			break;
		case LUA_TFUNCTION: 
		{
			_type = AT_PROCEDURE;
			string* registryindex = StringPointer(string("idp_argument_procedure_" + itos(ArgProcNumber)));
			++ArgProcNumber;
			lua_pushvalue(L,arg);
			lua_setfield(L,LUA_REGISTRYINDEX,registryindex->c_str());
			_value._string = registryindex;
			break;
		}
		case LUA_TNUMBER: {
			if(isInt(lua_tonumber(L,arg))) {
				_type = AT_INT;
				_value._int = lua_tointeger(L,arg);
			}
			else {
				_type = AT_DOUBLE;
				_value._double = lua_tonumber(L,arg);
			}
			break;
		}
		case LUA_TUSERDATA: {
			lua_getmetatable(L,arg);
			lua_getfield(L,-1,_typefield);
			_type = (ArgType)lua_tointeger(L,-1); assert(_type != AT_NIL);
			lua_pop(L,2);
			switch(_type) {
				case AT_SORT:
					_value._sort = *(set<Sort*>**)lua_touserdata(L,arg);
					break;
				case AT_PREDICATE:
					_value._predicate = *(set<Predicate*>**)lua_touserdata(L,arg);
					break;
				case AT_FUNCTION:		
					_value._function = *(set<Function*>**)lua_touserdata(L,arg);
					break;
				case AT_SYMBOL:
					_value._symbol = *(OverloadedSymbol**)lua_touserdata(L,arg);
					break;
				case AT_VOCABULARY:		
					_value._vocabulary = *(Vocabulary**)lua_touserdata(L,arg);
					break;
				case AT_COMPOUND:
					_value._compound = *(Compound**)lua_touserdata(L,arg);
					break;
				case AT_TUPLE:
					_value._tuple = *(ElementTuple**)lua_touserdata(L,arg);
					break;
				case AT_DOMAIN:
					_value._domain = *(SortTable**)lua_touserdata(L,arg);
					break;
				case AT_PREDTABLE:
					_value._predtable = *(PredTable**)lua_touserdata(L,arg);
					break;
				case AT_PREDINTER:		
					_value._predinter = *(PredInter**)lua_touserdata(L,arg);
					break;
				case AT_FUNCINTER:		
					_value._funcinter = *(FuncInter**)lua_touserdata(L,arg);
					break;
				case AT_STRUCTURE:		
					_value._structure = *(AbstractStructure**)lua_touserdata(L,arg);
					break;
				case AT_TABLEITERATOR:
					_value._tableiterator = *(TableIterator**)lua_touserdata(L,arg);
					break;
				case AT_DOMAINITERATOR:
					_value._sortiterator = *(SortIterator**)lua_touserdata(L,arg);
					break;
				case AT_THEORY:			
					_value._theory = *(AbstractTheory**)lua_touserdata(L,arg);
					break;
				case AT_OPTIONS:		
					_value._options = *(Options**)lua_touserdata(L,arg);
					break;
				case AT_NAMESPACE:
					_value._namespace = *(Namespace**)lua_touserdata(L,arg);
					break;
				case AT_OVERLOADED:
					_value._overloaded = *(OverloadedObject**)lua_touserdata(L,arg);
					break;
					break;
				default:
					assert(false);
			}
			break;
		}
		case LUA_TTHREAD: assert(false); break; 
		case LUA_TLIGHTUSERDATA: assert(false); break;
		case LUA_TNONE: assert(false); break;
	}
}


/**
 * Class to represent internal procedures
 */
class InternalProcedure {
	private:
		string				_name;		//!< the name of the procedure
		vector<ArgType>		_argtypes;	//!< types of the input arguments
		InternalArgument	(*_execute)(const vector<InternalArgument>&, lua_State*);

	public:
		InternalProcedure(const string& name, const vector<ArgType>& argtypes, 
						  InternalArgument (*execute)(const vector<InternalArgument>&, lua_State*)) :
			_name(name), _argtypes(argtypes), _execute(execute) { }
		~InternalProcedure() { }
		int operator()(lua_State*) const;
		const string& name() const { return _name;	}
};

/********************************************
	Implementation of internal procedures
********************************************/

InternalArgument idptype(const vector<InternalArgument>& args, lua_State*) {
	ArgType tp = (ArgType)args[0]._value._int;
	switch(tp) {
		case AT_SORT:
			return InternalArgument(StringPointer("type"));
		case AT_PREDICATE:
			return InternalArgument(StringPointer("predicate_symbol"));
		case AT_FUNCTION:
			return InternalArgument(StringPointer("function_symbol"));
		case AT_SYMBOL:
			return InternalArgument(StringPointer("symbol"));
		case AT_VOCABULARY:
			return InternalArgument(StringPointer("vocabulary"));
		case AT_COMPOUND:
			return InternalArgument(StringPointer("compound"));
		case AT_TUPLE:
			return InternalArgument(StringPointer("tuple"));
		case AT_DOMAIN:
			return InternalArgument(StringPointer("domain"));
		case AT_PREDTABLE:
			return InternalArgument(StringPointer("predicate_table"));
		case AT_PREDINTER:
			return InternalArgument(StringPointer("predicate_interpretation"));
		case AT_FUNCINTER:
			return InternalArgument(StringPointer("function_interpretation"));
		case AT_STRUCTURE:
			return InternalArgument(StringPointer("structure"));
		case AT_THEORY:
			return InternalArgument(StringPointer("theory"));
		case AT_OPTIONS:
			return InternalArgument(StringPointer("options"));
		case AT_NAMESPACE:
			return InternalArgument(StringPointer("namespace"));
		case AT_OVERLOADED:
			return InternalArgument(StringPointer("overloaded"));
		case AT_NIL:
			return InternalArgument(StringPointer("nil"));
		case AT_INT:
			return InternalArgument(StringPointer("number"));
		case AT_DOUBLE:
			return InternalArgument(StringPointer("number"));
		case AT_BOOLEAN:
			return InternalArgument(StringPointer("boolean"));
		case AT_STRING:
			return InternalArgument(StringPointer("string"));
		case AT_TABLE:
			return InternalArgument(StringPointer("table"));
		case AT_PROCEDURE:
			return InternalArgument(StringPointer("function"));
		case AT_MULT:
			return InternalArgument(StringPointer("mult"));
		case AT_REGISTRY:
			return InternalArgument(StringPointer("registry"));
		case AT_TABLEITERATOR:
			return InternalArgument(StringPointer("predicate_table_iterator"));
		case AT_DOMAINITERATOR:
			return InternalArgument(StringPointer("domain_iterator"));
		default:
			assert(false);
	}
	InternalArgument ia;
	return ia;
}

InternalArgument printtheory(const vector<InternalArgument>& args, lua_State* ) {
	AbstractTheory* theory = args[0].theory();
	Options* opts = args[1].options();

	Printer* printer = Printer::create(opts);
	string str = printer->print(theory);
	delete(printer);

	InternalArgument result(StringPointer(str));
	return result;
}

InternalArgument printoptions(const vector<InternalArgument>& args, lua_State* ) {
	Options* opts = args[0].options();
	return InternalArgument(StringPointer(opts->to_string()));
}

InternalArgument printstructure(const vector<InternalArgument>& args, lua_State* ) {
	AbstractStructure* structure = args[0].structure();
	Options* opts = args[1].options();

	Printer* printer = Printer::create(opts);
	string str = printer->print(structure);
	delete(printer);

	InternalArgument result(StringPointer(str));
	return result;
}

InternalArgument printvocabulary(const vector<InternalArgument>& args, lua_State* ) {
	Vocabulary* vocabulary = args[0].vocabulary();
	Options* opts = args[1].options();

	Printer* printer = Printer::create(opts);
	string str = printer->print(vocabulary);
	delete(printer);

	InternalArgument result(StringPointer(str));
	return result;
}

InternalArgument printnamespace(const vector<InternalArgument>& args, lua_State* ) {
	Namespace* space = args[0].space();
	Options* opts = args[1].options();

	Printer* printer = Printer::create(opts);
	string str = printer->print(space);
	delete(printer);

	InternalArgument result(StringPointer(str));
	return result;
}

InternalArgument newstructure(const vector<InternalArgument>& args, lua_State* ) {
	Vocabulary* v = args[0].vocabulary();
	Structure* s = new Structure("",ParseInfo());
	s->vocabulary(v);
	return InternalArgument(s);
}

InternalArgument newtheory(const vector<InternalArgument>& args, lua_State* ) {
	Vocabulary* v = args[0].vocabulary();
	Theory* t = new Theory("",v,ParseInfo());
	return InternalArgument(t);
}

InternalArgument newoptions(const vector<InternalArgument>& , lua_State* ) {
	Options* opts = new Options("",ParseInfo());
	return InternalArgument(opts);
}

InternalArgument clonetheory(const vector<InternalArgument>& args, lua_State* ) {
	AbstractTheory* t = args[0].theory();
	return InternalArgument(t->clone());
}

InternalArgument clonestructure(const vector<InternalArgument>& args, lua_State* ) {
	AbstractStructure* s = args[0].structure();
	return InternalArgument(s->clone());
}

InternalArgument pushnegations(const vector<InternalArgument>& args, lua_State* ) {
	AbstractTheory* t = args[0].theory();
	TheoryUtils::push_negations(t);
	return InternalArgument(t);
}

InternalArgument flatten(const vector<InternalArgument>& args, lua_State* ) {
	AbstractTheory* t = args[0].theory();
	TheoryUtils::flatten(t);
	return InternalArgument(t);
}

string help(Namespace* ns) {
	stringstream sstr;
	if(ns->procedures().empty()) {
		if(ns->isGlobal()) sstr << "There are no procedures in the global namespace\n";
		else {
			sstr << "There are no procedures in namespace ";
			ns->putname(sstr);
			sstr << '\n';
		}
	}
	else {
		sstr << "The following procedures are available:\n\n";
		stringstream prefixs;
		ns->putname(prefixs);
		string prefix = prefixs.str();
		if(prefix != "") prefix += "::";
		for(map<string,UserProcedure*>::const_iterator it = ns->procedures().begin(); it != ns->procedures().end(); ++it) {
			sstr << "    * " << prefix << it->second->name() << '(';
			if(!it->second->args().empty()) {
				sstr << it->second->args()[0];
				for(unsigned int n = 1; n < it->second->args().size(); ++n) {
					sstr << ',' << it->second->args()[n];
				}
			}
			sstr << ")\n";
			sstr << "        " << it->second->description() << "\n";
		}
	}
	if(!ns->subspaces().empty()) {
		sstr << "\nThe following subspaces are available:\n\n";
		for(map<string,Namespace*>::const_iterator it = ns->subspaces().begin(); it != ns->subspaces().end(); ++it) {
			sstr << "    * ";
			it->second->putname(sstr); 
			sstr << '\n';
		}
		sstr << "\nType help(<subspace>) for information on procedures in namespace <subspace>\n";
	}
	return sstr.str();
}

InternalArgument globalhelp(const vector<InternalArgument>&, lua_State* L) {
	string str = help(Namespace::global());
	lua_getglobal(L,"print");
	lua_pushstring(L,str.c_str());
	lua_call(L,1,0);

	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

InternalArgument help(const vector<InternalArgument>& args, lua_State* L) {
	Namespace* ns = args[0].space();
	string str = help(ns);
	lua_getglobal(L,"print");
	lua_pushstring(L,str.c_str());
	lua_call(L,1,0);
	
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

/********************************
	Model expansion inference
********************************/

SATSolver* createsolver(Options* options) {
	MinisatID::SolverOption modes;
	modes.nbmodels = options->nrmodels();
	modes.verbosity = options->satverbosity();
	modes.remap = false;
	return new SATSolver(modes);
}

MinisatID::Solution* initsolution(Options* options) {
	MinisatID::ModelExpandOptions opts;
	opts.nbmodelstofind = options->nrmodels();
	opts.printmodels = MinisatID::PRINT_NONE;
	opts.savemodels = MinisatID::SAVE_ALL;
	opts.search = MinisatID::MODELEXPAND;
	return new MinisatID::Solution(opts);
}

void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) {
	for(vector<MinisatID::Literal>::const_iterator literal = model->literalinterpretations.begin();
		literal != model->literalinterpretations.end(); ++literal) {
		int atomnr = literal->getAtom().getValue();
		PFSymbol* symbol = translator->symbol(atomnr);
		if(symbol) {
			const ElementTuple& args = translator->args(atomnr);
			if(typeid(*symbol) == typeid(Predicate)) {
				Predicate* pred = dynamic_cast<Predicate*>(symbol);
				if(literal->hasSign()) init->inter(pred)->makeFalse(args);
				else init->inter(pred)->makeTrue(args);
			}
			else {
				Function* func = dynamic_cast<Function*>(symbol);
				if(literal->hasSign()) init->inter(func)->graphinter()->makeFalse(args);
				else init->inter(func)->graphinter()->makeTrue(args);
			}
		}
	}
}

void addTerms(MinisatID::Model* model, GroundTermTranslator* translator, AbstractStructure* init) {
	for(vector<MinisatID::VariableEqValue>::const_iterator cpvar = model->variableassignments.begin();
		cpvar != model->variableassignments.end(); ++cpvar) {
		Function* function = translator->function(cpvar->variable);
		if(function) {
			ElementTuple tuple = translator->args(cpvar->variable);
			tuple.push_back(DomainElementFactory::instance()->create(cpvar->value));
			init->inter(function)->graphinter()->makeTrue(tuple);
		}
	}
}

vector<AbstractStructure*> modelexpand(AbstractTheory* theory, AbstractStructure* structure, Options* options) {
	// Create solver and grounder
	SATSolver* solver = createsolver(options);
	GrounderFactory grounderfactory(structure,options);
	TopLevelGrounder* grounder = grounderfactory.create(theory,solver);

	// Run grounder
	grounder->run();
	SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

	// Add information that is abstracted in the grounding
	grounding->addFuncConstraints();
	grounding->addFalseDefineds();

	// Run solver
	MinisatID::Solution* abstractsolutions = initsolution(options);
	solver->solve(abstractsolutions);
	
	// Collect solutions
	vector<AbstractStructure*> solutions;
	for(vector<MinisatID::Model*>::const_iterator model = abstractsolutions->getModels().begin();
		model != abstractsolutions->getModels().end(); ++model) {
		AbstractStructure* newsolution = structure->clone();
		addLiterals(*model,grounding->translator(),newsolution);
		addTerms(*model,grounding->termtranslator(),newsolution);
		newsolution->clean();
		solutions.push_back(newsolution);
	}
	
	return solutions;
}

InternalArgument modelexpand(const vector<InternalArgument>& args, lua_State* ) {
	vector<AbstractStructure*> solutions = modelexpand(args[0].theory(),args[1].structure(),args[2].options());
	InternalArgument result; 
	result._type = AT_TABLE;
	result._value._table = new vector<InternalArgument>();
	for(vector<AbstractStructure*>::const_iterator it = solutions.begin(); it != solutions.end(); ++it) {
		result._value._table->push_back(InternalArgument(*it));
	}
	return result;
}

AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options) {
	GrounderFactory factory(structure,options);
	TopLevelGrounder* grounder = factory.create(theory);
	grounder->run();
	return grounder->grounding();
}

InternalArgument ground(const vector<InternalArgument>& args, lua_State*  ) {
	AbstractTheory* grounding = ground(args[0].theory(),args[1].structure(),args[2].options());
	InternalArgument result(grounding); 
	return result;
}

InternalArgument createrange(const vector<InternalArgument>& args, lua_State* ) {
	int n1 = args[0]._value._int;
	int n2 = args[1]._value._int;
	InternalArgument ia; ia._type = AT_DOMAIN;
	if(n1 <= n2) {
		ia._value._domain = new SortTable(new IntRangeInternalSortTable(n1,n2));
	}
	else {
		ia._value._domain = new SortTable(new EnumeratedInternalSortTable());
	}
	return ia;
}

InternalArgument createtuple(const vector<InternalArgument>& , lua_State*) {
	InternalArgument ia;
	ia._type = AT_TUPLE;
	ia._value._tuple = 0;
	return ia;
}

InternalArgument derefandincrement(const vector<InternalArgument>& args, lua_State* ) {
	TableIterator* it = args[0]._value._tableiterator;
	if(it->hasNext()) {
		ElementTuple* tuple = new ElementTuple(*(*it));
		it->operator++();
		InternalArgument ia; ia._type = AT_TUPLE; ia._value._tuple = tuple;
		return ia;
	}
	else {
		InternalArgument ia; ia._type = AT_NIL;
		return ia;
	}
}

InternalArgument domderefandincrement(const vector<InternalArgument>& args, lua_State* ) {
	SortIterator* it = args[0]._value._sortiterator;
	if(it->hasNext()) {
		const DomainElement* element = *(*it);
		it->operator++();
		InternalArgument ia(element);
		return ia;
	}
	else {
		InternalArgument ia; ia._type = AT_NIL;
		return ia;
	}
}

InternalArgument tableiterator(const vector<InternalArgument>& args, lua_State* ) {
	const PredTable* pt = args[0]._value._predtable;
	TableIterator* tit = new TableIterator(pt->begin());
	InternalArgument ia; ia._type = AT_TABLEITERATOR;
	ia._value._tableiterator = tit;
	return ia;
}

InternalArgument domainiterator(const vector<InternalArgument>& args, lua_State* ) {
	const SortTable* st = args[0]._value._domain;
	SortIterator* it = new SortIterator(st->sortbegin());
	InternalArgument ia; ia._type = AT_DOMAINITERATOR;
	ia._value._sortiterator = it;
	return ia;
}

InternalArgument changevocabulary(const vector<InternalArgument>& args, lua_State* ) {
	AbstractStructure* s = args[0].structure();
	Vocabulary* v = args[1].vocabulary();
	s->vocabulary(v);
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

InternalArgument maketrue(const vector<InternalArgument>& args, lua_State* ) {
	PredInter* pri = args[0]._value._predinter;
	ElementTuple* tup = args[1]._value._tuple;
	pri->makeTrue(*tup);
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

InternalArgument maketabtrue(const vector<InternalArgument>& args, lua_State* L) {
	PredInter* pri = args[0]._value._predinter;
	vector<InternalArgument>* tab = args[1]._value._table;
	ElementTuple tup;
	for(vector<InternalArgument>::const_iterator it = tab->begin(); it != tab->end(); ++it) {
		switch(it->_type) {
			case AT_INT: tup.push_back(DomainElementFactory::instance()->create(it->_value._int)); break;
			case AT_DOUBLE: tup.push_back(DomainElementFactory::instance()->create(it->_value._double)); break;
			case AT_STRING: tup.push_back(DomainElementFactory::instance()->create(it->_value._string)); break;
			case AT_COMPOUND: tup.push_back(DomainElementFactory::instance()->create(it->_value._compound)); break;
			default:
				lua_pushstring(L,"Wrong value in a tuple. Expected an integer, double, string, or compound");
				return lua_error(L);
		}
	}
	pri->makeTrue(tup);
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

InternalArgument makefalse(const vector<InternalArgument>& args, lua_State* ) {
	PredInter* pri = args[0]._value._predinter;
	ElementTuple* tup = args[1]._value._tuple;
	pri->makeFalse(*tup);
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

InternalArgument maketabfalse(const vector<InternalArgument>& args, lua_State* L) {
	PredInter* pri = args[0]._value._predinter;
	vector<InternalArgument>* tab = args[1]._value._table;
	ElementTuple tup;
	for(vector<InternalArgument>::const_iterator it = tab->begin(); it != tab->end(); ++it) {
		switch(it->_type) {
			case AT_INT: tup.push_back(DomainElementFactory::instance()->create(it->_value._int)); break;
			case AT_DOUBLE: tup.push_back(DomainElementFactory::instance()->create(it->_value._double)); break;
			case AT_STRING: tup.push_back(DomainElementFactory::instance()->create(it->_value._string)); break;
			case AT_COMPOUND: tup.push_back(DomainElementFactory::instance()->create(it->_value._compound)); break;
			default:
				lua_pushstring(L,"Wrong value in a tuple. Expected an integer, double, string, or compound");
				return lua_error(L);
		}
	}
	pri->makeFalse(tup);
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}

/**************************
	Connection with Lua
**************************/

namespace LuaConnection {

	lua_State* _state;

	/**
	 * Push a domain element to the lua stack
	 */
	int convertToLua(lua_State* L, const DomainElement* d) {
		switch(d->type()) {
			case DET_INT:
				lua_pushinteger(L,d->value()._int);
				return 1;
			case DET_DOUBLE:
				lua_pushnumber(L,d->value()._double);
				return 1;
			case DET_STRING:
				lua_pushstring(L,d->value()._string->c_str());
				return 1;
			case DET_COMPOUND:
			{
				const Compound** ptr = (const Compound**)lua_newuserdata(L,sizeof(Compound*));
				(*ptr) = d->value()._compound;
				luaL_getmetatable(L,"compound");
				lua_setmetatable(L,-2);
				return 1;
			}
			default:
				assert(false);
				return 0;
		}
	}

	/**
	 * Push an internal argument to the lua stack
	 */
	int convertToLua(lua_State* L, InternalArgument arg) {
		switch(arg._type) {
			case AT_SORT:
			{
				set<Sort*>** ptr = (set<Sort*>**)lua_newuserdata(L,sizeof(set<Sort*>*));
				(*ptr) = arg._value._sort;
				luaL_getmetatable (L,"sort");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_PREDICATE:
			{
				set<Predicate*>** ptr = (set<Predicate*>**)lua_newuserdata(L,sizeof(set<Predicate*>*));
				(*ptr) = arg._value._predicate;
				luaL_getmetatable(L,"predicate_symbol");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_FUNCTION:
			{
				set<Function*>** ptr = (set<Function*>**)lua_newuserdata(L,sizeof(set<Function*>*));
				(*ptr) = arg._value._function;
				luaL_getmetatable(L,"function_symbol");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_SYMBOL:
			{
				OverloadedSymbol** ptr = (OverloadedSymbol**)lua_newuserdata(L,sizeof(OverloadedSymbol*));
				(*ptr) = arg._value._symbol;
				luaL_getmetatable(L,"symbol");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_VOCABULARY:
			{
				Vocabulary** ptr = (Vocabulary**)lua_newuserdata(L,sizeof(Vocabulary*));
				(*ptr) = arg._value._vocabulary;
				luaL_getmetatable(L,"vocabulary");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_COMPOUND:
			{
				const Compound** ptr = (const Compound**)lua_newuserdata(L,sizeof(Compound*));
				(*ptr) = arg._value._compound;
				luaL_getmetatable(L,"compound");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_TUPLE:
			{
				ElementTuple** ptr = (ElementTuple**)lua_newuserdata(L,sizeof(ElementTuple*));
				(*ptr) = arg._value._tuple;
				luaL_getmetatable(L,"tuple");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_DOMAIN:
			{
				SortTable** ptr = (SortTable**)lua_newuserdata(L,sizeof(SortTable*));
				(*ptr) = arg._value._domain;
				luaL_getmetatable(L,"domain");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_PREDTABLE:
			{
				const PredTable** ptr = (const PredTable**)lua_newuserdata(L,sizeof(const PredTable*));
				(*ptr) = arg._value._predtable;
				luaL_getmetatable(L,"predtable");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_PREDINTER:
			{
				PredInter** ptr = (PredInter**)lua_newuserdata(L,sizeof(PredInter*));
				(*ptr) = arg._value._predinter;
				luaL_getmetatable(L,"predinter");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_FUNCINTER:
			{
				FuncInter** ptr = (FuncInter**)lua_newuserdata(L,sizeof(FuncInter*));
				(*ptr) = arg._value._funcinter;
				luaL_getmetatable(L,"funcinter");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_STRUCTURE:
			{
				AbstractStructure** ptr = (AbstractStructure**)lua_newuserdata(L,sizeof(AbstractStructure*));
				(*ptr) = arg._value._structure;
				luaL_getmetatable(L,"structure");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_TABLEITERATOR:
			{
				TableIterator** ptr = (TableIterator**)lua_newuserdata(L,sizeof(TableIterator*));
				(*ptr) = arg._value._tableiterator;
				luaL_getmetatable(L,"tableiterator");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_DOMAINITERATOR:
			{
				SortIterator** ptr = (SortIterator**)lua_newuserdata(L,sizeof(SortIterator*));
				(*ptr) = arg._value._sortiterator;
				luaL_getmetatable(L,"domainiterator");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_THEORY:
			{
				AbstractTheory** ptr = (AbstractTheory**)lua_newuserdata(L,sizeof(AbstractTheory*));
				(*ptr) = arg._value._theory;
				luaL_getmetatable(L,"theory");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_OPTIONS:
			{
				Options** ptr = (Options**)lua_newuserdata(L,sizeof(Options*));
				(*ptr) = arg._value._options;
				luaL_getmetatable(L,"options");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_NAMESPACE:
			{
				Namespace** ptr = (Namespace**)lua_newuserdata(L,sizeof(Namespace*));
				(*ptr) = arg._value._namespace;
				luaL_getmetatable(L,"namespace");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_NIL:
			{
				lua_pushnil(L);
				return 1;
			}
			case AT_INT:
			{
				lua_pushinteger(L,arg._value._int);
				return 1;
			}
			case AT_DOUBLE:
			{
				lua_pushnumber(L,arg._value._double);
				return 1;
			}
			case AT_BOOLEAN:
			{
				lua_pushboolean(L,arg._value._boolean);
				return 1;
			}
			case AT_STRING:
			{
				lua_pushstring(L,arg._value._string->c_str());
				return 1;
			}
			case AT_TABLE:
			{
				lua_newtable(L);
				for(unsigned int n = 0; n < arg._value._table->size(); ++n) {
					lua_pushinteger(L,n+1);
					convertToLua(L,(*(arg._value._table))[n]);
					lua_settable(L,-3);
				}
				return 1;
			}
			case AT_PROCEDURE:
			{
				lua_getfield(L,LUA_REGISTRYINDEX,arg._value._string->c_str());
				return 1;
			}
			case AT_OVERLOADED:
			{
				OverloadedObject** ptr = (OverloadedObject**)lua_newuserdata(L,sizeof(OverloadedObject*));
				(*ptr) = arg._value._overloaded;
				luaL_getmetatable(L,"overloaded");
				lua_setmetatable(L,-2);
				return 1;
			}
			case AT_MULT:
			{
				int nrres = 0;
				for(unsigned int n = 0; n < arg._value._table->size(); ++n) {
					nrres += convertToLua(L,(*(arg._value._table))[n]);
				}
				return nrres;
			}
			case AT_REGISTRY:
			{
				lua_getfield(L,LUA_REGISTRYINDEX,arg._value._string->c_str());
				return 1;
			}
			default:
				assert(false); return 0;
		}
	}

	/**
	 * Get all argument types of an object on the lua stack
	 *
	 * \param arg	the index in the lua stack
	 */
	vector<ArgType> getArgTypes(lua_State* L, unsigned int arg) {
		vector<ArgType> result;
		switch(lua_type(L,arg)) {
			case LUA_TNIL: result.push_back(AT_NIL); break;
			case LUA_TBOOLEAN: result.push_back(AT_BOOLEAN); break;
			case LUA_TSTRING: result.push_back(AT_STRING); break;
			case LUA_TTABLE: result.push_back(AT_TABLE); break;
			case LUA_TFUNCTION: result.push_back(AT_PROCEDURE); break;
			case LUA_TNUMBER: {
				if(isInt(lua_tonumber(L,arg))) result.push_back(AT_INT);
				else result.push_back(AT_DOUBLE);
				break;
			}
			case LUA_TUSERDATA: {
				lua_getmetatable(L,arg);
				lua_getfield(L,-1,_typefield);
				ArgType type = (ArgType)lua_tointeger(L,-1);
				if(type == AT_OVERLOADED) {
					OverloadedObject* oo = *(OverloadedObject**)lua_touserdata(L,arg);
					result = oo->types();
				}
				else if(type == AT_SYMBOL) {
					OverloadedSymbol* os = *(OverloadedSymbol**)lua_touserdata(L,arg);
					result = os->types();
				}
				else  result.push_back(type);
				lua_pop(L,2);
				break;
			}
			case LUA_TTHREAD: assert(false); break; 
			case LUA_TLIGHTUSERDATA: assert(false); break;
			case LUA_TNONE: assert(false); break;
		}
		return result;
	}

	/**
	 *	Call to internal procedure
	 */
	int internalCall(lua_State* L) {
		// get the list of possible procedures
		map<vector<ArgType>,InternalProcedure*>* procs = *(map<vector<ArgType>,InternalProcedure*>**)lua_touserdata(L,1);
		lua_remove(L,1);

		// get the list of possible argument types
		vector<vector<ArgType> > argtypes(lua_gettop(L));
		for(int arg = 1; arg <= lua_gettop(L); ++arg) 
			argtypes[arg-1] = getArgTypes(L,arg);

		// find the right procedure
		InternalProcedure* proc = 0;
		vector<vector<ArgType>::iterator> carry(argtypes.size());
		for(unsigned int n = 0; n < argtypes.size(); ++n) carry[n] = argtypes[n].begin();
		while(true) {
			vector<ArgType> currtypes(argtypes.size());
			for(unsigned int n = 0; n < argtypes.size(); ++n) currtypes[n] = *(carry[n]);
			if(procs->find(currtypes) != procs->end()) {
				if(!proc) {
					proc = (*procs)[currtypes];
				}
				else { Error::ambigcommand(proc->name()); return 0; }
			}
			unsigned int c = 0;
			for(; c < argtypes.size(); ++c) {
				++(carry[c]);
				if(carry[c] != argtypes[c].end()) break;
				else carry[c] = argtypes[c].begin();
			}
			if(c == argtypes.size()) break;
		}

		// Execute the procedure
		if(proc) return (*proc)(L);
		else { 
			assert(!procs->empty());
			Error::wrongcommandargs(procs->begin()->second->name()); return 0; 
		}
	}

	/**
	 * Garbage collection for internal procedures
	 */
	int gcInternProc(lua_State* L) {
		map<vector<ArgType>,InternalProcedure*>* proc = *(map<vector<ArgType>,InternalProcedure*>**)lua_touserdata(L,1);
		delete(proc);
		return 0;
	}

	/**
	 * Garbage collection for sorts
	 */
	int gcSort(lua_State* L) {
		set<Sort*>* sort = *(set<Sort*>**)lua_touserdata(L,1);
		delete(sort);
		return 0;
	}

	/**
	 * Garbage collection for predicates
	 */
	int gcPredicate(lua_State* L) {
		set<Predicate*>* pred = *(set<Predicate*>**)lua_touserdata(L,1);
		delete(pred);
		return 0;
	}

	/**
	 * Garbage collection for functions
	 */
	int gcFunction(lua_State* L) {
		set<Function*>* func = *(set<Function*>**)lua_touserdata(L,1);
		delete(func);
		return 0;
	}

	/**
	 * Garbage collection for symbols
	 */
	int gcSymbol(lua_State* L) {
		OverloadedSymbol* symb = *(OverloadedSymbol**)lua_touserdata(L,1);
		delete(symb);
		return 0;
	}

	/**
	 * Garbage collection for vocabularies
	 */
	int gcVocabulary(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for compounds
	 */
	int gcCompound(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for tuples
	 */
	int gcTuple(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for domains
	 */
	int gcDomain(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for predicate tables
	 */
	int gcPredTable(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for predicate interpretations
	 */
	int gcPredInter(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for function interpretations
	 */
	int gcFuncInter(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for structures
	 */
	int gcStructure(lua_State* ) {
		//AbstractStructure* s = *(AbstractStructure**)lua_touserdata(L,1);
		//if(s->pi().line() == 0) delete(s);	// FIXME: replace this by a check whether s belongs to a namespace
		return 0;
	}

	/**
	 * Garbage collection for table iterators
	 */
	int gcTableIterator(lua_State* L) {
		TableIterator* i = *(TableIterator**)lua_touserdata(L,1);
		delete(i);
		return 0;
	}

	/**
	 * Garbage collection for domain iterators
	 */
	int gcDomainIterator(lua_State* L) {
		SortIterator* i = *(SortIterator**)lua_touserdata(L,1);
		delete(i);
		return 0;
	}

	/**
	 * Garbage collection for theories
	 */
	int gcTheory(lua_State* ) {
		//AbstractTheory* t = *(AbstractTheory**)lua_touserdata(L,1);
		//if(t->pi().line() == 0) t->recursiveDelete();	// FIXME: replace this by a check whether t belongs to a namespace
		return 0;
	}

	/**
	 * Garbage collection for options
	 */
	int gcOptions(lua_State* ) {
		//Options* opts = *(Options**)lua_touserdata(L,1);
		//if(opts->pi().line() == 0) delete(opts);	// FIXME: replace this by a check whether opts belongs to a namespace
		return 0;
	}

	/**
	 * Garbage collection for namespaces
	 */
	int gcNamespace(lua_State* ) {
		return 0;
	}

	/**
	 * Garbage collection for overloaded objects
	 */
	int gcOverloaded(lua_State* L) {
		OverloadedObject* obj = *(OverloadedObject**)lua_touserdata(L,1);
		delete(obj);
		return 0;
	}

	/**
	 * Index function for predicate symbols
	 */
	int predicateIndex(lua_State* L) {
		set<Predicate*>* pred = *(set<Predicate*>**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_SORT) {
			set<Sort*>* sort = index.sort();
			set<Predicate*>* newpred = new set<Predicate*>();
			for(set<Sort*>::const_iterator it = sort->begin(); it != sort->end(); ++it) {
				for(set<Predicate*>::const_iterator jt = pred->begin(); jt != pred->end(); ++jt) {
					if((*jt)->arity() == 1) {
						if((*jt)->resolve(vector<Sort*>(1,(*it)))) newpred->insert(*jt);
					}
				}
			}
			InternalArgument np(newpred);
			return convertToLua(L,np);
		}
		else if(index._type == AT_TABLE) {
			vector<InternalArgument>* table = index._value._table;
			for(vector<InternalArgument>::const_iterator it =table->begin(); it != table->end(); ++it) {
				if(it->_type != AT_SORT) {
					lua_pushstring(L,"A predicate can only be indexed by a tuple of types");
					return lua_error(L);
				}
			}
			set<Predicate*>* newpred = new set<Predicate*>();
			vector<set<Sort*>::iterator> carry(table->size());
			for(unsigned int n = 0; n < table->size(); ++n) carry[n] = (*table)[n].sort()->begin();
			while(true) {
				vector<Sort*> currsorts(table->size());
				for(unsigned int n = 0; table->size(); ++n) currsorts[n] = *(carry[n]);
				for(set<Predicate*>::const_iterator it = pred->begin(); it != pred->end(); ++it) {
					if((*it)->arity() == table->size()) {
						if((*it)->resolve(currsorts)) newpred->insert(*it);
					}
				}
				unsigned int c = 0;
				for(; c < table->size(); ++c) {
					++(carry[c]);
					if(carry[c] != (*table)[c].sort()->end()) break;
					else carry[c] = (*table)[c].sort()->begin();
				}
				if(c == table->size()) break;
			}
			InternalArgument np(newpred);
			return convertToLua(L,np);
		}
		else {
			lua_pushstring(L,"A predicate can only be indexed by a tuple of types");
			return lua_error(L);
		}
	}

	/**
	 * Index function for function symbols
	 */
	int functionIndex(lua_State* L) {
		set<Function*>* func = *(set<Function*>**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_TABLE) {
			vector<InternalArgument>* table = index._value._table;
			vector<InternalArgument> newtable;
			if(table->size() < 2) {
				lua_pushstring(L,"Invalid function symbol index");
				return lua_error(L);
			}
			for(unsigned int n = 0; n < table->size(); ++n) {
				if(n == table->size()-2) {
					if((*table)[n]._type != AT_PROCEDURE) {
						lua_pushstring(L,"Expected a colon in a function symbol index");
						return lua_error(L);
					}
				}
				else {
					if((*table)[n]._type == AT_SORT) {
						newtable.push_back((*table)[n]);
					}
					else {
						lua_pushstring(L,"A function symbol can only be indexed by a tuple of types");
						return lua_error(L);
					}
				}
			}
			set<Function*>* newfunc = new set<Function*>();
			vector<set<Sort*>::iterator> carry(newtable.size());
			for(unsigned int n = 0; n < newtable.size(); ++n) carry[n] = newtable[n].sort()->begin();
			while(true) {
				vector<Sort*> currsorts(newtable.size());
				for(unsigned int n = 0; newtable.size(); ++n) currsorts[n] = *(carry[n]);
				for(set<Function*>::const_iterator it = func->begin(); it != func->end(); ++it) {
					if((*it)->arity() == newtable.size()) {
						if((*it)->resolve(currsorts)) newfunc->insert(*it);
					}
				}
				unsigned int c = 0;
				for(; c < newtable.size(); ++c) {
					++(carry[c]);
					if(carry[c] != newtable[c].sort()->end()) break;
					else carry[c] = newtable[c].sort()->begin();
				}
				if(c == newtable.size()) break;
			}
			InternalArgument nf(newfunc);
			return convertToLua(L,nf);
		}
		else {
			lua_pushstring(L,"A function can only be indexed by a tuple of sorts");
			return lua_error(L);
		}
	}

	/**
	 * Index function for symbols
	 */
	int symbolIndex(lua_State* L) {
		OverloadedSymbol* symb = *(OverloadedSymbol**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_TABLE) {
			if(index._value._table->size() > 2 && 
				(*(index._value._table))[index._value._table->size()-2]._type == AT_PROCEDURE) {
				convertToLua(L,InternalArgument(new set<Function*>(*(symb->funcs()))));
				lua_replace(L,1);
				return functionIndex(L);
			}
			else {
				convertToLua(L,InternalArgument(new set<Predicate*>(*(symb->preds()))));
				lua_replace(L,1);
				return predicateIndex(L);
			}
		}
		else if(index._type == AT_STRING) {
			string str = *(index._value._string);
			if(str == "type") {
				convertToLua(L,InternalArgument(new set<Sort*>(*(symb->sorts()))));
				return 1;
			}
			else if(str == "predicate") {
				convertToLua(L,InternalArgument(new set<Predicate*>(*(symb->preds()))));
				return 1;
			}
			else if(str == "function") {
				convertToLua(L,InternalArgument(new set<Function*>(*(symb->funcs()))));
				return 1;
			}
			else {
				lua_pushstring(L,"A symbol can only be indexed by a tuple of sorts or the strings \"type\", \"predicate\", or \"function\".");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A symbol can only be indexed by a tuple of sorts or the strings \"type\", \"predicate\", or \"function\".");
			return lua_error(L);
		}
	}

	/**
	 * Index function for vocabularies
	 */
	int vocabularyIndex(lua_State* L) {
		Vocabulary* voc = *(Vocabulary**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_STRING) {
			unsigned int emptycounter = 0;
			const set<Sort*>* sorts = voc->sort(*(index._value._string));
			if((!sorts) || sorts->empty()) ++emptycounter;
			set<Predicate*> preds = voc->pred_no_arity(*(index._value._string));
			if(preds.empty()) ++emptycounter;
			set<Function*> funcs = voc->func_no_arity(*(index._value._string));
			if(funcs.empty()) ++emptycounter;
			if(emptycounter == 3) return 0;
			else if(emptycounter == 2) {
				if(sorts && !sorts->empty()) {
					set<Sort*>* newsorts = new set<Sort*>(*sorts);
					InternalArgument ns(newsorts);
					return convertToLua(L,ns);
				}
				else if(!preds.empty()) {
					set<Predicate*>* newpreds = new set<Predicate*>(preds);
					InternalArgument np(newpreds);
					return convertToLua(L,np);
				}
				else {
					assert(!funcs.empty());
					set<Function*>* newfuncs = new set<Function*>(funcs);
					InternalArgument nf(newfuncs);
					return convertToLua(L,nf);
				}
			}
			else {
				OverloadedSymbol* os = new OverloadedSymbol();
				if(sorts) {
					for(set<Sort*>::const_iterator it = sorts->begin(); it != sorts->end(); ++it) os->insert(*it);
				}
				for(set<Predicate*>::const_iterator it = preds.begin(); it != preds.end(); ++it) os->insert(*it);
				for(set<Function*>::const_iterator it = funcs.begin(); it != funcs.end(); ++it) os->insert(*it);
				InternalArgument s(os);
				return convertToLua(L,s);
			}
		}
		else {
			lua_pushstring(L,"A vocabulary can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Index function for tuples
	 */
	int tupleIndex(lua_State* L) {
		ElementTuple* tuple = *(ElementTuple**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_INT) {
			const DomainElement* element = (*tuple)[index._value._int - 1];
			return convertToLua(L,element);
		}
		else {
			lua_pushstring(L,"A tuple can only be indexed by an integer");
			return lua_error(L);
		}
	}

	/**
	 * Length operator for tuples
	 */
	int tupleLen(lua_State* L) {
		ElementTuple* tuple = *(ElementTuple**)lua_touserdata(L,1);
		int length = tuple->size();
		lua_pushinteger(L,length);
		return 1;
	}

	/**
	 * Index function for predicate interpretations
	 */
	int predinterIndex(lua_State* L) {
		PredInter* predinter = *(PredInter**)lua_touserdata(L,1);
		InternalArgument index = InternalArgument(2,L);
		if(index._type == AT_STRING) {
			string str = *(index._value._string);
			if(str == "ct") {
				InternalArgument tab(predinter->ct());
				return convertToLua(L,tab);
			}
			else if(str == "pt") {
				InternalArgument tab(predinter->pt());
				return convertToLua(L,tab);
			}
			else if(str == "cf") {
				InternalArgument tab(predinter->cf());
				return convertToLua(L,tab);
			}
			else if(str == "pf") {
				InternalArgument tab(predinter->pf());
				return convertToLua(L,tab);
			}
			else {
				lua_pushstring(L,"A predicate interpretation can only be indexed by \"ct\", \"cf\", \"pt\", and \"pf\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A predicate interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Index function for function interpretations
	 */
	int funcinterIndex(lua_State* L) {
		FuncInter* funcinter = *(FuncInter**)lua_touserdata(L,1);
		InternalArgument index = InternalArgument(2,L);
		if(index._type == AT_STRING) {
			if(*(index._value._string) == "graph") {
				InternalArgument predinter(funcinter->graphinter());
				return convertToLua(L,predinter);
			}
			else {
				lua_pushstring(L,"A function interpretation can only be indexed by \"graph\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A function interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}
	/**
	 * Index function for structures
	 */
	int structureIndex(lua_State* L) {
		AbstractStructure* structure = *(AbstractStructure**)lua_touserdata(L,1);
		InternalArgument symbol(2,L);
		switch(symbol._type) {
			case AT_SORT:
			{
				set<Sort*>* ss = symbol._value._sort;
				if(ss->size() == 1) {
					Sort* s = *(ss->begin());
					SortTable* result = structure->inter(s);
					return convertToLua(L,InternalArgument(result));
				}
				break;
			}
			case AT_PREDICATE:
			{
				set<Predicate*>* sp = symbol._value._predicate;
				if(sp->size() == 1) {
					Predicate* p = *(sp->begin());
					PredInter* result = structure->inter(p);
					return convertToLua(L,InternalArgument(result));
				}
				break;
			}
			case AT_FUNCTION:
			{
				set<Function*>* sf = symbol._value._function;
				if(sf->size() == 1) {
					Function* f = *(sf->begin());
					FuncInter* result = structure->inter(f);
					return convertToLua(L,InternalArgument(result));
				}
				break;
			}
			default:
				break;
		}
		lua_pushstring(L,"A structure can only be indexed by a single type, predicate, or function symbol");
		return lua_error(L);
	}

	/**
	 * Index function for options
	 */
	int optionsIndex(lua_State* L) {
		Options* opts = *(Options**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_STRING) {
			return convertToLua(L,opts->getvalue(*(index._value._string)));
		}
		else {
			lua_pushstring(L,"Options can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Index function for namespaces
	 */
	int namespaceIndex(lua_State* L) {
		Namespace* ns = *(Namespace**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		if(index._type == AT_STRING) {
			string str = *(index._value._string);
			unsigned int counter = 0;
			Namespace* subsp = 0;
			if(ns->isSubspace(str)) { subsp = ns->subspace(str); ++counter; }
			Vocabulary* vocab = 0;
			if(ns->isVocab(str)) { vocab = ns->vocabulary(str); ++counter; }
			AbstractTheory* theo = 0;
			if(ns->isTheory(str)) { theo = ns->theory(str); ++counter; }
			AbstractStructure* structure = 0;
			if(ns->isStructure(str)) { structure = ns->structure(str); ++counter; }
			Options* opts = 0;
			if(ns->isOptions(str)) { opts = ns->options(str); ++counter; }
			UserProcedure* proc = 0;
			if(ns->isProc(str)) { proc = ns->procedure(str); ++counter; }

			if(counter == 0) return 0;
			else if(counter == 1) {
				if(subsp) return convertToLua(L,InternalArgument(subsp));
				if(vocab) return convertToLua(L,InternalArgument(vocab));
				if(theo) return convertToLua(L,InternalArgument(theo));
				if(structure) return convertToLua(L,InternalArgument(structure));
				if(opts) return convertToLua(L,InternalArgument(opts));
				if(proc) {
					proc->compile(L);
					lua_getfield(L,LUA_REGISTRYINDEX,proc->registryindex().c_str());
					return 1;
				}
				assert(false);
				return 0;
			}
			else {
				OverloadedObject* oo = new OverloadedObject();
				oo->insert(subsp);
				oo->insert(vocab);
				oo->insert(theo);
				oo->insert(structure);
				oo->insert(opts);
				oo->insert(proc);
				return convertToLua(L,InternalArgument(oo));
			}
		}
		else {
			lua_pushstring(L,"Namespaces can only be indexed by strings");
			return lua_error(L);
		}
	}

	SortTable* toDomain(vector<InternalArgument>* table, lua_State* L) {
		EnumeratedInternalSortTable* ist = new EnumeratedInternalSortTable();
		SortTable* st = new SortTable(ist);
		for(vector<InternalArgument>::const_iterator it = table->begin(); it != table->end(); ++it) {
			switch(it->_type) {
				case AT_INT:
					st->add(DomainElementFactory::instance()->create(it->_value._int)); break;
				case AT_DOUBLE:
					st->add(DomainElementFactory::instance()->create(it->_value._double)); break;
				case AT_STRING:
					st->add(DomainElementFactory::instance()->create(it->_value._string)); break;
				case AT_COMPOUND:
					st->add(DomainElementFactory::instance()->create(it->_value._compound)); break;
				default:
					delete(st);
					lua_pushstring(L,"Only numbers, strings, and compounds are allowed in a predicate table");
					lua_error(L);
					return 0;
			}
		}
		return st;
	}

	PredTable* toPredTable(vector<InternalArgument>* table, lua_State* L, const Universe& univ) {
		EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
		for(vector<InternalArgument>::const_iterator it = table->begin(); it != table->end(); ++it) {
			if(it->_type == AT_TABLE) {
				ElementTuple tuple;
				for(vector<InternalArgument>::const_iterator jt = it->_value._table->begin(); 
					jt != it->_value._table->end(); ++jt) {
					switch(jt->_type) {
						case AT_INT:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._int)); break;
						case AT_DOUBLE:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._double)); break;
						case AT_STRING:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._string)); break;
						case AT_COMPOUND:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._compound)); break;
						default:
							lua_pushstring(L,"Only numbers, strings, and compounds are allowed in a predicate table");
							lua_error(L);
							return 0;
					}
				}
				ipt->add(tuple);
			}
			else if(it->_type == AT_TUPLE) ipt->add(*(it->_value._tuple));
			else {
				lua_pushstring(L,"Expected a two-dimensional table");
				lua_error(L);
				return 0;
			}
		}
		PredTable* pt = new PredTable(ipt,univ);
		return pt;
	}

	/**
	 * NewIndex function for predicate interpretations
	 */
	int predinterNewIndex(lua_State* L) {
		PredInter* predinter = *(PredInter**)lua_touserdata(L,1);
		InternalArgument index = InternalArgument(2,L);
		InternalArgument value = InternalArgument(3,L);
		const Universe& univ = predinter->ct()->universe();
		if(index._type == AT_STRING) {
			const PredTable* pt = 0;
			if(value._type == AT_PREDTABLE) {
				pt = value._value._predtable;
			}
			else if(value._type == AT_TABLE) {
				pt = toPredTable(value._value._table,L,univ);
			}
			else {
				lua_pushstring(L,"Wrong argument to __newindex procedure of a predicate interpretation");
				return lua_error(L);
			}
			assert(pt);
			string str = *(index._value._string);
			if(str == "ct") predinter->ct(new PredTable(pt->interntable(),univ));
			else if(str == "pt") predinter->pt(new PredTable(pt->interntable(),univ));
			else if(str == "cf") predinter->cf(new PredTable(pt->interntable(),univ));
			else if(str == "pf") predinter->pf(new PredTable(pt->interntable(),univ));
			else {
				lua_pushstring(L,"A predicate interpretation can only be indexed by \"ct\", \"cf\", \"pt\", and \"pf\"");
				return lua_error(L);
			}
			return 0;
		}
		else {
			lua_pushstring(L,"A predicate interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * NewIndex function for function interpretations
	 */
	int funcinterNewIndex(lua_State* L) {
		FuncInter* funcinter = *(FuncInter**)lua_touserdata(L,1);
		InternalArgument index = InternalArgument(2,L);
		InternalArgument value = InternalArgument(3,L);
		if(index._type == AT_STRING) {
			if((*(index._value._string)) == "graph") {
				if(value._type == AT_PREDINTER) {
					funcinter->graphinter(value._value._predinter);
					return 0;
				}
				else {
					lua_pushstring(L,"Expected a predicate interpretation");
					return lua_error(L);
				}
			}
			else {
				lua_pushstring(L,"A function interpretation can only be indexed by \"graph\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A function interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * NewIndex function for structures
	 */
	int structureNewIndex(lua_State* L) {
		AbstractStructure* structure = *(AbstractStructure**)lua_touserdata(L,1);
		InternalArgument index = InternalArgument(2,L);
		InternalArgument value = InternalArgument(3,L);
		switch(index._type) {
			case AT_SORT:
			{
				set<Sort*>* ss = index._value._sort;
				if(ss->size() == 1) {
					Sort* s = *(ss->begin());
					if(value._type == AT_DOMAIN) {
						SortTable* st = structure->inter(s);
						st->interntable(value._value._domain->interntable());
						return 0;
					}
					else if(value._type == AT_TABLE) {
						SortTable* dom = toDomain(value._value._table,L);
						SortTable* st = structure->inter(s);
						st->interntable(dom->interntable());
						delete(dom);
						return 0;
					}
					else {
						lua_pushstring(L,"Expected a table or a domain");
						return lua_error(L);
					}
				}
				break;
			}
			case AT_PREDICATE:
			{
				set<Predicate*>* sp = index._value._predicate;
				if(sp->size() == 1) {
					Predicate* p = *(sp->begin());
					if(value._type == AT_PREDINTER) {
						structure->inter(p,value._value._predinter);
						return 0;
					}
					else {
						lua_pushstring(L,"Expected a predicate interpretation");
						return lua_error(L);
					}
				}
				break;
			}
			case AT_FUNCTION:
			{
				set<Function*>* sf = index._value._function;
				if(sf->size() == 1) {
					Function* f = *(sf->begin());
					if(value._type == AT_FUNCINTER) {
						structure->inter(f,value._value._funcinter);
						return 0;
					}
					else {
						lua_pushstring(L,"Expected a function interpretation");
						return lua_error(L);
					}
				}
				break;
			}
			default:
				break;
		}
		lua_pushstring(L,"A structure can only be indexed by a single type, predicate, or function symbol");
		return lua_error(L);
	}

	/**
	 * NewIndex function for options
	 */
	int optionsNewIndex(lua_State* L) {
		Options* opts = *(Options**)lua_touserdata(L,1);
		InternalArgument index(2,L);
		InternalArgument value(3,L);
		if(index._type == AT_STRING) {
			string str = *(index._value._string);
			switch(value._type) {
				case AT_INT: opts->setvalue(str,value._value._int); break;
				case AT_DOUBLE: opts->setvalue(str,value._value._double); break;
				case AT_STRING: opts->setvalue(str,*(value._value._string)); break;
				case AT_BOOLEAN: opts->setvalue(str,value._value._boolean); break;
				default:
					lua_pushstring(L,"Wrong option value");
					return lua_error(L);
			}
			return 0;
		}
		else {
			lua_pushstring(L,"Options can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Call function for predicate tables
	 */
	int predtableCall(lua_State* L) {
		PredTable* predtable = *(PredTable**)lua_touserdata(L,1);
		lua_remove(L,1);
		ElementTuple tuple;
		for(int n = 1; n <= lua_gettop(L); ++n) {
			InternalArgument currarg(n,L);
			switch(currarg._type) {
				case AT_INT:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._int)); break;
				case AT_DOUBLE:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._double)); break;
				case AT_STRING:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._string)); break;
				case AT_COMPOUND:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._compound)); break;
				case AT_TUPLE:
					if(n == 1 && lua_gettop(L) == 1) {
						tuple = *(currarg._value._tuple);
						break;
					}
				default:
					lua_pushstring(L,"Only numbers, strings, and compounds can be arguments of a predicate table");
					lua_error(L);
					return 0;
			}
		}
		lua_pushboolean(L,predtable->contains(tuple));
		return 1;
	}

	/**
	 * Call function for function interpretations
	 */
	int funcinterCall(lua_State* L) {
		FuncInter* funcinter = *(FuncInter**)lua_touserdata(L,1);
		if(funcinter->approxtwovalued()) {
			FuncTable* ft = funcinter->functable();
			lua_remove(L,1);
			unsigned int nrargs = lua_gettop(L);
			if(nrargs == 1) {
				InternalArgument argone(1,L);
				if(argone._type == AT_TUPLE) {
					ElementTuple tuple = *(argone._value._tuple);
					while(tuple.size() > ft->arity()) tuple.pop_back();
					while(tuple.size() < ft->arity()) tuple.push_back(0);
					const DomainElement* d = (*ft)[tuple];
					return convertToLua(L,d);
				}
			}
			ElementTuple tuple;
			for(unsigned int n = 1; n <= nrargs; ++n) {
				InternalArgument arg(n,L);
				switch(arg._type) {
					case AT_INT:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._int)); break;
					case AT_DOUBLE:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._double)); break;
					case AT_STRING:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._string)); break;
					case AT_COMPOUND:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._compound)); break;
					default:
						lua_pushstring(L,"Only numbers, strings, and compounds can be arguments of a function interpretation");
						lua_error(L);
						return 0;
				}
			}
			while(tuple.size() > ft->arity()) tuple.pop_back();
			while(tuple.size() < ft->arity()) tuple.push_back(0);
			const DomainElement* d = (*ft)[tuple];
			return convertToLua(L,d);
		}
		else {
			lua_pushstring(L,"Only two-valued function interpretations can be called");
			return lua_error(L);
		}
	}

	/**
	 * Arity function for predicate symbols
	 */
	int predicateArity(lua_State* L) {
		set<Predicate*>* pred = *(set<Predicate*>**)lua_touserdata(L,1);
		InternalArgument arity(2,L);
		if(arity._type == AT_INT) {
			set<Predicate*>* newpred = new set<Predicate*>();
			for(set<Predicate*>::const_iterator it = pred->begin(); it != pred->end(); ++it) {
				if((int)(*it)->arity() == arity._value._int) newpred->insert(*it);
			}
			InternalArgument np(newpred);
			return convertToLua(L,np);
		}
		else {
			lua_pushstring(L,"The arity of a predicate must be an integer");
			return lua_error(L);
		}
	}

	/**
	 * Arity function for function symbols
	 */
	int functionArity(lua_State* L) {
		set<Function*>* func = *(set<Function*>**)lua_touserdata(L,1);
		InternalArgument arity(2,L);
		if(arity._type == AT_TABLE) {
			if(arity._value._table->size() > 0) {
				if((*(arity._value._table))[0]._type == AT_INT) {
					int ar = (*(arity._value._table))[0]._value._int;
					set<Function*>* newfunc = new set<Function*>();
					for(set<Function*>::const_iterator it = func->begin(); it != func->end(); ++it) {
						if((int)(*it)->arity() == ar) newfunc->insert(*it);
					}
					InternalArgument nf(newfunc);
					return convertToLua(L,nf);
				}
			}
		}
		lua_pushstring(L,"The arity of a function must be of the form \'integer : 1\'");
		return lua_error(L);
	}

	/**
	 * Arity function for symbols
	 */
	int symbolArity(lua_State* L) {
		OverloadedSymbol* symb = *(OverloadedSymbol**)lua_touserdata(L,1);
		InternalArgument arity(2,L);
		if(arity._type == AT_TABLE) {
			convertToLua(L,InternalArgument(new set<Function*>(*(symb->funcs()))));
			lua_replace(L,1);
			return functionArity(L);
		}
		else if(arity._type == AT_INT) {
			convertToLua(L,InternalArgument(new set<Predicate*>(*(symb->preds()))));
			lua_replace(L,1);
			return predicateIndex(L);
		}
		else {
			lua_pushstring(L,"The arity of a symbol must be an integer or of the form \'integer : 1\'");
			return lua_error(L);
		}
	}

	/**
	 * Create metatable for internal procedures
	 */
	void internProcMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"internalprocedure"); assert(ok);
		lua_pushcfunction(L,&gcInternProc);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&internalCall);
		lua_setfield(L,-2,"__call");
		lua_pop(L,1);
	}

	/** 
	 * Create metatable for sorts
	 */
	void sortMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"sort"); assert(ok);
		lua_pushinteger(L,(int)AT_SORT);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcSort);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/** 
	 * Create metatable for predicate symbols
	 */
	void predicateMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"predicate_symbol"); assert(ok);
		lua_pushinteger(L,(int)AT_PREDICATE);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcPredicate);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&predicateIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&predicateArity);
		lua_setfield(L,-2,"__div");
		lua_pop(L,1);
	}

	/** 
	 * Create metatable for function symbols
	 */
	void functionMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"function_symbol"); assert(ok);
		lua_pushinteger(L,(int)AT_FUNCTION);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcFunction);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&functionIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&functionArity);
		lua_setfield(L,-2,"__div");
		lua_pop(L,1);
	}

	/**
	 * Create metatale for overloaded symbols
	 */
	void symbolMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"symbol"); assert(ok);
		lua_pushinteger(L,(int)AT_SYMBOL);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcSymbol);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&symbolIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&symbolArity);
		lua_setfield(L,-2,"__div");
		lua_pop(L,1);
	}

	/** 
	 * Create metatable for vocabularies
	 */
	void vocabularyMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"vocabulary"); assert(ok);
		lua_pushinteger(L,(int)AT_VOCABULARY);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcVocabulary);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&vocabularyIndex);
		lua_setfield(L,-2,"__index");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for compounds
	 */
	void compoundMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"compound"); assert(ok);
		lua_pushinteger(L,(int)AT_COMPOUND);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcCompound);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for tuples
	 */
	void tupleMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"tuple"); assert(ok);
		lua_pushinteger(L,(int)AT_TUPLE);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcTuple);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&tupleIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&tupleLen);
		lua_setfield(L,-2,"__len");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for domains
	 */
	void domainMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"domain"); assert(ok);
		lua_pushinteger(L,(int)AT_DOMAIN);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcDomain);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for predicate tables
	 */
	void predtableMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"predtable"); assert(ok);
		lua_pushinteger(L,(int)AT_PREDTABLE);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcPredTable);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&predtableCall);
		lua_setfield(L,-2,"__call");
		lua_pop(L,1);
	}

	/** 
	 * Create metatable for predicate interpretations
	 */
	void predinterMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"predinter"); assert(ok);
		lua_pushinteger(L,(int)AT_PREDINTER);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcPredInter);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&predinterIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&predinterNewIndex);
		lua_setfield(L,-2,"__newindex");
		lua_pop(L,1);
	}

	/** 
	 * Create metatable for function interpretations
	 */
	void funcinterMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"funcinter"); assert(ok);
		lua_pushinteger(L,(int)AT_FUNCINTER);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcFuncInter);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&funcinterIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&funcinterNewIndex);
		lua_setfield(L,-2,"__newindex");
		lua_pushcfunction(L,&funcinterCall);
		lua_setfield(L,-2,"__call");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for structures
	 */
	void structureMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"structure"); assert(ok);
		lua_pushinteger(L,(int)AT_STRUCTURE);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcStructure);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&structureIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&structureNewIndex);
		lua_setfield(L,-2,"__newindex");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for table iterators
	 */
	void tableiteratorMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"tableiterator"); assert(ok);
		lua_pushinteger(L,(int)AT_TABLEITERATOR);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcTableIterator);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for table iterators
	 */
	void domainiteratorMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"domainiterator"); assert(ok);
		lua_pushinteger(L,(int)AT_DOMAINITERATOR);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcDomainIterator);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for theories
	 */
	void theoryMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"theory"); assert(ok);
		lua_pushinteger(L,(int)AT_THEORY);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcTheory);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for options
	 */
	void optionsMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"options"); assert(ok);
		lua_pushinteger(L,(int)AT_OPTIONS);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcOptions);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&optionsIndex);
		lua_setfield(L,-2,"__index");
		lua_pushcfunction(L,&optionsNewIndex);
		lua_setfield(L,-2,"__newindex");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for namespaces
	 */
	void namespaceMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"namespace"); assert(ok);
		lua_pushinteger(L,(int)AT_NAMESPACE);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcNamespace);
		lua_setfield(L,-2,"__gc");
		lua_pushcfunction(L,&namespaceIndex);
		lua_setfield(L,-2,"__index");
		lua_pop(L,1);
	}

	/**
	 * Create metatable for overloaded objects
	 */
	void overloadedMetaTable(lua_State* L) {
		int ok = luaL_newmetatable(L,"overloaded"); assert(ok);
		lua_pushinteger(L,(int)AT_OVERLOADED);
		lua_setfield(L,-2,_typefield);
		lua_pushcfunction(L,&gcOverloaded);
		lua_setfield(L,-2,"__gc");
		lua_pop(L,1);
	}

	/**
	 * Create all metatables
	 */
	void createMetaTables(lua_State* L) {
		internProcMetaTable(L);

		sortMetaTable(L);
		predicateMetaTable(L);
		functionMetaTable(L);
		symbolMetaTable(L);
		vocabularyMetaTable(L);

		compoundMetaTable(L);
		tupleMetaTable(L);
		domainMetaTable(L);
		predtableMetaTable(L);
		predinterMetaTable(L);
		funcinterMetaTable(L);
		structureMetaTable(L);
		tableiteratorMetaTable(L);
		domainiteratorMetaTable(L);

		theoryMetaTable(L);
		optionsMetaTable(L);
		namespaceMetaTable(L);

		overloadedMetaTable(L);
	}

	/**
	 * map interal procedure names to the actual procedures
	 */
	map<string,map<vector<ArgType>,InternalProcedure*> >	_internalprocedures;
	
	/**
	 *	\brief Add a new internal procedure
	 *
	 *	\param name			the name of the internal procedure 
	 *	\param argtypes		the type of the arguments of the procedure
	 *	\param execute		the implementation of the procedure
	 *
	 */
	void addInternalProcedure(const string& name, 
					   const vector<ArgType>& argtypes, 
					   InternalArgument (*execute)(const vector<InternalArgument>&, lua_State*)) {
		InternalProcedure* proc = new InternalProcedure(name,argtypes,execute);
		_internalprocedures[name][argtypes] = proc;
	}

	/**
	 * Adds all internal procedures
	 */
	void addInternProcs(lua_State* L) {
		// arguments of internal procedures
		vector<ArgType> vempty(0);
		vector<ArgType> vint(1,AT_INT);
		vector<ArgType> vtheo(1,AT_THEORY);
		vector<ArgType> vstruct(1,AT_STRUCTURE);
		vector<ArgType> vspace(1,AT_NAMESPACE);
		vector<ArgType> vvoc(1,AT_VOCABULARY);
		vector<ArgType> vpredtable(1,AT_PREDTABLE);
		vector<ArgType> vdomain(1,AT_DOMAIN);
		vector<ArgType> vtabitertuple(2); vtabitertuple[0] = AT_TABLEITERATOR; vtabitertuple[1] = AT_TUPLE;
		vector<ArgType> vtheoopt(2); vtheoopt[0] = AT_THEORY; vtheoopt[1] = AT_OPTIONS;
		vector<ArgType> vstructopt(2); vstructopt[0] = AT_STRUCTURE; vstructopt[1] = AT_OPTIONS;
		vector<ArgType> vstructvoc(2); vstructvoc[0] = AT_STRUCTURE; vstructvoc[1] = AT_VOCABULARY;
		vector<ArgType> vvocopt(2); vvocopt[0] = AT_VOCABULARY; vvocopt[1] = AT_OPTIONS;
		vector<ArgType> voptopt(2); voptopt[0] = AT_OPTIONS; voptopt[1] = AT_OPTIONS;
		vector<ArgType> vspaceopt(2); vspaceopt[0] = AT_NAMESPACE; vspaceopt[1] = AT_OPTIONS;
		vector<ArgType> vintint(2); vintint[0] = AT_INT; vintint[1] = AT_INT;
		vector<ArgType> vdomiterint(2); vdomiterint[0] = AT_DOMAINITERATOR; vdomiterint[1] = AT_INT;
		vector<ArgType> vdomiterdouble(2); vdomiterdouble[0] = AT_DOMAINITERATOR; vdomiterdouble[1] = AT_DOUBLE;
		vector<ArgType> vdomiterstring(2); vdomiterstring[0] = AT_DOMAINITERATOR; vdomiterstring[1] = AT_STRING;
		vector<ArgType> vdomitercomp(2); vdomitercomp[0] = AT_DOMAINITERATOR; vdomitercomp[1] = AT_COMPOUND;
		vector<ArgType> vpritab(2); vpritab[0] = AT_PREDINTER; vpritab[1] = AT_TABLE;
		vector<ArgType> vpritup(2); vpritup[0] = AT_PREDINTER; vpritup[1] = AT_TUPLE;
		vector<ArgType> vtheostructopt(3); 
			vtheostructopt[0] = AT_THEORY; 
			vtheostructopt[1] = AT_STRUCTURE; 
			vtheostructopt[2] = AT_OPTIONS;

		// Create internal procedures
		addInternalProcedure("idptype",vint,&idptype);
		addInternalProcedure("tostring",vtheoopt,&printtheory);
		addInternalProcedure("tostring",vstructopt,&printstructure);
		addInternalProcedure("tostring",vvocopt,&printvocabulary);
		addInternalProcedure("namespace",vspaceopt,&printnamespace);
		addInternalProcedure("tostring",voptopt,&printoptions);
		addInternalProcedure("help",vspace,&help);
		addInternalProcedure("newstructure",vvoc,&newstructure);
		addInternalProcedure("newtheory",vvoc,&newtheory);
		addInternalProcedure("newoptions",vempty,&newoptions);
		addInternalProcedure("clone",vtheo,&clonetheory);
		addInternalProcedure("clone",vstruct,&clonestructure);
		addInternalProcedure("push_negations",vtheo,&pushnegations);
		addInternalProcedure("flatten",vtheo,&flatten);
		addInternalProcedure("mx",vtheostructopt,&modelexpand);
		addInternalProcedure("ground",vtheostructopt,&ground);
		addInternalProcedure("range",vintint,&createrange);
		addInternalProcedure("dummytuple",vempty,&createtuple);
		addInternalProcedure("deref_and_increment",vtabitertuple,&derefandincrement);
		addInternalProcedure("deref_and_increment",vdomiterint,&domderefandincrement);
		addInternalProcedure("deref_and_increment",vdomiterstring,&domderefandincrement);
		addInternalProcedure("deref_and_increment",vdomiterdouble,&domderefandincrement);
		addInternalProcedure("deref_and_increment",vdomitercomp,&domderefandincrement);
		addInternalProcedure("tableiterator",vpredtable,&tableiterator);
		addInternalProcedure("domainiterator",vdomain,&domainiterator);
		addInternalProcedure("changevocabulary",vstructvoc,&changevocabulary);
		addInternalProcedure("maketrue",vpritab,&maketabtrue);
		addInternalProcedure("maketrue",vpritup,&maketrue);
		addInternalProcedure("makefalse",vpritab,&maketabfalse);
		addInternalProcedure("makefalse",vpritup,&makefalse);
		
		// Add the internal procedures to lua
		lua_getglobal(L,"idp_intern");
		for(map<string,map<vector<ArgType>,InternalProcedure*> >::iterator it =	_internalprocedures.begin();
			it != _internalprocedures.end(); ++it) {
			map<vector<ArgType>,InternalProcedure*>** ptr = 
				(map<vector<ArgType>,InternalProcedure*>**)lua_newuserdata(L,sizeof(map<vector<ArgType>,InternalProcedure*>*));
			(*ptr) = new map<vector<ArgType>,InternalProcedure*>(it->second);
			luaL_getmetatable (L,"internalprocedure");
			lua_setmetatable(L,-2);
			lua_setfield(L,-2,it->first.c_str());
		}
		lua_pop(L,1);
	}

	/**
	 * Establish the connection with lua. 
	 */
	void makeLuaConnection() {
		// Create the lua state
		_state = lua_open();
		luaL_openlibs(_state);

		// Create all metatables
		createMetaTables(_state);

		// Create the global table idp_intern
		lua_newtable(_state);
		lua_setglobal(_state,"idp_intern");

		// Add internal procedures
		addInternProcs(_state);
		
		// Overwrite some standard lua procedures
		stringstream ss;
		ss << DATADIR << "/std/idp_intern.lua";
		int err = luaL_dofile(_state,ss.str().c_str());
		if(err) { cerr << "Error in idp_intern.lua\n"; exit(1); }

		// Add the global namespace and standard options
		addGlobal(Namespace::global());
		addGlobal(Namespace::global()->options("stdoptions"));

		// Parse standard input file
		stringstream ss1; 
		ss1 << DATADIR <<"/std/idp_intern.idp";
		parsefile(ss1.str());

		// Parse configuration file
		stringstream ss2;
		ss2 << RCDIR << "/idp/.idprc";
		err = luaL_dofile(_state,ss2.str().c_str());
		if(err) { cerr << "Error in configuration file\n"; exit(1); }

	}

	/**
	 * End the connection with lua
	 */
	void closeLuaConnection() {
		lua_close(_state);
		for(map<string,map<vector<ArgType>,InternalProcedure*> >::iterator it =	_internalprocedures.begin(); 
			it != _internalprocedures.end(); ++it) {
			for(map<vector<ArgType>,InternalProcedure*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
				delete(jt->second);
			}
		}
		delete(Namespace::global());
		delete(DomainElementFactory::instance());
	}

	void execute(stringstream* chunk) {
		int err = luaL_dostring(_state,chunk->str().c_str());
		if(err) {
			Error::error();
			cerr << string(lua_tostring(_state,-1)) << endl;
			lua_pop(_state,1);
		}
	}

	void pushglobal(const vector<string>& name, const ParseInfo& pi) {
		lua_getglobal(_state,name[0].c_str());
		for(unsigned int n = 1; n < name.size(); ++n) {
			if(lua_istable(_state,-1)) {
				lua_getfield(_state,-1,name[n].c_str());
				lua_remove(_state,-2);
			}
			else {
				Error::error(pi);
				cerr << "unknown object" << endl;
			}
		}
	}

	InternalArgument* call(const vector<string>& proc, const vector<vector<string> >& args, const ParseInfo& pi) {
		pushglobal(proc,pi);
		for(unsigned int n = 0; n < args.size(); ++n) pushglobal(args[n],pi);
		int err = lua_pcall(_state,args.size(),1,0);
		if(err) {
			Error::error(pi);
			cerr << lua_tostring(_state,-1) << endl;
			lua_pop(_state,1);
			return 0;
		}
		else {
			InternalArgument* ia = new InternalArgument(-1,_state);
			lua_pop(_state,1);
			return ia;
		}
	}

	const DomainElement* funccall(string* procedure, const ElementTuple& input) {
		lua_getfield(_state,LUA_REGISTRYINDEX,procedure->c_str());
		for(ElementTuple::const_iterator it = input.begin(); it != input.end(); ++it) {
			convertToLua(_state,*it);
		}
		int err = lua_pcall(_state,input.size(),1,0);
		if(err) {
			Error::error();
			cerr << string(lua_tostring(_state,-1)) << endl;
			lua_pop(_state,1);
			return 0;
		}
		else {
			const DomainElement* d = convertToElement(-1,_state);
			lua_pop(_state,1);
			return d;
		}
	}

	bool predcall(string* procedure, const ElementTuple& input) {
		lua_getfield(_state,LUA_REGISTRYINDEX,procedure->c_str());
		for(ElementTuple::const_iterator it = input.begin(); it != input.end(); ++it) {
			convertToLua(_state,*it);
		}
		int err = lua_pcall(_state,input.size(),1,0);
		if(err) {
			Error::error();
			cerr << string(lua_tostring(_state,-1)) << endl;
			lua_pop(_state,1);
			return 0;
		}
		else {
			bool b = lua_toboolean(_state,-1);
			lua_pop(_state,1);
			return b;
		}
	}

	void addGlobal(Vocabulary* v) {
		convertToLua(_state,InternalArgument(v));
		lua_setglobal(_state,v->name().c_str());
	}

	void addGlobal(AbstractStructure* s) {
		convertToLua(_state,InternalArgument(s));
		lua_setglobal(_state,s->name().c_str());
	}

	void addGlobal(AbstractTheory* t) {
		convertToLua(_state,InternalArgument(t));
		lua_setglobal(_state,t->name().c_str());
	}

	void addGlobal(Options* o) {
		convertToLua(_state,InternalArgument(o));
		lua_setglobal(_state,o->name().c_str());
	}

	void addGlobal(UserProcedure* p) {
		InternalArgument ia;
		ia._type = AT_PROCEDURE;
		ia._value._string = StringPointer(p->registryindex());
		convertToLua(_state,ia);
		lua_setglobal(_state,p->name().c_str());
	}

	void addGlobal(Namespace* n) {
		convertToLua(_state,InternalArgument(n));
		lua_setglobal(_state,n->name().c_str());
	}

	void compile(UserProcedure* proc) {
		proc->compile(_state);
	}

	AbstractStructure* structure(InternalArgument* arg) {
		switch(arg->_type) {
			case AT_STRUCTURE:
				return arg->_value._structure;
			default:
				return 0;
		}
	}

	AbstractTheory* theory(InternalArgument* arg) {
		switch(arg->_type) {
			case AT_THEORY:
				return arg->_value._theory;
			default:
				return 0;
		}
	}

	Vocabulary* vocabulary(InternalArgument* arg) {
		switch(arg->_type) {
			case AT_VOCABULARY:
				return arg->_value._vocabulary;
			default:
				return 0;
		}
	}

	string*	getProcedure(const std::vector<std::string>& name, const ParseInfo& pi) {
		pushglobal(name,pi);
		InternalArgument ia(-1,_state);
		lua_pop(_state,1);
		if(ia._type == AT_PROCEDURE) {
			return ia._value._string;
		}
		else return 0;
	}

}

int InternalProcedure::operator()(lua_State* L) const {
	vector<InternalArgument> args;
	for(int arg = 1; arg <= lua_gettop(L); ++arg) {
		args.push_back(InternalArgument(arg,L));
	}
	InternalArgument result = _execute(args,L);
	return LuaConnection::convertToLua(L,result);
}

InternalArgument Options::getvalue(const string& opt) const {
	map<string,bool>::const_iterator bit = _booloptions.find(opt);
	if(bit != _booloptions.end()) {
		return InternalArgument(bit->second);
	}
	map<string,IntOption*>::const_iterator iit = _intoptions.find(opt);
	if(iit != _intoptions.end()) {
		return InternalArgument(iit->second->value());
	}
	map<string,FloatOption*>::const_iterator fit = _floatoptions.find(opt);
	if(fit != _floatoptions.end()) {
		return InternalArgument(fit->second->value());
	}
	map<string,StringOption*>::const_iterator sit = _stringoptions.find(opt);
	if(sit != _stringoptions.end()) {
		return InternalArgument(StringPointer(sit->second->value()));
	}
	
	InternalArgument ia; ia._type = AT_NIL;
	return ia;
}






















#ifdef OLD



/*
	Connection with lua
*/


namespace BuiltinProcs {

	map<string,vector<Inference*> >	_inferences;	// All inference methods

	void initialize() {
		_inferences["print"].push_back(new PrintTheory());
		_inferences["print"].push_back(new PrintVocabulary());
		_inferences["print"].push_back(new PrintStructure());
		_inferences["print"].push_back(new PrintNamespace());
		_inferences["push_negations"].push_back(new PushNegations());
		_inferences["remove_equivalences"].push_back(new RemoveEquivalences());
		_inferences["remove_eqchains"].push_back(new RemoveEqchains());
		_inferences["flatten"].push_back(new FlattenFormulas());
		_inferences["convert_to_theory"].push_back(new StructToTheory());
		_inferences["move_quantifiers"].push_back(new MoveQuantifiers());
		_inferences["tseitin"].push_back(new ApplyTseitin());
		_inferences["reduce"].push_back(new GroundReduce());
		_inferences["move_functions"].push_back(new MoveFunctions());
		_inferences["load_file"].push_back(new LoadFile());
		_inferences["clone"].push_back(new CloneStructure());
		_inferences["clone"].push_back(new CloneTheory());
		_inferences["fastground"].push_back(new FastGrounding());
		_inferences["fastmx"].push_back(new FastMXInference());

		_inferences["newoptions"].push_back(new NewOption());

		_inferences["forcetwovalued"].push_back(new ForceTwoValued());
		_inferences["changevoc"].push_back(new ChangeVoc());
		_inferences["getbdds"].push_back(new BDDPrinter());

		_inferences["delete"].push_back(new DeleteData(IAT_THEORY));
		_inferences["delete"].push_back(new DeleteData(IAT_STRUCTURE));
		_inferences["delete"].push_back(new DeleteData(IAT_VOCABULARY));
		_inferences["delete"].push_back(new DeleteData(IAT_NAMESPACE));
		_inferences["delete"].push_back(new DeleteData(IAT_OPTIONS));
		_inferences["delete"].push_back(new DeleteData(IAT_PREDICATE));
		_inferences["delete"].push_back(new DeleteData(IAT_FUNCTION));
		_inferences["delete"].push_back(new DeleteData(IAT_SORT));
		_inferences["delete"].push_back(new DeleteData(IAT_PREDINTER));
		_inferences["delete"].push_back(new DeleteData(IAT_FUNCINTER));
		_inferences["delete"].push_back(new DeleteData(IAT_PREDTABLE));
		_inferences["delete"].push_back(new DeleteData(IAT_TUPLE));

		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_PREDICATE));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_FUNCTION));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_SORT));
		_inferences["index"].push_back(new GetIndex(IAT_NAMESPACE,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_VOCABULARY,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_OPTIONS,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_PREDINTER,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_FUNCINTER,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_PREDTABLE,IAT_INT));
		_inferences["index"].push_back(new GetIndex(IAT_TUPLE,IAT_INT));

		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_DOUBLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_BOOLEAN));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_STRING));

		_inferences["len"].push_back(new LenghtOperator(IAT_PREDTABLE));
		_inferences["len"].push_back(new LenghtOperator(IAT_TUPLE));

		_inferences["cast"].push_back(new CastOperator());
		_inferences["aritycast"].push_back(new ArityCastOperator(IAT_PREDICATE));
		_inferences["aritycast"].push_back(new ArityCastOperator(IAT_FUNCTION));
	}

	void cleanup() {
		for(map<string,vector<Inference*> >::iterator it = _inferences.begin(); it != _inferences.end(); ++it) {
			for(unsigned int n = 0; n < it->second.size(); ++n) {
				delete(it->second[n]);
			}
		}
	}

	string typestring(InfArgType t) {
		switch(t) {
			case IAT_THEORY: 
				return("theory");
			case IAT_STRUCTURE: 
				return("structure");
			case IAT_VOCABULARY: 
				return("vocabulary");
			case IAT_NAMESPACE: 
				return("namespace");
			case IAT_OPTIONS: 
				return("options");
			case IAT_NIL: 
				return("nil");
			case IAT_INT: 
				return("number");
			case IAT_DOUBLE:
				return("number");
			case IAT_BOOLEAN: 
				return("boolean");
			case IAT_STRING: 
				return("string");
			case IAT_TABLE: 
				return("table");
			case IAT_PROCEDURE: 
				return("function");
			case IAT_OVERLOADED: 
				return("overloaded");
			case IAT_SORT:
				return("type");
			case IAT_PREDICATE: 
				return("predicate_symbol");
			case IAT_FUNCTION:
				return("function_symbol");
			case IAT_PREDTABLE:
				return("predicate_table");
			case IAT_PREDINTER:
				return("predicate_interpretation");
			case IAT_FUNCINTER:
				return("function_interpretation");
			case IAT_TUPLE:
				return("tuple");
			case IAT_MULT:
				assert(false); return "mult";
			case IAT_REGISTRY:
				assert(false); return "registry";
			default:
				assert(false);
		}
	}

	string typestring(lua_State* L, int index) {
		lua_getglobal(L,"type");
		lua_pushvalue(L,index);
		lua_call(L,1,1);
		string str = string(lua_tostring(L,-1));
		lua_pop(L,1);
		return str;
	}

	InfArgType typeenum(const string& strtype) {
		if(strtype == "theory") return IAT_THEORY;
		if(strtype == "structure") return IAT_STRUCTURE;
		if(strtype == "vocabulary") return IAT_VOCABULARY;
		if(strtype == "namespace") return IAT_NAMESPACE;
		if(strtype == "options") return IAT_OPTIONS;
		if(strtype == "nil") return IAT_NIL;
		if(strtype == "number") return IAT_DOUBLE;
		if(strtype == "boolean") return IAT_BOOLEAN;
		if(strtype == "string") return IAT_STRING;
		if(strtype == "table") return IAT_TABLE;
		if(strtype == "predicate_table") return IAT_PREDTABLE;
		if(strtype == "function") return IAT_PROCEDURE;
		if(strtype == "overloaded") return IAT_OVERLOADED;
		if(strtype == "sort") return IAT_SORT;
		if(strtype == "predicate_symbol") return IAT_PREDICATE;
		if(strtype == "function_symbol") return IAT_FUNCTION;
		if(strtype == "predicate_interpretation") return IAT_PREDINTER;
		if(strtype == "function_interpretation") return IAT_FUNCINTER;
		if(strtype == "tuple") return IAT_TUPLE;
		if(strtype == "mult") return IAT_MULT;
		if(strtype == "registry") return IAT_REGISTRY;
		assert(false); return IAT_INT;
	}

	InfArgType typeenum(lua_State* L, int index) {
		string strtype = typestring(L,index);
		return typeenum(strtype);
	}

	bool checkarg(lua_State* L, int n, InfArgType t) {
		string typestr = typestring(t);
		string nstr = typestring(L,n);
		if(nstr == typestr) return true;
		else if(nstr == "overloaded") {
			OverloadedObject* obj = *((OverloadedObject**)lua_touserdata(L,n));
			switch(t) {
				case IAT_THEORY:
					return obj->isTheory();
				case IAT_STRUCTURE:
					return obj->isStructure();
				case IAT_VOCABULARY:
					return obj->isVocabulary();
				case IAT_NAMESPACE:
					return obj->isNamespace();
				case IAT_OPTIONS:
					return obj->isOptions();
				case IAT_PROCEDURE:
					return (obj->isProcedure());
				case IAT_SORT:
					return (obj->isSort());
				case IAT_PREDICATE:
					return (obj->isPredicate());
				case IAT_FUNCTION:
					return (obj->isFunction());
				default:
					return false;
			}
		}
		else return false;
	}


	int converttolua(lua_State* L, InfArg res, InfArgType t) {
		switch(t) {
			case IAT_THEORY:
			{
				AbstractTheory** ptr = (AbstractTheory**) lua_newuserdata(L,sizeof(AbstractTheory*));
				(*ptr) = res._theory;
				luaL_getmetatable (L,"theory");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_VOCABULARY:
			{
				Vocabulary** ptr = (Vocabulary**) lua_newuserdata(L,sizeof(Vocabulary*));
				(*ptr) = res._vocabulary;
				luaL_getmetatable (L,"vocabulary");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_STRUCTURE:
			{
				AbstractStructure** ptr = (AbstractStructure**) lua_newuserdata(L,sizeof(AbstractStructure*));
				(*ptr) = res._structure;
				luaL_getmetatable (L,"structure");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_NAMESPACE:
			{
				Namespace** ptr = (Namespace**) lua_newuserdata(L,sizeof(Namespace*));
				(*ptr) = res._namespace;
				luaL_getmetatable (L,"namespace");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_OPTIONS:
			{
				InfOptions** ptr = (InfOptions**) lua_newuserdata(L,sizeof(InfOptions*));
				(*ptr) = res._options;
				luaL_getmetatable (L,"options");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_SORT:
			{
				set<Sort*>** ptr = (set<Sort*>**) lua_newuserdata(L,sizeof(set<Sort*>*));
				(*ptr) = res._sort;
				luaL_getmetatable (L,"sort");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_PREDICATE:
			{
				set<Predicate*>** ptr = (set<Predicate*>**) lua_newuserdata(L,sizeof(set<Predicate*>*));
				(*ptr) = res._predicate;
				luaL_getmetatable (L,"predicate_symbol");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_FUNCTION:
			{
				set<Function*>** ptr = (set<Function*>**) lua_newuserdata(L,sizeof(set<Function*>*));
				(*ptr) = res._function;
				luaL_getmetatable (L,"function_symbol");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_PREDINTER:
			{
				PredInter** ptr = (PredInter**) lua_newuserdata(L,sizeof(set<PredInter*>*));
				(*ptr) = res._predinter;
				luaL_getmetatable (L,"predicate_interpretation");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_FUNCINTER:
			{
				FuncInter** ptr = (FuncInter**) lua_newuserdata(L,sizeof(set<FuncInter*>*));
				(*ptr) = res._funcinter;
				luaL_getmetatable (L,"function_interpretation");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_PREDTABLE:
			{
				PredTable** ptr = (PredTable**) lua_newuserdata(L,sizeof(set<PredTable*>*));
				(*ptr) = res._predtable;
				luaL_getmetatable (L,"predicate_table");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_TUPLE:
			{
				PredTableTuple** ptr = (PredTableTuple**) lua_newuserdata(L,sizeof(set<PredTableTuple*>*));
				(*ptr) = res._tuple;
				luaL_getmetatable (L,"tuple");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_NIL:
				lua_pushnil(L);
				break;
			case IAT_INT:
				lua_pushnumber(L,double(res._int));
				break;
			case IAT_DOUBLE:
				lua_pushnumber(L,res._double);
				break;
			case IAT_BOOLEAN:
				lua_pushboolean(L,res._boolean);
				break;
			case IAT_STRING:
				lua_pushstring(L,res._string->c_str());
				break;
			case IAT_TABLE:
				lua_newtable(L);
				for(unsigned int n = 0; n < res._table->size(); ++n) {
					lua_pushinteger(L,n+1);
					converttolua(L,(*(res._table))[n]._value,(*(res._table))[n]._type);
					lua_settable(L,-3);
				}
				break;
			case IAT_PROCEDURE:
			{
				lua_getfield(L,LUA_REGISTRYINDEX,res._procedure->c_str());
				break;
			}
			case IAT_MULT:
			{
				int nrres = 0;
				for(unsigned int n = 0; n < res._table->size(); ++n) {
					nrres += converttolua(L,(*(res._table))[n]._value,(*(res._table))[n]._type);
				}
				return nrres;
			}
			case IAT_REGISTRY:
			{
				lua_getfield(L,LUA_REGISTRYINDEX,res._string->c_str());
				break;
			}
			case IAT_OVERLOADED:
			{
				OverloadedObject* obj = res._overloaded;
				if(obj->single()) {
					InfArg newarg;
					if(obj->isNamespace())	{ 
						newarg._namespace = obj->getNamespace(); 
						converttolua(L,newarg,IAT_NAMESPACE);	
					}
					if(obj->isVocabulary()) { 
						newarg._vocabulary = obj->getVocabulary(); 
						converttolua(L,newarg,IAT_VOCABULARY);	
					}
					if(obj->isTheory()) { 
						newarg._theory = obj->getTheory(); 
						converttolua(L,newarg,IAT_THEORY);	
					}
					if(obj->isStructure()) { 
						newarg._structure = obj->getStructure(); 
						converttolua(L,newarg,IAT_STRUCTURE);	
					}
					if(obj->isOptions()) { 
						newarg._options = obj->getOptions(); 
						converttolua(L,newarg,IAT_OPTIONS);	
					}
					if(obj->isProcedure()) {
						newarg._procedure = obj->getProcedure();
						converttolua(L,newarg,IAT_PROCEDURE);	
					}
					if(obj->isPredicate()) {
						newarg._predicate = obj->getPredicate();
						converttolua(L,newarg,IAT_PREDICATE);	
					}
					if(obj->isFunction()) {
						newarg._function = obj->getFunction();
						converttolua(L,newarg,IAT_FUNCTION);	
					}
					if(obj->isSort()) {
						newarg._sort = obj->getSort();
						converttolua(L,newarg,IAT_SORT);
					}
				}
				else {
					OverloadedObject** ptr = (OverloadedObject**) lua_newuserdata(L,sizeof(OverloadedObject*));
					(*ptr) = obj;
					luaL_getmetatable(L,"overloaded");
					lua_setmetatable(L,-2);
				}
				break;
			}
			default:
				assert(false);
		}
		return 1;
	}

	InfArg convertarg(lua_State* L, int n, InfArgType t);

	TypedInfArg convertarg(lua_State* L, int n) {
		TypedInfArg result;
		result._type = typeenum(L,n);
		if(result._type == IAT_DOUBLE) {
			if(isInt(lua_tonumber(L,n))) result._type = IAT_INT;
		}
		result._value = convertarg(L,n,result._type);
		return result;
	}

	InfArg convertarg(lua_State* L, int n, InfArgType t) {
		string nstr = typestring(L,n);
		InfArg a;
		if(nstr != "overloaded") {
			switch(t) {
				case IAT_THEORY:
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
					break;
				case IAT_STRUCTURE:
					a._structure = *((AbstractStructure**)lua_touserdata(L,n));
					break;
				case IAT_VOCABULARY:
					a._vocabulary = *((Vocabulary**)lua_touserdata(L,n));
					break;
				case IAT_NAMESPACE:
					a._namespace = *((Namespace**)lua_touserdata(L,n));
					break;
				case IAT_OPTIONS:
					a._options = *((InfOptions**)lua_touserdata(L,n));
					break;
				case IAT_INT:
					a._int = int(lua_tonumber(L,n));
					break;
				case IAT_DOUBLE:
					a._double = lua_tonumber(L,n);
					break;
				case IAT_BOOLEAN:
					a._boolean = lua_toboolean(L,n);
					break;
				case IAT_STRING:
					a._string = IDPointer(lua_tostring(L,n));
					break;
				case IAT_SORT:
					a._sort = *((set<Sort*>**)lua_touserdata(L,n));
					break;
				case IAT_PREDICATE:
					a._predicate = *((set<Predicate*>**)lua_touserdata(L,n));
					break;
				case IAT_FUNCTION:
					a._function = *((set<Function*>**)lua_touserdata(L,n));
					break;
				case IAT_PREDINTER:
					a._predinter = *((PredInter**)lua_touserdata(L,n));
					break;
				case IAT_FUNCINTER:
					a._funcinter = *((FuncInter**)lua_touserdata(L,n));
					break;
				case IAT_PREDTABLE:
					a._predtable = *((PredTable**)lua_touserdata(L,n));
					break;
				case IAT_TUPLE:
					a._tuple = *((PredTableTuple**)lua_touserdata(L,n));
					break;
				case IAT_TABLE:
				{
					a._table = new vector<TypedInfArg>;
					for(unsigned int i = 1; ; ++i) {
						lua_pushinteger(L,i);
						lua_gettable(L,n);
						if(lua_isnil(L,-1)) {
							lua_pop(L,1);
							break;
						}
						else {
							a._table->push_back(convertarg(L,-1));
							lua_pop(L,1);
						}
					}
					break;
				}
				case IAT_PROCEDURE:
				{
					string registryindex = "idp_argument_procedure" + itos(LuaArgProcNumber);
					++LuaArgProcNumber;
					lua_pushvalue(L,n);
					lua_setfield(L,LUA_REGISTRYINDEX,registryindex.c_str());
					break;
				}
				default:
					assert(false);
			}
		}
		else {
			OverloadedObject* obj = *((OverloadedObject**)lua_touserdata(L,n)); 
			switch(t) {
				case IAT_THEORY:
					a._theory = obj->getTheory();
					break;
				case IAT_STRUCTURE:
					a._structure = obj->getStructure();
					break;
				case IAT_VOCABULARY:
					a._vocabulary = obj->getVocabulary();
					break;
				case IAT_NAMESPACE:
					a._namespace = obj->getNamespace();
					break;
				case IAT_OPTIONS:
					a._options = obj->getOptions();
					break;
				case IAT_SORT:
					a._sort = obj->getSort();
					break;
				case IAT_PREDICATE:
					a._predicate = obj->getPredicate();
					break;
				case IAT_FUNCTION:
					a._function = obj->getFunction();
					break;
				case IAT_PROCEDURE:
					a._procedure = obj->getProcedure();
					break;
				case IAT_OVERLOADED:
					a._overloaded = obj;
					break;
				default:
					assert(false);
			}

		}
		return a;
	}

}

int overloaddiv(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_OVERLOADED);
	OverloadedObject* obj = a._overloaded;
	InfArg b = BuiltinProcs::convertarg(L,2,IAT_INT);
	unsigned int div = b._int;
	set<Predicate*>* sp = 0;
	set<Function*>* sf = 0;
	if(obj->isPredicate()) {
		for(set<Predicate*>::iterator it = obj->getPredicate()->begin(); it != obj->getPredicate()->end(); ++it) {
			if((*it)->arity() == div) {
				if(!sp) sp = new set<Predicate*>();
				sp->insert(*it);
			}
		}
	}
	if(obj->isFunction()) {
		for(set<Function*>::iterator it = obj->getFunction()->begin(); it != obj->getFunction()->end(); ++it) {
			if((*it)->arity() == div) {
				if(!sf) sf = new set<Function*>();
				sf->insert(*it);
			}
		}
	}
	if(sp) {
		if(sf) {
			OverloadedObject* newobj = new OverloadedObject();
			newobj->setpredicate(sp);
			newobj->setfunction(sf);
			InfArg res; res._overloaded = newobj;
			BuiltinProcs::converttolua(L,res,IAT_OVERLOADED);
		}
		else {
			InfArg res; res._predicate = sp;
			BuiltinProcs::converttolua(L,res,IAT_PREDICATE);
		}
	}
	else if(sf) {
		InfArg res; res._function = sf;
		BuiltinProcs::converttolua(L,res,IAT_FUNCTION);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

int overloadcall(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_OVERLOADED);
	if(a._overloaded->isProcedure()) {
		lua_getfield(L,LUA_REGISTRYINDEX,a._overloaded->getProcedure()->c_str());
		lua_replace(L,1);
		int nrargs = lua_gettop(L) -1; 
		lua_call(L,nrargs,LUA_MULTRET);
		return lua_gettop(L);
	}
	else {
		Error::notcommand();
		return 0;
	}
}

int idppredcall(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_PREDTABLE);
	PredTable* pt = a._predtable;
	vector<TypedElement> args;
	lua_remove(L,1);
	unsigned int nrargs = lua_gettop(L);
	for(unsigned int n = 1; n <= nrargs; ++n) {
		TypedInfArg tia = BuiltinProcs::convertarg(L,n);
		TypedElement te;
		switch(tia._type) {
			case IAT_INT:
				te._type = ELINT;
				te._element._int = tia._value._int;
				break;
			case IAT_DOUBLE:
				te._type = ELDOUBLE;
				te._element._double = tia._value._double;
				break;
			case IAT_STRING:
				te._type = ELSTRING;
				te._element._string = tia._value._string;
				break;
			default:
				te._type = ELINT;
				te._element = ElementUtil::nonexist(ELINT);
				break;
		}
		args.push_back(te);
		}
		while(args.size() > pt->arity()) args.pop_back();
		while(args.size() < pt->arity()) {
			TypedElement te;
			te._type = ELINT;
			te._element = ElementUtil::nonexist(ELINT);
			args.push_back(te);
		}
		if(pt->contains(args)) lua_pushboolean(L,1);
		else lua_pushboolean(L,0);
		return 1;
}

int idpfunccall(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_FUNCINTER);
	FuncTable* ft = a._funcinter->functable();
	if(ft) {
		vector<TypedElement> args;
		lua_remove(L,1);
		unsigned int nrargs = lua_gettop(L);
		for(unsigned int n = 1; n <= nrargs; ++n) {
			TypedInfArg tia = BuiltinProcs::convertarg(L,n);
			TypedElement te;
			switch(tia._type) {
				case IAT_INT:
					te._type = ELINT;
					te._element._int = tia._value._int;
					break;
				case IAT_DOUBLE:
					te._type = ELDOUBLE;
					te._element._double = tia._value._double;
					break;
				case IAT_STRING:
					te._type = ELSTRING;
					te._element._string = tia._value._string;
					break;
				default:
					te._type = ELINT;
					te._element = ElementUtil::nonexist(ELINT);
					break;
			}
			args.push_back(te);
		}
		while(args.size() > ft->arity()) args.pop_back();
		while(args.size() < ft->arity()) {
			TypedElement te;
			te._type = ELINT;
			te._element = ElementUtil::nonexist(ELINT);
			args.push_back(te);
		}
		Element e = (*ft)[args];
		if(ElementUtil::exists(e,ft->outtype())) {
			switch(ft->outtype()) {
				case ELINT:
					lua_pushinteger(L,e._int);
					break;
				case ELDOUBLE:
					lua_pushnumber(L,e._double);
					break;
				case ELSTRING:
					lua_pushstring(L,e._string->c_str());
					break;
				case ELCOMPOUND:
					lua_pushstring(L,IDPointer(ElementUtil::ElementToString(e,ELCOMPOUND))->c_str());
					break;
				default:
					assert(false);
			}
		}
		else lua_pushnil(L);
	}
	else {
		Error::threevalcall();
		lua_pushnil(L);
	}
	return 1;
}

int idpcall(lua_State* L) {
	// Collect name
	string name = string(lua_tostring(L,1));
	lua_remove(L,1);

	// Try to find correct inference 
	vector<Inference*> pvi = BuiltinProcs::_inferences[name];
	unsigned int nrargs = lua_gettop(L); 
	vector<Inference*> vi;
	for(unsigned int n = 0; n < pvi.size(); ++n) {
		if(nrargs == pvi[n]->arity()) vi.push_back(pvi[n]);
	}
	if(vi.empty()) {
		name += "/" + itos(nrargs);
		Error::unkncommand(name);
		return 0;
	}
	vector<Inference*> vi2;
	for(unsigned int n = 0; n < vi.size(); ++n) {
		bool ok = true;
		for(unsigned int m = 1; m <= nrargs; ++m) {
			ok = BuiltinProcs::checkarg(L,m,(vi[n]->intypes())[m-1]);
			if(!ok) break;
		}
		if(ok) vi2.push_back(vi[n]);
	}
	if(vi2.empty()) {
		Error::wrongcommandargs(name + '/' + itos(nrargs));
		return 0;
	}
	else if(vi2.size() == 1) {
		vector<InfArg> via;
		for(unsigned int m = 1; m <= nrargs; ++m)
			via.push_back(BuiltinProcs::convertarg(L,m,(vi2[0]->intypes())[m-1]));
		TypedInfArg res = vi2[0]->execute(via,L);
		return BuiltinProcs::converttolua(L,res._value,res._type);
	}
	else {
		Error::ambigcommand(name + '/' + itos(nrargs));
		return 0;
	}
}



/*
	Built-in procedures
*/

extern void parsefile(const string&);
TypedInfArg LoadFile::execute(const vector<InfArg>& args, lua_State* L) const {
	parsefile(*(args[0]._string));
	Namespace::global()->toLuaGlobal(L);
	TypedInfArg a; a._type = IAT_NIL; 
	return a;
}

TypedInfArg DeleteData::execute(const vector<InfArg>& args, lua_State*) const {
	switch(_intypes[0]) {
		case IAT_THEORY:
			if(!(args[0]._theory->pi().line())) delete(args[0]._theory);
			break;
		case IAT_STRUCTURE:
			if(!(args[0]._structure->pi().line())) delete(args[0]._structure);
			break;
		case IAT_NAMESPACE:
			if(!(args[0]._namespace->pi().line())) delete(args[0]._namespace);
			break;
		case IAT_VOCABULARY:
			if(!(args[0]._vocabulary->pi().line())) {
				if(args[0]._vocabulary != Namespace::global()->vocabulary("std")) delete(args[0]._vocabulary);
			}
			break;
		case IAT_OPTIONS:
			if(!(args[0]._options->pi().line())) delete(args[0]._options);
			break;
		case IAT_PREDICATE: case IAT_FUNCTION: case IAT_SORT:
			// do nothing
			break;
		case IAT_TUPLE:
			delete(args[0]._tuple);
			break;
		case IAT_PREDINTER:
			// TODO
			break;
		case IAT_FUNCINTER:
			// TODO
			break;
		case IAT_PREDTABLE:
			// TODO
			break;
		default:
			assert(false);
	}
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg NewOption::execute(const vector<InfArg>&, lua_State*) const {
	TypedInfArg a; a._type = IAT_OPTIONS;
	a._value._options = new InfOptions("",ParseInfo());
	return a;
}

TypedInfArg PrintTheory::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._theory);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintVocabulary::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._vocabulary);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintStructure::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._structure);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintNamespace::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._namespace);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PushNegations::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg RemoveEquivalences::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_equiv(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg FlattenFormulas::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::flatten(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg RemoveEqchains::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_eqchains(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

FastMXInference::FastMXInference() {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_intypes[2] = IAT_OPTIONS;
	_description = "Performs model expansion on the structure given the theory it should satisfy.";
}

class TraceWriter {
	private:
		GroundTranslator*	_translator;
		lua_State*			L;
		string*				_registryindex;
		static int			_tracenr;
		int					_timepoint;

	public:
		TraceWriter(GroundTranslator* trans, lua_State* Ls) : _translator(trans), L(Ls), _timepoint(1) { 
			++_tracenr;
			_registryindex = IDPointer(string("trace") + itos(_tracenr));
			lua_newtable(L);
			lua_setfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
		}

		void backtrack(int a){
			lua_getfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
			lua_pushinteger(L,_timepoint);
			++_timepoint;
			lua_newtable(L);
			lua_pushstring(L,"backtrack");
			lua_setfield(L,-2,"type");
			lua_pushinteger(L,a);
			lua_setfield(L,-2,"dl");
			lua_settable(L,-3);
			lua_pop(L,1);
		}

		void propagate(MinisatID::Literal a, int b){
			lua_getfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
			lua_pushinteger(L,_timepoint);
			++_timepoint;
			lua_newtable(L);
			lua_pushstring(L,"assign");
			lua_setfield(L,-2,"type");
			lua_pushinteger(L,b);
			lua_setfield(L,-2,"dl");
			lua_pushboolean(L,!a.hasSign());
			lua_setfield(L,-2,"value");
			// TODO: change next two lines to push real atoms
			lua_pushstring(L,_translator->printAtom(a.getAtom().getValue()).c_str());
			lua_setfield(L,-2,"atom");
			lua_settable(L,-3);
			lua_pop(L,1);
		}

		TypedInfArg trace() const {
			TypedInfArg trace; trace._type = IAT_REGISTRY; trace._value._string = _registryindex;	
			return trace;
		}
};

int TraceWriter::_tracenr = 0;

TypedInfArg FastMXInference::execute(const vector<InfArg>& args, lua_State* L) const {

	// Convert arguments
	AbstractTheory* theory = args[0]._theory;
	AbstractStructure* structure = args[1]._structure;
	InfOptions* opts = args[2]._options;
	
	// Create solver
	MinisatID::SolverOption modes;
	modes.nbmodels = opts->_nrmodels;
	modes.verbosity = opts->_satverbosity;
	modes.remap = false;
	//TODO pass other solver options from opts->_solveroptions
	SATSolver* solver = new SATSolver(modes);

	// Create grounder
	GrounderFactory gf(structure,opts->_cpsupport);
	TopLevelGrounder* grounder = gf.create(theory,solver);

	// Ground
	grounder->run();
	assert(typeid(*(grounder->grounding())) == typeid(SolverTheory));
	SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

	// Create monitor
	TraceWriter tracewriter(grounding->translator(),L);
	if(opts->_trace) {
		cb::Callback1<void, int> callbackback(&tracewriter, &TraceWriter::backtrack);
		cb::Callback2<void, MinisatID::Literal, int> callbackprop(&tracewriter, &TraceWriter::propagate);
		MinisatID::Monitor* m = new MinisatID::Monitor();
		m->setBacktrackCB(callbackback);
		m->setPropagateCB(callbackprop);
		solver->addMonitor(m);
	}

	// Add function constraints
	grounding->addFuncConstraints();
	grounding->addFalseDefineds();

	// Solve
	vector<MinisatID::Literal> assumpts;
	MinisatID::ModelExpandOptions options;
	options.nbmodelstofind = modes.nbmodels;
	options.printmodels = MinisatID::PRINT_NONE;
	options.savemodels = MinisatID::SAVE_ALL;
	options.search = MinisatID::MODELEXPAND;
	MinisatID::Solution* sol = new MinisatID::Solution(options);
	solver->solve(sol);

	// Translate
	TypedInfArg a; a._type = IAT_TABLE; a._value._table = new vector<TypedInfArg>();
	if(sol->isSat()){
		for(vector<MinisatID::Model*>::const_iterator modelit = sol->getModels().begin(); modelit != sol->getModels().end(); ++modelit) {
//cerr << "---Building new model---" << endl;
			AbstractStructure* mod = structure->clone();
			set<PredInter*>	tobesorted1;
			set<FuncInter*>	tobesorted2;
//cerr << "-Normal SAT part-" << endl;
			for(vector<MinisatID::Literal>::const_iterator literalit = (*modelit)->literalinterpretations.begin();
					literalit != (*modelit)->literalinterpretations.end(); ++literalit) {
				PFSymbol* pfs = grounding->translator()->symbol(((*literalit).getAtom().getValue()));
				if(pfs && mod->vocabulary()->contains(pfs)) {
					vector<domelement> vd = grounding->translator()->args((*literalit).getAtom().getValue());
					vector<TypedElement> args = ElementUtil::convert(vd);
					if(pfs->ispred()) {
						mod->inter(pfs)->add(args,!((*literalit).hasSign()),true);
						tobesorted1.insert(mod->inter(pfs));
					}
					else {
						Function* function = dynamic_cast<Function*>(pfs);
//if(!((*literalit).hasSign())) cerr << "Adding value " << args.back()._element._int << " for function " << function->name() << endl;
//else cerr << "Adding impossible value " << args.back()._element._int << " for function " << function->name() << endl;
						mod->inter(function)->add(args,!((*literalit).hasSign()),true);
						tobesorted2.insert(mod->inter(function));
					}
				}
			}
//cerr << "-CP part-" << endl;
			for(vector<MinisatID::VariableEqValue>::const_iterator cpvarit = (*modelit)->variableassignments.begin();
					cpvarit != (*modelit)->variableassignments.end(); ++cpvarit) {
				Function* function = grounding->termtranslator()->function((*cpvarit).variable);
				if(function && mod->vocabulary()->contains(function)) {
					vector<domelement> vd = grounding->termtranslator()->args((*cpvarit).variable);
					vector<TypedElement> args = ElementUtil::convert(vd);
					TypedElement value((*cpvarit).value);
					args.push_back(value);
//cerr << "Adding value " << args.back()._element._int << " for function " << function->name() << endl;
					mod->inter(function)->add(args,true,true);
					tobesorted2.insert(mod->inter(function));
				}
			}
			for(set<PredInter*>::const_iterator it=tobesorted1.begin(); it != tobesorted1.end(); ++it)
				(*it)->sortunique();
			for(set<FuncInter*>::const_iterator it=tobesorted2.begin(); it != tobesorted2.end(); ++it)
				(*it)->sortunique();

			if(opts->_modelformat == MF_TWOVAL) {
				mod->forcetwovalued();
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._value._table->push_back(b);
			}
			else if(opts->_modelformat == MF_ALL) {
				// TODO
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._value._table->push_back(b);
			}
			else {
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._value._table->push_back(b);
			}
		}
	}

	// Return answer
	if(opts->_trace) {
		TypedInfArg b; b._type = IAT_MULT; b._value._table = new vector<TypedInfArg>(1,a);
		b._value._table->push_back(tracewriter.trace());
		return b;
	}
	else {
		return a;
	}
}

TypedInfArg StructToTheory::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	AbstractTheory* t = StructUtils::convert_to_theory(args[0]._structure);
	TypedInfArg a; a._type = IAT_THEORY; a._value._theory = t;
	return a;
}

TypedInfArg MoveQuantifiers::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::move_quantifiers(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg ApplyTseitin::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::tseitin(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

GroundReduce::GroundReduce() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_description = "Replace ground atoms in the theory by their truth value in the structure";
}

TypedInfArg GroundReduce::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::reduce(args[0]._theory,args[1]._structure);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg MoveFunctions::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::move_functions(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg CloneStructure::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg a; 
	a._type = IAT_STRUCTURE; a._value._structure = args[0]._structure->clone();
	return a;
}

TypedInfArg CloneTheory::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg a;
	a._type = IAT_THEORY; a._value._theory = args[0]._theory->clone();
	return a;
}

FastGrounding::FastGrounding() {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_intypes[2] = IAT_OPTIONS;
	_description = "Ground the theory and structure and store the grounding";
}

TypedInfArg FastGrounding::execute(const vector<InfArg>& args, lua_State*) const {
	GrounderFactory factory(args[1]._structure,args[2]._options->_cpsupport);
	TopLevelGrounder* grounder = factory.create(args[0]._theory);
	grounder->run();
	TypedInfArg a; a._type = IAT_THEORY;
	a._value._theory = grounder->grounding();
	return a;
}

TypedInfArg ForceTwoValued::execute(const vector<InfArg>& args, lua_State*) const {
	args[0]._structure->forcetwovalued();
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

ChangeVoc::ChangeVoc() {
	_intypes.push_back(IAT_STRUCTURE);
	_intypes.push_back(IAT_VOCABULARY);
	_description = "Change the vocabulary of a structure";
}

TypedInfArg ChangeVoc::execute(const vector<InfArg>& args, lua_State*) const {
	AbstractStructure* str = args[0]._structure;
	Vocabulary* v = args[1]._vocabulary;
	StructUtils::changevoc(str,v);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg GetIndex::execute(const vector<InfArg>& args, lua_State* L) const {
	TypedInfArg a;
	switch(_intypes[0]) {
		case IAT_STRUCTURE:
		{
			AbstractStructure* as = args[0]._structure;
			switch(_intypes[1]) {
				case IAT_SORT:
					a = as->getObject(args[1]._sort);
					break;
				case IAT_PREDICATE:
					a = as->getObject(args[1]._predicate);
					break;
				case IAT_FUNCTION:
					a = as->getObject(args[1]._function);
					break;
				default:
					assert(false);
			}
			break;
		}
		case IAT_NAMESPACE:
		{
			Namespace* nsp = args[0]._namespace;
			string str = *(args[1]._string);
			a = nsp->getObject(str,L);
			break;
		}
		case IAT_VOCABULARY:
		{
			Vocabulary* voc = args[0]._vocabulary;
			string str = *(args[1]._string);
			a = voc->getObject(str);
			break;
		}
		case IAT_OPTIONS:
			a = args[0]._options->get(*(args[1]._string));
			break;
		case IAT_PREDINTER:
		{
			PredInter* pinter = args[0]._predinter;
			string str = *(args[1]._string);
			a._type = IAT_PREDTABLE;
			if(str == "ct") {
				if(pinter->ct()) a._value._predtable = pinter->ctpf();
				else {
					// TODO
					assert(false);
				}
			}
			else if(str == "cf") {
				if(pinter->cf()) a._value._predtable = pinter->cfpt();
				else {
					// TODO
					assert(false);
				}
			}
			else if(str == "pt") {
				if(!(pinter->cf())) a._value._predtable = pinter->cfpt();
				else {
					// TODO
					assert(false);
				}
			}
			else if(str == "pf") {
				if(!(pinter->ct())) a._value._predtable = pinter->ctpf();
				else {
					// TODO
					assert(false);
				}
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		case IAT_FUNCINTER:
		{
			FuncInter* finter = args[0]._funcinter;
			string str = *(args[1]._string);
			if(str == "graph") {
				a._type = IAT_PREDINTER;
				a._value._predinter = finter->predinter();
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		case IAT_PREDTABLE:
		{
			PredTable* pt = args[0]._predtable;
			unsigned int index = args[1]._int - 1;
			if(pt->finite() && index < pt->size()) {
				if(pt->arity() == 1) {
					Element e = pt->element(index,0);
					switch(pt->type(0)) {
						case ELINT: 
							a._type = IAT_INT;
							a._value._int = e._int;
							break;
						case ELDOUBLE: 
							a._type = IAT_DOUBLE;
							a._value._int = e._double;
							break;
						case ELSTRING:
							a._type = IAT_STRING;
							a._value._string = e._string;
							break;
						case ELCOMPOUND:
							a._type = IAT_STRING;
							a._value._string = IDPointer(ElementUtil::ElementToString(e,ELCOMPOUND));
							break;
						default:
							assert(false);
					}
				}
				else {
					a._type = IAT_TUPLE;
					a._value._tuple = new PredTableTuple(pt,index);
				}
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		case IAT_TUPLE:
		{
			PredTableTuple* ptt = args[0]._tuple;
			unsigned int column = args[1]._int - 1;
			if(column < ptt->_table->size()) {
				Element e = ptt->_table->element(ptt->_index,column);
				switch(ptt->_table->type(column)) {
					case ELINT: 
						a._type = IAT_INT;
						a._value._int = e._int;
						break;
					case ELDOUBLE: 
						a._type = IAT_DOUBLE;
						a._value._int = e._double;
						break;
					case ELSTRING:
						a._type = IAT_STRING;
						a._value._string = e._string;
						break;
					case ELCOMPOUND:
						a._type = IAT_STRING;
						a._value._string = IDPointer(ElementUtil::ElementToString(e,ELCOMPOUND));
						break;
					default:
						assert(false);
				}
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		default:
			assert(false);
	}
	return a;
}

TypedInfArg BDDPrinter::execute(const vector<InfArg>& args, lua_State*) const {
	AbstractTheory* theory = args[0]._theory;
	FOBDDManager manager;
	FOBDDFactory factory(&manager,theory->vocabulary());
	FOBDD* result = manager.truebdd();
	for(unsigned int n = 0; n < theory->nrSentences(); ++n) {
		theory->sentence(n)->accept(&factory);
		result = manager.conjunction(result,factory.bdd());
	}
	// TODO: assert that there are no definitions and no fixpoint definitions
	TypedInfArg a; a._type = IAT_STRING;
	a._value._string = new string(manager.to_string(result));
	return a;
}

SetIndex::SetIndex(InfArgType table, InfArgType key, InfArgType value) {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = table; _intypes[1] = key; _intypes[2] = value;
	_description = "newindex";
}

TypedInfArg SetIndex::execute(const vector<InfArg>& args, lua_State*) const {
	switch(_intypes[0]) {
		case IAT_OPTIONS:
			switch(_intypes[2]) {
				case IAT_DOUBLE:
					if(isInt(args[2]._double)) {
						args[0]._options->set(*(args[1]._string),int(args[2]._double));
					}
					else {
						args[0]._options->set(*(args[1]._string),args[2]._double);
					}
					break;
				case IAT_STRING:
					args[0]._options->set(*(args[1]._string),*(args[2]._string));
					break;
				case IAT_BOOLEAN:
					args[0]._options->set(*(args[1]._string),args[2]._boolean);
					break;
				default:
					assert(false);
			}
			break;
		case IAT_STRUCTURE:
			// TODO
			assert(false);
			break;
		default:
			assert(false);
	}
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg LenghtOperator::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg a;
	a._type = IAT_INT;
	if(_intypes[0] == IAT_PREDTABLE) {
		if(args[0]._predtable->finite()) {
			a._value._int = args[0]._predtable->size();
		}
		else {
			a._value._string = IDPointer(string("infinite"));
			a._type = IAT_STRING;
		}
	}
	else {
		a._value._int = args[0]._tuple->_table->arity();
	}
	return a;
}

TypedInfArg ArityCastOperator::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg result; result._type = IAT_NIL;
	unsigned int arity = args[1]._int;
	switch(_intypes[0]) {
		case IAT_PREDICATE:
		{
			set<Predicate*>* preds = args[0]._predicate;
			set<Predicate*>* newpreds = 0;
			for(set<Predicate*>::iterator it = preds->begin(); it != preds->end(); ++it) {
				if((*it)->arity() == arity) {
					if(!newpreds) {
						newpreds = new set<Predicate*>();
						result._type = IAT_PREDICATE;
						result._value._predicate = newpreds;
					}
					newpreds->insert(*it);
				}
			}
			break;
		}
		case IAT_FUNCTION:
		{
			set<Function*>* funcs = args[0]._function;
			set<Function*>* newfuncs = 0;
			for(set<Function*>::iterator it = funcs->begin(); it != funcs->end(); ++it) {
				if((*it)->arity() == arity) {
					if(!newfuncs) {
						newfuncs = new set<Function*>();
						result._type = IAT_FUNCTION;
						result._value._function = newfuncs;
					}
					newfuncs->insert(*it);
				}
			}
			break;
		}
		default:
			break;
	}
	return result;
}

TypedInfArg CastOperator::execute(const vector<InfArg>& args, lua_State*) const {
	OverloadedObject* obj = args[0]._overloaded;
	InfArgType newtype = BuiltinProcs::typeenum(*(args[1]._string));
	TypedInfArg result;
	result._type = IAT_NIL;
	switch(newtype) {
		case IAT_THEORY:
			if(obj->isTheory()) {
				result._type = IAT_THEORY;
				result._value._theory = obj->getTheory();
			}
			break;
		case IAT_STRUCTURE: 
			if(obj->isTheory()) {
				result._type = IAT_STRUCTURE;
				result._value._structure = obj->getStructure();
			}
			break;
		case IAT_VOCABULARY:
			if(obj->isVocabulary()) {
				result._type = IAT_VOCABULARY;
				result._value._vocabulary = obj->getVocabulary();
			}
			break;
		case IAT_NAMESPACE: 
			if(obj->isNamespace()) {
				result._type = IAT_NAMESPACE;
				result._value._namespace = obj->getNamespace();
			}
			break;
		case IAT_OPTIONS:
			if(obj->isOptions()) {
				result._type = IAT_OPTIONS;
				result._value._options = obj->getOptions();
			}
			break;
		case IAT_PROCEDURE:
			if(obj->isProcedure()) {
				result._type = IAT_PROCEDURE;
				result._value._procedure = obj->getProcedure();
			}
			break;
		case IAT_SORT:
			if(obj->isSort()) {
				result._type = IAT_SORT;
				result._value._sort = obj->getSort();
			}
			break;
		case IAT_PREDICATE:
			if(obj->isPredicate()) {
				result._type = IAT_PREDICATE;
				result._value._predicate = obj->getPredicate();
			}
			break;
		case IAT_FUNCTION:
			if(obj->isFunction()) {
				result._type = IAT_FUNCTION;
				result._value._function = obj->getFunction();
			}
			break;
		default:
			break;
	}
	return result;
}

/*************************
	Overloaded objects
*************************/

void OverloadedObject::makepredicate(Predicate* p) {
	if(!_predicate) _predicate = new set<Predicate*>();
	_predicate->insert(p);
}

void OverloadedObject::makefunction(Function* f) {
	if(!_function) _function = new set<Function*>();
	_function->insert(f);
}

void OverloadedObject::makesort(Sort* s) {
	if(!_sort) _sort = new set<Sort*>();
	_sort->insert(s);
}

bool OverloadedObject::isPredicate() const {
	if(_predicate) return (!(_predicate->empty()));
	else return false;
}

bool OverloadedObject::isFunction() const {
	if(_function) return (!(_function->empty()));
	else return false;
}

bool OverloadedObject::isSort() const {
	if(_sort) return (!(_sort->empty()));
	else return false;
}

bool OverloadedObject::single() const {
	unsigned int count = 0;
	if(isNamespace()) ++count;
	if(isVocabulary()) ++count;
	if(isTheory()) ++count;	
	if(isStructure()) ++count;
	if(isOptions()) ++count;		
	if(isProcedure()) ++count;	
	if(isPredicate()) ++count;	
	if(isFunction()) ++count;	
	if(isSort()) ++count;		
	return count <= 1;
}

/*********************
	Lua Procedures
*********************/


TypedInfArg InfOptions::get(const string& opt) const {
	TypedInfArg tia;
	if(opt == "language") {
		tia._type = IAT_STRING;
		switch(_format) {
			case OF_IDP: tia._value._string = IDPointer(string("idp")); break;
			case OF_TXT: tia._value._string = IDPointer(string("txt")); break;
			default: assert(false); tia._type = IAT_NIL; break;
		}
	}
	else if(opt == "modelformat") {
		tia._type = IAT_STRING;
		switch(_modelformat) {
			case MF_ALL: tia._value._string = IDPointer(string("all")); break;
			case MF_TWOVAL: tia._value._string = IDPointer(string("twovalued")); break;
			case MF_THREEVAL: tia._value._string = IDPointer(string("threevalued")); break;
			default: assert(false); tia._type = IAT_NIL; break;
		}
	}
	else if(opt == "nrmodels") {
		tia._type = IAT_INT;
		tia._value._int = _nrmodels;
	}
	else if(opt == "satverbosity") {
		tia._type = IAT_INT;
		tia._value._int = _satverbosity;
	}
	else if(opt == "printtypes") {
		tia._type = IAT_BOOLEAN;
		tia._value._boolean = _printtypes ? true : false;
	}
	else {
		Error::unknopt(opt,0);
		tia._type = IAT_NIL;
	}
	return tia;
}

TypedInfArg AbstractStructure::getObject(set<Sort*>* sort) const {
	TypedInfArg a;
	if(sort->size() == 1) {
		a._type = IAT_PREDTABLE;
		a._value._predtable = inter(*(sort->begin()));
	}
	else {
		Error::indexoverloadedsort();
		a._type = IAT_NIL;
	}
	return a;
}

TypedInfArg AbstractStructure::getObject(set<Predicate*>* predicate) const {
	TypedInfArg a;
	if(predicate->size() == 1) {
		a._type = IAT_PREDINTER;
		a._value._predinter = inter(*(predicate->begin()));
	}
	else {
		Error::indexoverloadedpred();
		a._type = IAT_NIL;
	}
	return a;
}

TypedInfArg AbstractStructure::getObject(set<Function*>* function) const {
	TypedInfArg a;
	if(function->size() == 1) {
		a._type = IAT_FUNCINTER;
		a._value._funcinter = inter(*(function->begin()));
	}
	else {
		Error::indexoverloadedfunc();
		a._type = IAT_NIL;
	}
	return a;
}

TypedInfArg Vocabulary::getObject(const string& str) const {
	TypedInfArg a;
	OverloadedObject* obj = new OverloadedObject();
	vector<Predicate*> vp = pred_no_arity(str);
	vector<Function*> vf = func_no_arity(str);
	const set<Sort*>* ss = sort(str);
	for(vector<Predicate*>::const_iterator it = vp.begin(); it != vp.end(); ++it) {
		obj->makepredicate(*it);
	}
	for(vector<Function*>::const_iterator it = vf.begin(); it != vf.end(); ++it) {
		obj->makefunction(*it);
	}
	if(ss) {
		for(set<Sort*>::const_iterator it = ss->begin(); it != ss->end(); ++it) {
			obj->makesort(*it);
		}
	}
	if(obj->single()) {
		if(obj->isPredicate()) {
			a._type = IAT_PREDICATE;
			a._value._predicate = obj->getPredicate();
		}
		else if(obj->isFunction()) {
			a._type = IAT_FUNCTION;
			a._value._function = obj->getFunction();
		}
		else if(obj->isSort()) {
			a._type = IAT_SORT;
			a._value._sort = obj->getSort();
		}
		else {
			a._type = IAT_NIL;
		}
		delete(obj);
	}
	else {
		a._type = IAT_OVERLOADED;
		a._value._overloaded = obj;
	}
	return a;
}

TypedInfArg Namespace::getObject(const string& str, lua_State* L) const {
	TypedInfArg a;
	OverloadedObject* obj = new OverloadedObject();
	if(isSubspace(str)) obj->makenamespace(subspace(str));
	if(isVocab(str)) obj->makevocabulary(vocabulary(str));
	if(isTheory(str)) obj->maketheory(theory(str));
	if(isStructure(str)) obj->makestructure(structure(str));
	if(isOption(str)) obj->makeoptions(option(str));
	if(isProc(str)) {
		LuaProcedure* proc = procedure(str);
		proc->compile(L);
		obj->makeprocedure(&(proc->registryindex()));
	}
	if(obj->single()) {
		if(obj->isTheory()) {
			a._type = IAT_THEORY;
			a._value._theory = obj->getTheory();
		}
		else if(obj->isVocabulary()) {
			a._type = IAT_VOCABULARY;
			a._value._vocabulary = obj->getVocabulary();
		}
		else if(obj->isStructure()) {
			a._type = IAT_STRUCTURE;
			a._value._structure = obj->getStructure();
		}
		else if(obj->isNamespace()) {
			a._type = IAT_NAMESPACE;
			a._value._namespace = obj->getNamespace();
		}
		else if(obj->isOptions()) {
			a._type = IAT_OPTIONS;
			a._value._options = obj->getOptions();
		}
		else if(obj->isProcedure()) {
			a._type = IAT_PROCEDURE;
			a._value._procedure = obj->getProcedure();
		}
		else {
			a._type = IAT_NIL;
		}
		delete(obj);
	}
	else {
		a._type = IAT_OVERLOADED;
		a._value._overloaded = obj;
	}
	return a;
}



/*
 * void Namespace::toLuaGlobal(lua_State* L) {
 * DESCRIPTION
 *		Loads all objects of the namespaces as global variables in a given lua state.
 * PARAMETERS
 *		L	- the given lua state
 */
void Namespace::toLuaGlobal(lua_State* L) {

	set<string> doublenames = doubleNames();
	map<string,OverloadedObject*> doubleobjects;
	for(set<string>::const_iterator it = doublenames.begin(); it != doublenames.end(); ++it) 
		doubleobjects[*it] = new OverloadedObject();

	Namespace** ptr = (Namespace**) lua_newuserdata(L,sizeof(Namespace*));
	(*ptr) = this;
	luaL_getmetatable (L,"namespace");
	lua_setmetatable(L,-2);
	lua_setglobal(L,name().c_str());

	for(unsigned int n = 0; n < nrSubs(); ++n) {
		if(doublenames.find(subspace(n)->name()) == doublenames.end()) {
			Namespace** ptr = (Namespace**) lua_newuserdata(L,sizeof(Namespace*));
			(*ptr) = subspace(n);
			luaL_getmetatable (L,"namespace");
			lua_setmetatable(L,-2);
			lua_setglobal(L,subspace(n)->name().c_str());
		}
		else doubleobjects[subspace(n)->name()]->makenamespace(subspace(n));
	}

	for(unsigned int n = 0; n < nrVocs(); ++n) {
		if(doublenames.find(vocabulary(n)->name()) == doublenames.end()) {
			Vocabulary** ptr = (Vocabulary**) lua_newuserdata(L,sizeof(Vocabulary*));
			(*ptr) = vocabulary(n);
			luaL_getmetatable (L,"vocabulary");
			lua_setmetatable(L,-2);
			lua_setglobal(L,vocabulary(n)->name().c_str());
		}
		else doubleobjects[vocabulary(n)->name()]->makevocabulary(vocabulary(n));
	}

	for(unsigned int n = 0; n < nrStructs(); ++n) {
		if(doublenames.find(structure(n)->name()) == doublenames.end()) {
			AbstractStructure** ptr = (AbstractStructure**) lua_newuserdata(L,sizeof(AbstractStructure*));
			(*ptr) = structure(n);
			luaL_getmetatable (L,"structure");
			lua_setmetatable(L,-2);
			lua_setglobal(L,structure(n)->name().c_str());
		}
		else doubleobjects[structure(n)->name()]->makestructure(structure(n));
	}

	for(unsigned int n = 0; n < nrTheos(); ++n) {
		if(doublenames.find(theory(n)->name()) == doublenames.end()) {
			AbstractTheory** ptr = (AbstractTheory**) lua_newuserdata(L,sizeof(AbstractTheory*));
			(*ptr) = theory(n);
			luaL_getmetatable (L,"theory");
			lua_setmetatable(L,-2);
			lua_setglobal(L,theory(n)->name().c_str());
		}
		else doubleobjects[theory(n)->name()]->maketheory(theory(n));
	}

	for(map<string,Options*>::const_iterator it = _options.begin(); it != _options.end(); ++it) {
		if(doublenames.find(it->first) == doublenames.end()) {
			Options** ptr = (Options**) lua_newuserdata(L,sizeof(Options*));
			(*ptr) = it->second;
			luaL_getmetatable (L,"options");
			lua_setmetatable(L,-2);
			lua_setglobal(L,it->first.c_str());
		}
		else doubleobjects[it->first]->makeoptions(it->second);
	}

	for(map<string,UserProcedure*>::const_iterator it = _procedures.begin(); it != _procedures.end(); ++it) {
		if(doublenames.find(it->second->name()) == doublenames.end()) {
			it->second->compile(L);
			lua_getfield(L,LUA_REGISTRYINDEX,it->second->registryindex().c_str());
			lua_setglobal(L,it->second->name().c_str());
		}
		else {
			it->second->compile(L);
			doubleobjects[it->second->name()]->makeprocedure(new string(it->second->registryindex()));
		}
	}

	for(map<string,OverloadedObject*>::const_iterator it = doubleobjects.begin(); it != doubleobjects.end(); ++it) {
		OverloadedObject** ptr = (OverloadedObject**) lua_newuserdata(L,sizeof(OverloadedObject*));
		(*ptr) = it->second;
		luaL_getmetatable(L,"overloaded");
		lua_setmetatable(L,-2);
		lua_setglobal(L,it->first.c_str());
	}
}

/*
 * set<string> Namespace::doubleNames() const {
 * DESCRIPTION
 *		Collects the object names that occur more than once in the namespace
 */
set<string> Namespace::doubleNames() const {
	set<string> allnames;
	set<string> doublenames;
	for(unsigned int n = 0; n < nrSubs(); ++n) {
		if(allnames.find(subspace(n)->name()) == allnames.end()) allnames.insert(subspace(n)->name());
		else doublenames.insert(subspace(n)->name());
	}
	for(unsigned int n = 0; n < nrVocs(); ++n) {
		if(allnames.find(vocabulary(n)->name()) == allnames.end()) allnames.insert(vocabulary(n)->name());
		else doublenames.insert(vocabulary(n)->name());
	}
	for(unsigned int n = 0; n < nrStructs(); ++n) {
		if(allnames.find(structure(n)->name()) == allnames.end()) allnames.insert(structure(n)->name());
		else doublenames.insert(structure(n)->name());
	}
	for(unsigned int n = 0; n < nrTheos(); ++n) {
		if(allnames.find(theory(n)->name()) == allnames.end()) allnames.insert(theory(n)->name());
		else doublenames.insert(theory(n)->name());
	}
	for(map<string,Options*>::const_iterator it = _options.begin(); it != _options.end(); ++it) {
		if(allnames.find(it->first) == allnames.end()) allnames.insert(it->first);
		else doublenames.insert(it->first);
	}
	for(map<string,UserProcedure*>::const_iterator it = _procedures.begin(); it != _procedures.end(); ++it) {
		if(allnames.find(it->second->name()) == allnames.end()) allnames.insert(it->second->name());
		else doublenames.insert(it->second->name());
	}
	return doublenames;
}


#endif
