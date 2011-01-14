/************************************
	execute.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_H
#define EXECUTE_H

#include "namespace.hpp"
#include <sstream>

namespace BuiltinProcs {
	void initialize();
}

/** Lua procedures **/

class LuaProcedure {
	private:
		string				_name;
		ParseInfo			_pi;
		vector<string>		_innames;
		stringstream		_code;
	public:
		LuaProcedure(const string& name, const ParseInfo& pi) :
			_name(name), _pi(pi), _innames(0) { }

		// Mutators
		void addarg(const string& name) { _innames.push_back(name);	}
		void add(char* s) { _code << s;	}
		void add(const string& s)	{ _code << s;	}
		
		// Inspectors
		const ParseInfo&	pi()	const { return _pi;		}
		const string&		name()	const { return _name;	}
		unsigned int		arity()	const { return _innames.size();	}
		string				code()	const { return _code.str();		}

};

/** Possible argument or return value of an execute statement **/
union InfArg {
	Vocabulary*			_vocabulary;
	AbstractStructure*	_structure;
	AbstractTheory*		_theory;
	Namespace*			_namespace;
	double				_number;
	bool				_boolean;
	string*				_string;
};

/** An execute statement **/
class Inference {
	protected:
		vector<InfArgType>	_intypes;		// types of the input arguments
		InfArgType			_outtype;		// type of the return value
		string				_description;	// description of the inference
	public:
		virtual ~Inference() { }
		virtual InfArg execute(const vector<InfArg>& args) const = 0;	// execute the statement
		const vector<InfArgType>&	intypes()		const { return _intypes;		}
		InfArgType					outtype()		const { return _outtype;		}
		unsigned int				arity()			const { return _intypes.size();	}
		string						description()	const { return _description;	}
};

class PrintTheory : public Inference {
	public:
		PrintTheory() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;
			_description = "Print the theory";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary() { 
			_intypes = vector<InfArgType>(1,IAT_VOCABULARY); 
			_outtype = IAT_VOID;	
			_description = "Print the vocabulary";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure() { 
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_outtype = IAT_VOID;	
			_description = "Print the structure";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace() { 
			_intypes = vector<InfArgType>(1,IAT_NAMESPACE); 
			_outtype = IAT_VOID;	
			_description = "Print the namespace";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Push all negations inside until they are in front of atoms";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class RemoveEquivalences : public Inference {
	public:
		RemoveEquivalences() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite equivalences into pairs of implications";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class RemoveEqchains : public Inference {
	public:
		RemoveEqchains() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite chains of (in)equalities to conjunctions of atoms";
		}
		InfArg execute(const vector<InfArg>& args) const;

};

class FlattenFormulas : public Inference {
	public:
		FlattenFormulas() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite ((A & B) & C) to (A & B & C), rewrite (! x : ! y : phi(x,y)) to (! x y : phi(x,y)), etc.";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class GroundingInference : public Inference {
	public:
		GroundingInference();
		InfArg execute(const vector<InfArg>& args) const;
};

class GroundingWithResult : public Inference {
	public:
		GroundingWithResult();
		InfArg execute(const vector<InfArg>& args) const;
};

class ModelExpansionInference : public Inference {
	public:
		ModelExpansionInference();
		InfArg execute(const vector<InfArg>& args) const;
};

class StructToTheory : public Inference {
	public:
		StructToTheory() {
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_outtype = IAT_THEORY;	
			_description = "Rewrite a structure to a theory";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class MoveQuantifiers : public Inference {
	public:
		MoveQuantifiers() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Move universal (existential) quantifiers inside conjunctions (disjunctions)";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class ApplyTseitin : public Inference {
	public:
		ApplyTseitin() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Apply the tseitin transformation to a theory";
		}
		InfArg execute(const vector<InfArg>& args) const;
};

class GroundReduce : public Inference {
	public:
		GroundReduce();
		InfArg execute(const vector<InfArg>& args) const;
};

class MoveFunctions : public Inference {
	public:
		MoveFunctions() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Move functions until no functions are nested";
		}
		InfArg execute(const vector<InfArg>& args) const;
};


#endif
