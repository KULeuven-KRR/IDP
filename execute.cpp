/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "print.hpp"
#include "data.hpp"
#include "lua.hpp"
#include "error.hpp"
#include "fobdd.hpp"

/*
	Connection with lua
*/

namespace BuiltinProcs {

	map<string,vector<Inference*> >	_inferences;	// All inference methods

	void initialize() {
		_inferences["print"].push_back(new PrintTheory(false));
		_inferences["print"].push_back(new PrintVocabulary(false));
		_inferences["print"].push_back(new PrintStructure(false));
		_inferences["print"].push_back(new PrintNamespace(false));
		_inferences["print"].push_back(new PrintTheory(true));
		_inferences["print"].push_back(new PrintVocabulary(true));
		_inferences["print"].push_back(new PrintStructure(true));
		_inferences["print"].push_back(new PrintNamespace(true));
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
		_inferences["fastmx"].push_back(new FastMXInference(false));
		_inferences["fastmx"].push_back(new FastMXInference(true));

		_inferences["newoptions"].push_back(new NewOption(true));
		_inferences["newoptions"].push_back(new NewOption(false));

		_inferences["forcetwovalued"].push_back(new ForceTwoValued());
		_inferences["changevoc"].push_back(new ChangeVoc());
		_inferences["getbdds"].push_back(new BDDPrinter());

		_inferences["delete"].push_back(new DeleteData(IAT_THEORY));
		_inferences["delete"].push_back(new DeleteData(IAT_STRUCTURE));
		_inferences["delete"].push_back(new DeleteData(IAT_VOCABULARY));
		_inferences["delete"].push_back(new DeleteData(IAT_NAMESPACE));
		_inferences["delete"].push_back(new DeleteData(IAT_OPTIONS));

		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_PREDICATE));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_FUNCTION));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_SORT));
		_inferences["index"].push_back(new GetIndex(IAT_NAMESPACE,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_VOCABULARY,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_OPTIONS,IAT_STRING));

		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_INT));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_DOUBLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_BOOLEAN));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_STRING));
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
			case IAT_IDPOBJECT: 
				return("overloaded");
			case IAT_SORT:
				return("sort");
			case IAT_PREDICATE: 
				return("predicate");
			case IAT_FUNCTION:
				return("idpfunction");
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


	bool checkarg(lua_State* L, int n, InfArgType t) {
		string typestr = typestring(t);
		string nstr = typestring(L,n);
		if(nstr == typestr) return true;
		else if(nstr == "overloaded") {
			overloadedObject* obj = *((overloadedObject**)lua_touserdata(L,n));
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
				case IAT_INT:
					return obj->isInt();
				case IAT_DOUBLE:
					return obj->isDouble();
				case IAT_BOOLEAN:
					return (obj->isBool());
				case IAT_STRING:
					return (obj->isString());
				case IAT_PROCEDURE:
					return (obj->isProcedure());
				case IAT_SORT:
					return (obj->isSort());
				case IAT_PREDICATE:
					return (obj->isPredicate());
				case IAT_FUNCTION:
					return (obj->isFunction());
				case IAT_TABLE:
					// TODO
				default:
					return false;
			}
		}
		else return false;
	}


	void converttolua(lua_State* L, InfArg res, InfArgType t) {
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
				// TODO
				assert(false);
			case IAT_PROCEDURE:
				// TODO
				assert(false);
			case IAT_IDPOBJECT:
			{
				overloadedObject* obj = res._overloaded;
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
					if(obj->isString()) {
						newarg._string = obj->getString();
						converttolua(L,newarg,IAT_STRING);
					}
					if(obj->isBool()) {
						newarg._boolean = obj->getBool();
						converttolua(L,newarg,IAT_BOOLEAN);
					}
					if(obj->isDouble()) {
						newarg._double = obj->getDouble();
						converttolua(L,newarg,IAT_DOUBLE);
					}
					if(obj->isInt()) {
						newarg._int = obj->getInt();
						converttolua(L,newarg,IAT_INT);
					}
				}
				else {
					overloadedObject** ptr = (overloadedObject**) lua_newuserdata(L,sizeof(overloadedObject*));
					(*ptr) = obj;
					luaL_getmetatable(L,"overloaded");
					lua_setmetatable(L,-2);
				}
				break;
			}
			default:
				assert(false);
		}
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
					a._sort = *((Sort**)lua_touserdata(L,n));
					break;
				case IAT_PREDICATE:
					a._predicate = *((Predicate**)lua_touserdata(L,n));
					break;
				case IAT_FUNCTION:
					a._function = *((Function**)lua_touserdata(L,n));
					break;
				case IAT_TABLE:
					// TODO
					assert(false);
				case IAT_PROCEDURE:
					// TODO
					assert(false);
				default:
					assert(false);
			}
		}
		else {
			overloadedObject* obj = *((overloadedObject**)lua_touserdata(L,n)); 
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
				case IAT_INT:
					a._int = obj->getInt();
					break;
				case IAT_DOUBLE:
					a._double = obj->getDouble();
					break;
				case IAT_BOOLEAN:
					a._boolean = obj->getBool();
					break;
				case IAT_STRING:
					a._string = obj->getString();
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
				case IAT_TABLE:
					// TODO
					assert(false);
				default:
					assert(false);
			}

		}
		return a;
	}

}

int idpcall(lua_State* L) {
	// Collect name
	string name = string(lua_tostring(L,1));
	lua_remove(L,1);
	vector<Inference*> pvi = BuiltinProcs::_inferences[name];

	// Try to find correct inference 
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
		InfArg res = vi2[0]->execute(via);
		if(vi2[0]->reload()) (Namespace::global())->tolua(L);
		BuiltinProcs::converttolua(L,res,vi2[0]->outtype());
		return 1;
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
InfArg LoadFile::execute(const vector<InfArg>& args) const {
	parsefile(*(args[0]._string));
	InfArg a; 
	return a;
}

InfArg DeleteData::execute(const vector<InfArg>& args) const {
	switch(_intypes[0]) {
		case IAT_THEORY:
			delete(args[0]._theory);
			break;
		case IAT_STRUCTURE:
			delete(args[0]._structure);
			break;
		case IAT_NAMESPACE:
			delete(args[0]._namespace);
			break;
		case IAT_VOCABULARY:
			delete(args[0]._vocabulary);
			break;
		case IAT_OPTIONS:
			delete(args[0]._options);
			break;
		default:
			assert(false);
	}
	InfArg a;
	return a;
}

SetOption::SetOption(InfArgType t) {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = IAT_OPTIONS;
	_intypes[1] = IAT_STRING;
	_intypes[2] = t;
	_outtype = IAT_NIL;
	_description = "Set the value of a single option";
	_reload = false;
}

GetOption::GetOption() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_OPTIONS;
	_intypes[1] = IAT_STRING;
	_outtype = IAT_IDPOBJECT;
	_description = "Get the value of a single option";
	_reload = false;
}

extern void setoption(InfOptions*,const string&, const string&, ParseInfo*);
extern void setoption(InfOptions*,const string&, double, ParseInfo*);
extern void setoption(InfOptions*,const string&, int, ParseInfo*);
extern void setoption(InfOptions*,const string&, bool, ParseInfo*);
extern string getoption(InfOptions*,const string&);

InfArg SetOption::execute(const vector<InfArg>& args) const {
	InfOptions* opts = args[0]._options;
	string optname = *(args[1]._string);
	switch(_intypes[2]) {
		case IAT_INT:
		{
			setoption(opts,optname,args[2]._int,0);
			break;
		}
		case IAT_DOUBLE:
		{
			setoption(opts,optname,args[2]._double,0);
			break;
		}
		case IAT_STRING:
		{
			setoption(opts,optname,*(args[2]._string),0);
			break;
		}
		case IAT_BOOLEAN:
		{
			setoption(opts,optname,args[2]._boolean,0);
			break;
		}
		default:
			assert(false);
	}
	InfArg a;
	return a;
}

InfArg GetOption::execute(const vector<InfArg>& args) const {
	InfOptions* opts = args[0]._options;
	string optname = *(args[1]._string);
	InfArg a;
	a._string = IDPointer(getoption(opts,optname));
	return a;
}

InfArg NewOption::execute(const vector<InfArg>& args) const {
	InfArg a;
	if(args.empty()) {
		a._options = new InfOptions("",ParseInfo());
	}
	else {
		InfOptions* opts = args[0]._options;
		a._options = new InfOptions(opts);
	}
	return a;
}

InfArg PrintTheory::execute(const vector<InfArg>& args) const {
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 2) opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	AbstractTheory* t = args[0]._theory;
	string str = printer->print(t);
	delete(printer);
	InfArg a; a._string = IDPointer(str);
	return a;
}

InfArg PrintVocabulary::execute(const vector<InfArg>& args) const {
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 2) opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._vocabulary);
	delete(printer);
	InfArg a; a._string = IDPointer(str);
	return a;
}

InfArg PrintStructure::execute(const vector<InfArg>& args) const {
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 2) opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	AbstractStructure* s = args[0]._structure;
	string str = printer->print(s);
	delete(printer);
	InfArg a; a._string = IDPointer(str);
	return a;
}

InfArg PrintNamespace::execute(const vector<InfArg>&) const {
	// TODO
	// string s = (args[0]._namespace)->to_string();
	//cout << s;
	InfArg a; a._string = IDPointer(string(""));
	return a;
}

InfArg PushNegations::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
	InfArg a;
	return a;
}

InfArg RemoveEquivalences::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::remove_equiv(args[0]._theory);
	InfArg a;
	return a;
}

InfArg FlattenFormulas::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::flatten(args[0]._theory);
	InfArg a;
	return a;
}

InfArg RemoveEqchains::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::remove_eqchains(args[0]._theory);
	InfArg a;
	return a;
}

FastMXInference::FastMXInference(bool opts) {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	if(opts) _intypes.push_back(IAT_OPTIONS);
	_outtype = IAT_TABLE;
	_description = "Performs model expansion on the structure given the theory it should satisfy.";
	_reload = false;
}

InfArg FastMXInference::execute(const vector<InfArg>& args) const {

	// Convert arguments
	AbstractTheory* theory = args[0]._theory;
	AbstractStructure* structure = args[1]._structure;
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 3) opts = args[2]._options;
	
	// Create solver
	MinisatID::SolverOption modes;
	modes.nbmodels = opts->_nrmodels;
	modes.verbosity = opts->_satverbosity;
	modes.remap = false;
	SATSolver* solver = new SATSolver(modes);

	// Create grounder
	GrounderFactory gf(structure);
	TopLevelGrounder* grounder = gf.create(theory,solver);

	// Ground
	grounder->run();
	assert(typeid(*(grounder->grounding())) == typeid(SolverTheory));
	SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

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
	bool sat = solver->solve(sol);

	// Translate
	InfArg a; a._table = new vector<TypedInfArg>();
	if(sat){
		for(unsigned int i=0; i<sol->getModels().size(); i++){
			AbstractStructure* mod = structure->clone();
			set<PredInter*>	tobesorted1;
			set<FuncInter*>	tobesorted2;
			for(unsigned int j=0; j<sol->getModels()[i].size(); j++) {
				PFSymbol* pfs = grounding->translator()->symbol((sol->getModels()[i][j].getAtom().getValue()));
				if(pfs && mod->vocabulary()->contains(pfs)) {
					vector<domelement> vd = grounding->translator()->args(sol->getModels()[i][j].getAtom().getValue());
					vector<TypedElement> args = ElementUtil::convert(vd);
					if(pfs->ispred()) {
						mod->inter(pfs)->add(args,!(sol->getModels()[i][j].hasSign()),true);
						tobesorted1.insert(mod->inter(pfs));
					}
					else {
						Function* f = dynamic_cast<Function*>(pfs);
						mod->inter(f)->add(args,!(sol->getModels()[i][j].hasSign()),true);
						tobesorted2.insert(mod->inter(f));
					}
				}
			}
			for(set<PredInter*>::const_iterator it=tobesorted1.begin(); it != tobesorted1.end(); ++it)
				(*it)->sortunique();
			for(set<FuncInter*>::const_iterator it=tobesorted2.begin(); it != tobesorted2.end(); ++it)
				(*it)->sortunique();

			if(opts->_modelformat == MF_TWOVAL) {
				mod->forcetwovalued();
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._table->push_back(b);
			}
			else if(opts->_modelformat == MF_ALL) {
				// TODO
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._table->push_back(b);
			}
			else {
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._table->push_back(b);
			}
		}
	}
	return a;

}

InfArg StructToTheory::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	AbstractTheory* t = StructUtils::convert_to_theory(args[0]._structure);
	InfArg a; a._theory = t;
	return a;
}

InfArg MoveQuantifiers::execute(const vector<InfArg>& args) const {
	TheoryUtils::move_quantifiers(args[0]._theory);
	InfArg a;
	return a;
}

InfArg ApplyTseitin::execute(const vector<InfArg>& args) const {
	TheoryUtils::tseitin(args[0]._theory);
	InfArg a;
	return a;
}

GroundReduce::GroundReduce() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_NIL;
	_description = "Replace ground atoms in the theory by their truth value in the structure";
	_reload = false;
}

InfArg GroundReduce::execute(const vector<InfArg>& args) const {
	TheoryUtils::reduce(args[0]._theory,args[1]._structure);
	InfArg a;
	return a;
}

InfArg MoveFunctions::execute(const vector<InfArg>& args) const {
	TheoryUtils::move_functions(args[0]._theory);
	InfArg a;
	return a;
}

InfArg CloneStructure::execute(const vector<InfArg>& args) const {
	InfArg a; 
	a._structure = args[0]._structure->clone();
	return a;
}

InfArg CloneTheory::execute(const vector<InfArg>& args) const {
	InfArg a;
	a._theory = args[0]._theory->clone();
	return a;
}

FastGrounding::FastGrounding() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_THEORY;	
	_description = "Ground the theory and structure and store the grounding";
	_reload = false;
}

InfArg FastGrounding::execute(const vector<InfArg>& args) const {
	GrounderFactory factory(args[1]._structure);
	TopLevelGrounder* g = factory.create(args[0]._theory);
	g->run();
	InfArg a;
	a._theory = g->grounding();
	return a;
}

InfArg ForceTwoValued::execute(const vector<InfArg>& args) const {
	args[0]._structure->forcetwovalued();
	InfArg a;
	return a;
}

ChangeVoc::ChangeVoc() {
	_intypes.push_back(IAT_STRUCTURE);
	_intypes.push_back(IAT_VOCABULARY);
	_outtype = IAT_NIL;
	_description = "Change the vocabulary of a structure";
	_reload = true;
}

InfArg ChangeVoc::execute(const vector<InfArg>& args) const {
	AbstractStructure* str = args[0]._structure;
	Vocabulary* v = args[1]._vocabulary;
	StructUtils::changevoc(str,v);
	InfArg a;
	return a;
}

InfArg GetIndex::execute(const vector<InfArg>& args) const {
	InfArg a;
	switch(_intypes[0]) {
		case IAT_STRUCTURE:
			// TODO
			break;
		case IAT_NAMESPACE:
		{
			Namespace* nsp = args[0]._namespace;
			string str = *(args[1]._string);
			a._overloaded = nsp->getObject(str);
			break;
		}
		case IAT_VOCABULARY:
			// TODO
			break;
		case IAT_OPTIONS:
			// TODO
			break;
		default:
			assert(false);
	}
	return a;
}

InfArg BDDPrinter::execute(const vector<InfArg>& args) const {
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	AbstractTheory* theory = args[0]._theory;
	FOBDD* result = manager.truebdd();
	for(unsigned int n = 0; n < theory->nrSentences(); ++n) {
		theory->sentence(n)->accept(&factory);
		result = manager.conjunction(result,factory.bdd());
	}
	// TODO: assert that there are no definitions and no fixpoint definitions
	InfArg a;
	a._string = new string(manager.to_string(result));
	return a;
}

SetIndex::SetIndex(InfArgType table, InfArgType key, InfArgType value) {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = table; _intypes[1] = key; _intypes[2] = value;
	_outtype = IAT_NIL;
	_description = "newindex";
	_reload = false;
}

InfArg SetIndex::execute(const vector<InfArg>& args) const {
	// TODO
	assert(false);
	InfArg a;
	return a;
}

bool overloadedObject::single() const {
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
cerr << "count=" << count << endl;
	return count <= 1;
}

void overloadedObject::makeprocedure(LuaProcedure* proc) {
	if(!isProcedure()) {
		_procedure = proc;
	}
	else if(typeid(*_procedure) == typeid(OverloadedLuaProcedure)) {
		OverloadedLuaProcedure* olp = dynamic_cast<OverloadedLuaProcedure*>(_procedure);
		olp->addproc(proc);
	}
	else {
		OverloadedLuaProcedure* olp = new OverloadedLuaProcedure(_procedure->name());
		olp->addproc(_procedure);
		olp->addproc(proc);
		_procedure = olp;
	}
}

string OverloadedLuaProcedure::code() const {
	stringstream ss;
	ss << " function(...)\n";
	ss << "local idp_intern_tab = {...}\n";
	for(unsigned int n = 0; n < _subprocedures.size(); ++n) {
		ss << "local idp_intern_tempfunc" << n << " = " << _subprocedures[n]->code() << "\n";
	}
	for(unsigned int n = 0; n < _subprocedures.size(); ++n) {
		ss << "if #idp_intern_tab == " << _subprocedures[n]->arity() << " then return idp_intern_tempfunc" << n << "(...) end\n";
	}
	ss << "end\n";
	return ss.str();
}
