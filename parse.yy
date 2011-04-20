/************************************
	parse.yy	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

%{

#include <sstream>
#include "common.hpp"
#include "insert.hpp"

// Lexer
extern int yylex();

// Inserter
Insert insert;

// Errors
void yyerror(const char* s);

// Common 
extern std::string itos(int);
extern std::string dtos(double);

typedef std::pair<int,std::string*> isp;
typedef std::list<isp>				lisp;

%}

/** Produce readable error messages **/
%debug
%locations
%error-verbose

/** Data structures **/
%union{
	int						nmr;
	char					chr;
	double					dou;
	std::string*			str;

	InternalArgument*		arg;

	Sort*					sor;
	Predicate*				pre;
	Function*				fun;
	const Compound*			cpo;
	Term*					ter;
	Rule*					rul;
	FixpDef*				fpd;
	Definition*				def;
	Formula*				fom;
	Variable*				var;
	SetExpr*				set;
	EnumSetExpr*			est;
	NSPair*					nsp;
	SortTable*				sta;
	PredTable*				pta;
	FuncTable*				fta;
	const DomainElement*	dom;

	std::vector<std::string>*			vstr;
	std::vector<Sort*>*					vsor;
	std::set<Variable*>*				svar;
	std::vector<Term*>*					vter;
	std::vector<Formula*>*				vfom;
	std::vector<Rule*>*					vrul;
	std::pair<int,int>*					vint;
	std::pair<char,char>*				vcha;
	std::vector<const DomainElement*>*	vdom;
	std::vector<ElRange>*				vera;
	std::stringstream*					sstr;

	std::vector<std::vector<std::string> >* vvstr;

}

/** Headers  **/
%token VOCAB_HEADER
%token THEORY_HEADER
%token STRUCT_HEADER
%token ASP_HEADER
%token NAMESPACE_HEADER
%token PROCEDURE_HEADER
%token OPTION_HEADER
%token EXEC_HEADER

/** Keywords **/
%token VOCABULARY
%token NAMESPACE
%token PROCEDURE
%token OPTIONS
%token PARTIAL
%token EXTENDS
%token EXTERN
%token MINAGG
%token MAXAGG
%token FALSE
%token USING
%token CARD
%token TYPE
%token PROD
%token TRUE
%token ABS
%token ISA
%token SOM 
%token LFD
%token GFD

/** Other Terminals **/
%token <nmr> INTEGER
%token <dou> FLNUMBER
%token <chr> CHARACTER
%token <chr> CHARCONS
%token <str> IDENTIFIER
%token <str> STRINGCONS
%token <sstr>	LUACHUNK

/** Aliases **/
%token <operator> MAPS			"->"
%token <operator> EQUIV			"<=>"
%token <operator> IMPL			"=>"
%token <operator> RIMPL			"<="
%token <operator> DEFIMP		"<-"
%token <operator> NEQ			"~="
%token <operator> EQ			"=="
%token <operator> LEQ			"=<"
%token <operator> GEQ			">="
%token <operator> RANGE			".."
%token <operator> NSPACE		"::"

/** Precedence declarations for connectives **/
%right ':'
%nonassoc "<=>"
%nonassoc "=>"
%nonassoc "<="
%right '|'
%right '&'
%right '~' 

/**  Precedence declarations for arithmetic **/
%left '%'
%left '-' '+'
%left '/' '*' 
%left '^'
%left UMINUS

/** Non-terminals with semantic value **/
%type <nmr> integer
%type <dou> floatnr
%type <str> strelement
%type <str>	identifier
%type <var> variable
%type <sor> sort_pointer
%type <sor> sort_decl
%type <sor> theosort_pointer
%type <pre> pred_decl
%type <nsp>	intern_pointer
%type <fun> func_decl
%type <fun> full_func_decl
%type <fun> arit_func_decl
%type <ter> term
%type <ter> domterm
%type <ter> function
%type <ter> arterm
%type <ter> aggterm
%type <fom> predicate
%type <fom> head
%type <fom> formula
%type <fom>	eq_chain
%type <rul> rule
%type <sta> elements_es
%type <sta> elements
%type <fpd> fixpdef
%type <fpd> fd_rules
%type <def> definition
%type <set> formulaset
%type <set> termset
%type <est> form_list
%type <est> form_term_list
%type <cpo> compound
%type <arg> function_call
%type <pta>	ptuples
%type <pta>	ptuples_es
%type <fta> ftuples
%type <fta> ftuples_es
%type <pta> f3tuples
%type <pta> f3tuples_es
%type <dom>	pelement
%type <dom>	domain_element

%type <vint>	intrange
%type <vcha>	charrange
%type <vter>	term_tuple
%type <svar>	variables
%type <vrul>	rules
%type <vstr>	pointer_name
%type <vsor>	sort_pointer_tuple
%type <vsor>	nonempty_spt
%type <vsor>	binary_arit_func_sorts
%type <vsor>	unary_arit_func_sorts
%type <vdom>	compound_args
%type <vdom>	ptuple	
%type <vdom>	ftuple	
%type <vera>	domain_tuple

%type <vvstr> pointer_names

%%

/*********************
	Global structure
*********************/

idporblock		: idp
				| execstatement
				;

idp		        : /* empty */
				| idp namespace 
				| idp vocabulary 
		        | idp theory
				| idp structure
				| idp asp_structure
				| idp instructions
				| idp options
				| idp using
		        ;
	
namespace		: NAMESPACE_HEADER namespace_name '{' idp '}'	{ insert.closespace();	}
				;

namespace_name	: identifier									{ insert.openspace(*$1,@1); }
				;

using			: USING VOCABULARY pointer_name					{ insert.usingvocab(*$3,@1); delete($3);	}
				| USING NAMESPACE pointer_name					{ insert.usingspace(*$3,@1); delete($3);	}
				;

 
/***************************
	Vocabulary declaration
***************************/

/** Structure of vocabulary declaration **/

vocabulary			: VOCAB_HEADER vocab_name '{' vocab_content '}'		{ insert.closevocab();	}	
					| VOCAB_HEADER vocab_name '=' function_call			{ insert.assignvocab($4,@2);
																		  insert.closevocab();	}
					;

vocab_name			: identifier	{ insert.openvocab(*$1,@1); }
					;

vocab_pointer		: pointer_name	{ insert.setvocab(*$1,@1); delete($1); }
					;

vocab_content		: /* empty */
					| vocab_content symbol_declaration
					| vocab_content EXTERN extern_symbol
					| vocab_content EXTERN VOCABULARY pointer_name	{ insert.externvocab(*$4,@4); delete($4);	}
					| vocab_content using
					| vocab_content error
					;

symbol_declaration	: sort_decl
					| pred_decl
					| func_decl
					;

extern_symbol		: extern_sort
					| extern_predicate
					| extern_function
					;

extern_sort			: TYPE sort_pointer	{ insert.sort($2);			}
					;

extern_predicate	: pointer_name '[' sort_pointer_tuple ']'	{ insert.predicate(insert.predpointer(*$1,*$3,@1));
																  delete($1); delete($3); }
					| pointer_name '/' INTEGER					{ insert.predicate(insert.predpointer(*$1,$3,@1));
																  delete($1); }
					;

extern_function		: pointer_name '[' sort_pointer_tuple ':' sort_pointer ']' 
																	{ insert.function(insert.funcpointer(*$1,*$3,@1));
																	  delete($1); delete($3); }
					| pointer_name '/' INTEGER ':' INTEGER			{ insert.function(insert.funcpointer(*$1,$3,@1));
																	  delete($1); }
					;

/** Symbol declarations **/

sort_decl		: TYPE identifier											{ $$ = insert.sort(*$2,@2);		}
				| TYPE identifier ISA nonempty_spt							{ $$ = insert.sort(*$2,*$4,true,@2);	
																			  delete($4); }
				| TYPE identifier EXTENDS nonempty_spt						{ $$ = insert.sort(*$2,*$4,false,@2);	
																			  delete($4); }
				| TYPE identifier ISA nonempty_spt EXTENDS nonempty_spt		{ $$ = insert.sort(*$2,*$4,*$6,@2);		
																			  delete($4); delete($6); }
				| TYPE identifier EXTENDS nonempty_spt ISA nonempty_spt		{ $$ = insert.sort(*$2,*$6,*$4,@2);		
																			  delete($4); delete($6); }
				;

pred_decl		: identifier '(' sort_pointer_tuple ')'	{ insert.predicate(*$1,*$3,@1); delete($3); }
				| identifier							{ insert.predicate(*$1,@1);  }
				;

func_decl		: PARTIAL full_func_decl				{ $$ = $2; insert.partial($$);	}
				| full_func_decl						{ $$ = $1;								}
				| PARTIAL arit_func_decl				{ $$ = $2; insert.partial($$);	}
				| arit_func_decl						{ $$ = $1;								}
				;

full_func_decl	: identifier '(' sort_pointer_tuple ')' ':' sort_pointer	{ $$ = insert.function(*$1,*$3,$6,@1);
																			  delete($3); }
				| identifier ':' sort_pointer								{ $$ = insert.function(*$1,$3,@1); }	
				; 														

arit_func_decl	: '-' binary_arit_func_sorts				{ $$ = insert.aritfunction("-/2",*$2,@1); delete($2);	}
                | '+' binary_arit_func_sorts				{ $$ = insert.aritfunction("+/2",*$2,@1); delete($2);	}
                | '*' binary_arit_func_sorts				{ $$ = insert.aritfunction("*/2",*$2,@1); delete($2);	}
                | '/' binary_arit_func_sorts				{ $$ = insert.aritfunction("//2",*$2,@1); delete($2);	}
                | '%' binary_arit_func_sorts				{ $$ = insert.aritfunction("%/2",*$2,@1); delete($2);	}
                | '^' binary_arit_func_sorts				{ $$ = insert.aritfunction("^/2",*$2,@1); delete($2);	}
                | '-' unary_arit_func_sorts  %prec UMINUS	{ $$ = insert.aritfunction("-/1",*$2,@1); delete($2);	}
                | ABS unary_arit_func_sorts					{ $$ = insert.aritfunction("abs/1",*$2,@1); delete($2);	}
				;

binary_arit_func_sorts	: '(' sort_pointer ',' sort_pointer ')' ':' sort_pointer	
							{ $$ = new std::vector<Sort*>(3); (*$$)[0] = $2; (*$$)[1] = $4; (*$$)[2] = $7; }
						;

unary_arit_func_sorts	: '(' sort_pointer ')' ':' sort_pointer	
							{ $$ = new std::vector<Sort*>(2); (*$$)[0] = $2; (*$$)[1] = $5; }
						;

/** Symbol pointers **/

sort_pointer		: pointer_name				{ $$ = insert.sortpointer(*$1,@1); delete($1); }
					;

intern_pointer		: pointer_name '[' sort_pointer_tuple ']'	{ $$ = insert.internpredpointer(*$1,*$3,@1);
																  delete($1); delete($3); }
					| pointer_name '[' sort_pointer_tuple ':' sort_pointer ']'	
																{ $$ = insert.internfuncpointer(*$1,*$3,$5,@1);
																  delete($1); delete($3); }
					| pointer_name								{ $$ = insert.internpointer(*$1,@1);
																  delete($1); }
					;

pointer_name		: pointer_name "::" identifier	{ $$ = $1; $$->push_back(*$3); 		}
					| identifier					{ $$ = new std::vector<std::string>(1,*$1);	}
					;

/*************
	Theory	
*************/

theory		: THEORY_HEADER theory_name ':' vocab_pointer '{' def_forms '}'		{ insert.closetheory();	}
			| THEORY_HEADER theory_name '=' function_call						{ insert.assigntheory($4,@2); 
																				  insert.closetheory();	}
			;

theory_name	: identifier	{ insert.opentheory(*$1,@1);  }
			;

function_call	: pointer_name '(' pointer_names ')'	{ $$ = insert.call(*$1,*$3,@1); delete($1); delete($3);	}
				| pointer_name '(' ')'					{ $$ = insert.call(*$1,@1); delete($1);					}
				;

pointer_names	: pointer_names ',' pointer_name	{ $$ = $1; $$->push_back(*$3);
													  delete($3); }
				| pointer_name						{ $$ = new std::vector<std::vector<std::string> >(1,*$1);
													  delete($1); }
				;

def_forms	: /* empty */
			| def_forms definition		{ insert.definition($2);	}
			| def_forms formula '.'		{ insert.sentence($2);		}
			| def_forms fixpdef			{ insert.fixpdef($2);		}
			| def_forms using
			| def_forms error '.' 
			| def_forms '{' error '}'
			;

/** Definitions **/

definition	: '{' rules '}'		{ $$ = insert.definition(*$2); delete($2);	}
			;

rules		: rules rule '.'	{ $$ = $1; $1->push_back($2);	}				
			| rule '.'			{ $$ = new std::vector<Rule*>(1,$1);	}			
			;

rule		: '!' variables ':' head "<-" formula	{ $$ = insert.rule(*$2,$4,$6,@1); delete($2);	}
			| '!' variables ':' head "<-"			{ $$ = insert.rule(*$2,$4,@1); delete($2);		}
			| '!' variables ':' head				{ $$ = insert.rule(*$2,$4,@1); delete($2);		}
			| head "<-" formula						{ $$ = insert.rule($1,$3,@1);	}
			| head "<-"								{ $$ = insert.rule($1,@1);		}
			| head									{ $$ = insert.rule($1,@1);		}
			;

head		: predicate										{ $$ = $1;												}
			| intern_pointer '(' term_tuple ')' '=' term	{ $$ = insert.funcgraphform($1,*$3,$6,@1); delete($3);	}
			| intern_pointer '(' ')' '=' term				{ $$ = insert.funcgraphform($1,$5,@1);					}
			| intern_pointer '=' term						{ $$ = insert.funcgraphform($1,$3,@1);					}
			;


/** Fixpoint definitions **/

fixpdef		: LFD '[' fd_rules ']'		{ $$ = $3; insert.makeLFD($3,true); 	}
			| GFD '[' fd_rules ']'		{ $$ = $3; insert.makeLFD($3,false);	}
			;

fd_rules	: fd_rules rule	'.'			{ $$ = $1; insert.addRule($$,$2);					}
			| fd_rules fixpdef			{ $$ = $1; insert.addDef($$,$2);					}
			| rule '.'					{ $$ = insert.createFD(); insert.addRule($$,$1);	}
			| fixpdef					{ $$ = insert.createFD(); insert.addDef($$,$1);		}
			;	

/** Formulas **/

formula		: '!' variables ':' formula					{ $$ = insert.univform(*$2,$4,@1); delete($2);		}
            | '?' variables ':' formula					{ $$ = insert.existform(*$2,$4,@1); delete($2);	}
			| '?' INTEGER  variables ':' formula		{ $$ = insert.bexform(CT_EQ,$2,*$3,$5,@1);
														  delete($3);										}
			| '?' '=' INTEGER variables ':' formula		{ $$ = insert.bexform(CT_EQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '<' INTEGER variables ':' formula		{ $$ = insert.bexform(CT_LT,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '>' INTEGER variables ':' formula		{ $$ = insert.bexform(CT_GT,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' "=<" INTEGER variables ':' formula	{ $$ = insert.bexform(CT_LEQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' ">=" INTEGER variables ':' formula	{ $$ = insert.bexform(CT_GEQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '~' formula								{ $$ = $2; insert.negate($$);						}
            | formula '&' formula						{ $$ = insert.conjform($1,$3,@1);					}
            | formula '|' formula						{ $$ = insert.disjform($1,$3,@1);					}
            | formula "=>" formula						{ $$ = insert.implform($1,$3,@1);					}
            | formula "<=" formula						{ $$ = insert.revimplform($1,$3,@1);				}
            | formula "<=>" formula						{ $$ = insert.equivform($1,$3,@1);					}
            | '(' formula ')'							{ $$ = $2;											}
			| TRUE										{ $$ = insert.trueform(@1);						}
			| FALSE										{ $$ = insert.falseform(@1);						}
			| eq_chain									{ $$ = $1;											}
            | predicate									{ $$ = $1;											}
            ;

predicate   : intern_pointer							{ $$ = insert.predform($1,@1);					}
			| intern_pointer '(' ')'					{ $$ = insert.predform($1,@1);					}
            | intern_pointer '(' term_tuple ')'			{ $$ = insert.predform($1,*$3,@1); delete($3); }
            ;

eq_chain	: eq_chain '='  term	{ $$ = insert.eqchain(CT_EQ,$1,$3,@1);	}
			| eq_chain "~=" term	{ $$ = insert.eqchain(CT_NEQ,$1,$3,@1);	}
			| eq_chain '<'  term	{ $$ = insert.eqchain(CT_LT,$1,$3,@1);	}	
			| eq_chain '>'  term    { $$ = insert.eqchain(CT_GT,$1,$3,@1);	}
			| eq_chain "=<" term	{ $$ = insert.eqchain(CT_LEQ,$1,$3,@1);	}		
			| eq_chain ">=" term	{ $$ = insert.eqchain(CT_GEQ,$1,$3,@1);	}		
			| term '='  term		{ $$ = insert.eqchain(CT_EQ,$1,$3,@1);	}
            | term "~=" term		{ $$ = insert.eqchain(CT_NEQ,$1,$3,@1);	}
            | term '<'  term		{ $$ = insert.eqchain(CT_LT,$1,$3,@1);	}
            | term '>'  term		{ $$ = insert.eqchain(CT_GT,$1,$3,@1);	}
            | term "=<" term		{ $$ = insert.eqchain(CT_LEQ,$1,$3,@1);	}
            | term ">=" term		{ $$ = insert.eqchain(CT_GEQ,$1,$3,@1);	}
			;

variables   : variables variable	{ $$ = $1; $$->insert($2);						}		
            | variable				{ $$ = new std::set<Variable*>;	$$->insert($1);	}
			;

variable	: identifier							{ $$ = insert.quantifiedvar(*$1,@1);		}
			| identifier '[' theosort_pointer ']'	{ $$ = insert.quantifiedvar(*$1,$3,@1);	}
			;

theosort_pointer	:	pointer_name		{ $$ = insert.theosortpointer(*$1,@1); delete($1);	}
					;

/** Terms **/                                            

term		: function		{ $$ = $1;	}		
			| arterm		{ $$ = $1;	}
			| domterm		{ $$ = $1;	}
			| aggterm		{ $$ = $1;	}
			;

function	: intern_pointer '(' term_tuple ')'		{ $$ = insert.functerm($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = insert.functerm($1);					}
			| intern_pointer						{ $$ = insert.functerm($1);					}
			;

arterm		: term '-' term				{ $$ = insert.arterm('-',$1,$3,@1);	}				
			| term '+' term				{ $$ = insert.arterm('+',$1,$3,@1);	}
			| term '*' term				{ $$ = insert.arterm('*',$1,$3,@1);	}
			| term '/' term				{ $$ = insert.arterm('/',$1,$3,@1);	}
			| term '%' term				{ $$ = insert.arterm('%',$1,$3,@1);	}
			| term '^' term				{ $$ = insert.arterm('^',$1,$3,@1);	}
			| '-' term %prec UMINUS		{ $$ = insert.arterm("-",$2,@1);		}
			| ABS '(' term ')'			{ $$ = insert.arterm("abs",$3,@1);		}
			| '(' arterm ')'			{ $$ = $2;								}
			;

domterm		: INTEGER									{ $$ = insert.domterm($1,@1);		}
			| FLNUMBER									{ $$ = insert.domterm($1,@1);		}
			| STRINGCONS								{ $$ = insert.domterm($1,@1);		}
			| CHARCONS									{ $$ = insert.domterm($1,@1);		}
			| '@' identifier '[' theosort_pointer ']'	{ $$ = insert.domterm($2,$4,@1);	}
			| '@' identifier							{ $$ = insert.domterm($2,0,@1);	}
			;

aggterm		: CARD formulaset	{ $$ = insert.aggregate(AGG_CARD,$2,@1);	}
			| SOM termset		{ $$ = insert.aggregate(AGG_SUM,$2,@1);		}
			| PROD termset		{ $$ = insert.aggregate(AGG_PROD,$2,@1);	}
			| MINAGG termset	{ $$ = insert.aggregate(AGG_MIN,$2,@1);		}
			| MAXAGG termset	{ $$ = insert.aggregate(AGG_MAX,$2,@1);		}
			;

formulaset		: '{' variables ':' formula '}'				{ $$ = insert.set(*$2,$4,@1); delete($2);	}
				| '[' form_list ']'							{ $$ = insert.set($2);						}
				;

termset			: '{' variables ':' formula ':' term '}'	{ $$ = insert.set(*$2,$4,$6,@1); delete($2);	}	
				| '[' form_term_list ']'					{ $$ = insert.set($2);							}
				;

form_list		: form_list ';' formula						{ $$ = $1; insert.addFormula($$,$3);						}
				| formula									{ $$ = insert.createEnum(@1);	insert.addFormula($$,$1);	}		
				;

form_term_list	: form_term_list ';' '(' formula ',' term ')'	{ $$ = $1; insert.addFT($$,$4,$6);						}
				| '(' formula ',' term ')'						{ $$ = insert.createEnum(@1); insert.addFT($$,$2,$4);	}
				;


/**********************
	Input structure
**********************/

structure		: STRUCT_HEADER struct_name ':' vocab_pointer '{' interpretations '}'	{ insert.closestructure();	}
				| STRUCT_HEADER struct_name '=' function_call							{ insert.assignstructure($4,@2);
																						  insert.closestructure();	}
				;

struct_name		: identifier	{ insert.openstructure(*$1,@1); }
				;

interpretations	:  //empty
				| interpretations interpretation
				| interpretations using
				;

interpretation	: empty_inter
				| sort_inter
				| pred_inter
				| func_inter
				| proc_inter
				| three_inter
				| error
				;

/** Empty interpretations **/

empty_inter	: intern_pointer '=' '{' '}'				{ insert.emptyinter($1); }
			;

/** Interpretations with arity 1 **/

sort_inter	: intern_pointer '=' '{' elements_es '}'		{ insert.sortinter($1,$4);  }	
			;

elements_es		: elements ';'						{ $$ = $1;	}
				| elements							{ $$ = $1;	}
				;

elements		: elements ';' charrange			{ $$ = $1; insert.addElement($$,$3->first,$3->second);		}
				| elements ';' intrange				{ $$ = $1; insert.addElement($$,$3->first,$3->second);		}
				| elements ';' '(' strelement ')'	{ $$ = $1; insert.addElement($$,$4);						}
				| elements ';' '(' integer ')'		{ $$ = $1; insert.addElement($$,$4);						}
				| elements ';' '(' compound ')'		{ $$ = $1; insert.addElement($$,$4);						}
				| elements ';' '(' floatnr ')'		{ $$ = $1; insert.addElement($$,$4);						}
				| elements ';' integer				{ $$ = $1; insert.addElement($$,$3);						}
				| elements ';' strelement			{ $$ = $1; insert.addElement($$,$3);						}
				| elements ';' floatnr				{ $$ = $1; insert.addElement($$,$3);						}
				| elements ';' compound				{ $$ = $1; insert.addElement($$,$3);						}
				| charrange							{ $$ = insert.createSortTable(); 
													  insert.addElement($$,$1->first,$1->second);				}
				| intrange							{ $$ = insert.createSortTable(); 
													  insert.addElement($$,$1->first,$1->second);				}
				| '(' strelement ')'				{ $$ = insert.createSortTable(); insert.addElement($$,$2);	}
				| '(' integer ')'					{ $$ = insert.createSortTable(); insert.addElement($$,$2);	}
				| '(' floatnr ')'                   { $$ = insert.createSortTable(); insert.addElement($$,$2);	}
				| '(' compound ')'					{ $$ = insert.createSortTable(); insert.addElement($$,$2);	}
				| strelement						{ $$ = insert.createSortTable(); insert.addElement($$,$1);	}
				| integer							{ $$ = insert.createSortTable(); insert.addElement($$,$1);	}
				| floatnr							{ $$ = insert.createSortTable(); insert.addElement($$,$1);	}
				| compound							{ $$ = insert.createSortTable(); insert.addElement($$,$1);	}
				;

strelement		: identifier	{ $$ = $1;									}
				| STRINGCONS	{ $$ = $1;									}
				| CHARCONS		{ $$ = StringPointer(std::string(1,$1));	}
				;

/** Interpretations with arity not 1 **/

pred_inter		: intern_pointer '=' '{' ptuples_es '}'		{ insert.predinter($1,$4);		}
				| intern_pointer '=' TRUE					{ insert.truepredinter($1);		}
				| intern_pointer '=' FALSE					{ insert.falsepredinter($1);	}
				;

ptuples_es		: ptuples ';'					{ $$ = $1;	}	
				| ptuples						{ $$ = $1;	}
				;

ptuples			: ptuples ';' '(' ptuple ')'	{ $$ = $1; insert.addTuple($$,*$4,@4); delete($4);	}
				| ptuples ';' ptuple			{ $$ = $1; insert.addTuple($$,*$3,@3); delete($3);	}
				| ptuples ';' emptyptuple		{ $$ = $1; insert.addTuple($$,@3);					}
				| '(' ptuple ')'				{ $$ = insert.createPredTable(); insert.addTuple($$,*$2,@2); delete($2);	}
				| ptuple						{ $$ = insert.createPredTable(); insert.addTuple($$,*$1,@1); delete($1);	}
				| emptyptuple					{ $$ = insert.createPredTable(); insert.addTuple($$,@1);					}
				;

emptyptuple		: '(' ')'										
				;

ptuple			: ptuple ',' pelement			{ $$ = $1; $$->push_back($3);	}			
				| pelement ',' pelement			{ $$ = new std::vector<const DomainElement*>(); 
												  $$->push_back($1); $$->push_back($3); }
				;

pelement		: integer		{ $$ = insert.element($1);	}
				| identifier	{ $$ = insert.element($1);	}
				| CHARCONS		{ $$ = insert.element($1);	}
				| STRINGCONS	{ $$ = insert.element($1);	}
				| floatnr		{ $$ = insert.element($1);	}
				| compound		{ $$ = insert.element($1);	}
				;

/** Interpretations for functions **/

func_inter	: intern_pointer '=' '{' ftuples_es '}'	{ insert.funcinter($1,$4); }	
			| intern_pointer '=' pelement			{ FuncTable* ft = insert.createFuncTable();
													  insert.addTupleVal(ft,$3,@3);
													  insert.funcinter($1,ft); }
			;

ftuples_es		: ftuples ';'					{ $$ = $1;	}
				| ftuples						{ $$ = $1;	}
				;

ftuples			: ftuples ';' ftuple			{ $$ = $1; insert.addTupleVal($$,*$3,@3); delete($3);					    }
				| ftuple						{ $$ = insert.createFuncTable(); insert.addTupleVal($$,*$1,@1);	delete($1);	}
				;

ftuple			: ptuple "->" pelement			{ $$ = $1; $$->push_back($3);	}			
				| pelement "->" pelement		{ $$ = new std::vector<const DomainElement*>(1,$1); $$->push_back($3);	}
				| emptyptuple "->" pelement		{ $$ = new std::vector<const DomainElement*>(1,$3);	}		
				| "->" pelement					{ $$ = new std::vector<const DomainElement*>(1,$2);	}
				;

f3tuples_es		: f3tuples ';'					{ $$ = $1;	}
				| f3tuples						{ $$ = $1;	}
				;

f3tuples		: f3tuples ';' ftuple			{ $$ = $1; insert.addTuple($$,*$3,@3); delete($3);							}
				| ftuple						{ $$ = insert.createPredTable(); insert.addTuple($$,*$1,@1); delete($1);	}
				;

/** Procedural interpretations **/

proc_inter		: intern_pointer '=' PROCEDURE pointer_name	{ insert.inter($1,*$4,@1); delete($4);	}
				;

/** Three-valued interpretations **/

three_inter		: threepred_inter
				| threefunc_inter
				| threeempty_inter
				;

threeempty_inter	: intern_pointer '<' identifier '>' '=' '{' '}'			{ insert.emptythreeinter($1,*$3); }
					;

threepred_inter : intern_pointer '<' identifier '>' '=' '{' ptuples_es '}'	{ insert.threepredinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' '{' elements_es '}'	{ insert.threepredinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' TRUE				{ insert.truethreepredinter($1,*$3);	}
				| intern_pointer '<' identifier '>' '=' FALSE				{ insert.falsethreepredinter($1,*$3);	}
				;

threefunc_inter	: intern_pointer '<' identifier '>' '=' '{' f3tuples_es '}'	{ insert.threefuncinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' pelement			{ PredTable* ft = insert.createPredTable();
																			  std::vector<const DomainElement*> vd(1,$6);
																			  insert.addTuple(ft,vd,@6);
																			  insert.threefuncinter($1,*$3,ft);		}
				;

/** Ranges **/

intrange	: integer ".." integer			{ $$ = insert.range($1,$3,@1); }
			;
charrange	: CHARACTER ".." CHARACTER		{ $$ = insert.range($1,$3,@1);	}
			| CHARCONS ".." CHARCONS		{ $$ = insert.range($1,$3,@1);	}
			;

/** Compound elements **/

compound	: intern_pointer '(' compound_args ')'	{ $$ = insert.compound($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = insert.compound($1);					}
			;

compound_args	: compound_args ',' floatnr		{ $$ = $1; $$->push_back(insert.element($3));	}
				| compound_args ',' integer		{ $$ = $1; $$->push_back(insert.element($3));	}
				| compound_args ',' strelement	{ $$ = $1; $$->push_back(insert.element($3));	}
				| compound_args ',' compound	{ $$ = $1; $$->push_back(insert.element($3));	}
				| floatnr						{ $$ = new std::vector<const DomainElement*>(1,insert.element($1));	}
				| integer						{ $$ = new std::vector<const DomainElement*>(1,insert.element($1));	}
				| strelement					{ $$ = new std::vector<const DomainElement*>(1,insert.element($1));	}
				| compound						{ $$ = new std::vector<const DomainElement*>(1,insert.element($1));	}
				;
	         
/** Terminals **/

integer			: INTEGER		{ $$ = $1;		}
				| '-' INTEGER	{ $$ = -$2;		}
				;

floatnr			: FLNUMBER			{ $$ = $1;		}
				| '-' FLNUMBER		{ $$ = -($2);	}
				;

identifier		: IDENTIFIER	{ $$ = $1;	}
				| CHARACTER		{ $$ = StringPointer(std::string(1,$1)); } 
				| VOCABULARY	{ $$ = StringPointer(std::string("vocabulary"));	}
				| NAMESPACE		{ $$ = StringPointer(std::string("namespace"));	}
				;

/********************
	ASP structure
********************/

asp_structure	: ASP_HEADER struct_name ':' vocab_pointer '{' atoms '}'	{ insert.closestructure();	}
				;

atoms	: /* empty */
		| atoms atom '.'
		| atoms using
		;

atom	: predatom
		| funcatom
		;

predatom	: intern_pointer '(' domain_tuple ')'		{ insert.predatom($1,*$3,true);	delete($3);		}
			| intern_pointer '(' ')'					{ insert.predatom($1,true);						}
			| intern_pointer							{ insert.predatom($1,true);						}
			| '-' intern_pointer '(' domain_tuple ')'	{ insert.predatom($2,*$4,false); delete($4);	}
			| '-' intern_pointer '(' ')'				{ insert.predatom($2,false);					}
			| '-' intern_pointer						{ insert.predatom($2,false);					}
			; 

funcatom	: intern_pointer '(' domain_tuple ')' '=' domain_element		{ insert.funcatom($1,*$3,$6,true); delete($3);	}
			| intern_pointer '(' ')' '=' domain_element				        { insert.funcatom($1,$5,true);		}
			| intern_pointer '=' domain_element						        { insert.funcatom($1,$3,true);		}
			| '-' intern_pointer '(' domain_tuple ')' '=' domain_element	{ insert.funcatom($2,*$4,$7,false);	delete($4);	}
			| '-' intern_pointer '(' ')' '=' domain_element				    { insert.funcatom($2,$6,false);		}
			| '-' intern_pointer '=' domain_element						    { insert.funcatom($2,$4,false);		}
			;

domain_tuple	: domain_tuple ',' domain_element	{ $$ = insert.domaintuple($1,$3);	}
				| domain_tuple ',' intrange			{ $$ = insert.domaintuple($1,$3);	}
				| domain_tuple ',' charrange		{ $$ = insert.domaintuple($1,$3);	}
				| domain_element					{ $$ = insert.domaintuple($1);		}
				| intrange							{ $$ = insert.domaintuple($1);		}
				| charrange							{ $$ = insert.domaintuple($1);		}
				;

domain_element	: strelement	{ $$ = insert.element($1); }
				| integer		{ $$ = insert.element($1); }
				| floatnr		{ $$ = insert.element($1); }
				| compound		{ $$ = insert.element($1); }
				;

term_tuple		: term_tuple ',' term						{ $$ = $1; $$->push_back($3);		}	
				| term										{ $$ = new std::vector<Term*>(1,$1);		}	
				;

sort_pointer_tuple	: /* empty */							{ $$ = new std::vector<Sort*>(0);		}
					| nonempty_spt							{ $$ = $1;							}
					;
					
nonempty_spt		: sort_pointer_tuple ',' sort_pointer	{ $$ = $1; $$->push_back($3);		}
					| sort_pointer							{ $$ = new std::vector<Sort*>(1,$1);		}
					;


/*******************
	Instructions
*******************/

execstatement		: EXEC_HEADER '{' LUACHUNK 		{ insert.exec($3); delete($3);	}
					;

instructions		: PROCEDURE_HEADER proc_name proc_sig '{' LUACHUNK 		{ insert.closeprocedure($5); delete($5);	}
					;

proc_name			: identifier	{ insert.openprocedure(*$1,@1);	}
					;

proc_sig			: '(' ')'		
					| '(' args ')'
					;

args				: args ',' identifier	{ insert.procarg(*$3);		}
					| identifier			{ insert.procarg(*$1);		}
					;


/**************
	Options
**************/

options	: OPTION_HEADER option_name '{' optassigns '}'	{ insert.closeoptions();	}
		;

option_name	: identifier	{ insert.openoptions(*$1,@1);	}
			;

optassigns	: /* empty */
			| optassigns optassign	
			| optassigns EXTERN OPTIONS pointer_name	{ insert.externoption(*$4,@4);	delete($4);	}
			;

optassign	: identifier '=' strelement		{ insert.option(*$1,*$3,@1);	}
			| identifier '=' floatnr		{ insert.option(*$1,$3,@1);		}
			| identifier '=' integer		{ insert.option(*$1,$3,@1);		}
			| identifier '=' TRUE			{ insert.option(*$1,true,@1);	}
			| identifier '=' FALSE			{ insert.option(*$1,false,@1);	}
			;


%%

#include <iostream>
#include "error.hpp"

extern FILE* yyin;

void yyerror(const char* s) {
	ParseInfo pi(yylloc.first_line,yylloc.first_column,insert.currfile());
	Error::error(pi);
	std::cerr << s << std::endl;
}

void parsefile(const std::string& str) {
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yyin = fopen(str.c_str(),"r");
	if(yyin) {
		insert.currfile(str);
		yyparse();
		fclose(yyin);
	}
	else Error::unknfile(str);
}

void parsestdin() {
	yyin = stdin;
	insert.currfile(0);
	yyparse();
}

