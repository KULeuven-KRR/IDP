%{

#include <sstream>
#include "GlobalData.hpp"
#include "common.hpp"
#include "insert.hpp"
#include "parser/yyltype.hpp"

// Lexer
extern int yylex();

Insert& getInserter();

// Errors
void yyerror(const char* s);

typedef std::pair<int,std::string*> isp;
typedef std::list<isp>				lisp;

# define YYLLOC_DEFAULT(Current, Rhs, N)									 \
         do                                                                  \
           if (N)                                                            \
             {                                                               \
               (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
               (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
               (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
               (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
			   (Current).descr		  = YYRHSLOC(Rhs, 1).descr;				 \
             }                                                               \
           else                                                              \
             {                                                               \
               (Current).first_line   = (Current).last_line   =              \
                 YYRHSLOC(Rhs, 0).last_line;                                 \
               (Current).first_column = (Current).last_column =              \
                 YYRHSLOC(Rhs, 0).last_column;                               \
			   (Current).descr		  = YYRHSLOC(Rhs,0).descr;				 \
             }                                                               \
         while (0)

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
	Query*					que;
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
	std::vector<Variable*>*				vvar;
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
%token QUERY_HEADER
%token TERM_HEADER

/** Keywords **/
%token CONSTRUCTOR
%token VOCABULARY
%token NAMESPACE
%token PROCEDURE
%token OPTIONS
%token PARTIAL
%token EXTENDS
%token EXTERN
%token P_MINAGG P_MAXAGG P_CARD P_PROD P_SOM
%token FALSE
%token USINGVOCABULARY
%token USINGNAMESPACE
%token TYPE
%token TRUE
%token ABS
%token ISA
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
%token <operator> P_IMPL		"=>"
%token <operator> P_RIMPL		"<="
%token <operator> DEFIMP		"<-"
%token <operator> P_NEQ			"~="
%token <operator> P_EQ			"=="
%token <operator> P_LEQ			"=<"
%token <operator> P_GEQ			">="
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
%type <que> query

%type <vint>	intrange
%type <vcha>	charrange
%type <vter>	term_tuple
%type <svar>	variables
%type <vvar>	query_vars
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

idp		: /* empty */
				| idp namespace 
				| idp vocabulary 
				| idp theory
				| idp structure
				| idp asp_structure
				| idp instructions
				| idp options
				| idp namedquery
				| idp namedterm
				| idp using
		        ;
	
namespace		: NAMESPACE_HEADER namespace_name '{' idp '}'	{ getInserter().closespace();	}
				;

namespace_name	: identifier									{ getInserter().openspace(*$1,@1); }
				;

using			: USINGNAMESPACE pointer_name					{ getInserter().usingspace(*$2,@1); delete($2);	}
				| USINGVOCABULARY pointer_name					{ getInserter().usingvocab(*$2,@1); delete($2);	}
				;

 
/***************************
	Vocabulary declaration
***************************/

/** Structure of vocabulary declaration **/

vocabulary			: VOCAB_HEADER vocab_name '{' vocab_content '}'		{ getInserter().closevocab();	}	
					| VOCAB_HEADER vocab_name '=' function_call			{ getInserter().assignvocab($4,@2);
																		  getInserter().closevocab();	}
					;

vocab_name			: identifier	{ getInserter().openvocab(*$1,@1); }
					;

vocab_pointer		: pointer_name	{ getInserter().setvocab(*$1,@1); delete($1); }
					;

vocab_content		: /* empty */
					| vocab_content symbol_declaration
					| vocab_content EXTERN extern_symbol
					| vocab_content EXTERN VOCABULARY pointer_name	{ getInserter().externvocab(*$4,@4); delete($4);	}
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

extern_sort			: TYPE sort_pointer	{ getInserter().sort($2);			}
					;

extern_predicate	: pointer_name '[' sort_pointer_tuple ']'	{ getInserter().predicate(getInserter().predpointer(*$1,*$3,@1));
																  delete($1); delete($3); }
					| pointer_name '/' INTEGER					{ getInserter().predicate(getInserter().predpointer(*$1,$3,@1));
																  delete($1); }
					;

extern_function		: pointer_name '[' sort_pointer_tuple ':' sort_pointer ']' 
																	{ getInserter().function(getInserter().funcpointer(*$1,*$3,@1));
																	  delete($1); delete($3); }
					| pointer_name '/' INTEGER ':' INTEGER			{ getInserter().function(getInserter().funcpointer(*$1,$3,@1));
																	  delete($1); }
					;

/** Symbol declarations **/

sort_decl		: TYPE identifier											{ $$ = getInserter().sort(*$2,@2);		}
				| TYPE identifier ISA nonempty_spt							{ $$ = getInserter().sort(*$2,*$4,true,@2);	
																			  delete($4); }
				| TYPE identifier EXTENDS nonempty_spt						{ $$ = getInserter().sort(*$2,*$4,false,@2);	
																			  delete($4); }
				| TYPE identifier ISA nonempty_spt EXTENDS nonempty_spt		{ $$ = getInserter().sort(*$2,*$4,*$6,@2);		
																			  delete($4); delete($6); }
				| TYPE identifier EXTENDS nonempty_spt ISA nonempty_spt		{ $$ = getInserter().sort(*$2,*$6,*$4,@2);		
																			  delete($4); delete($6); }
				;

pred_decl		: identifier '(' sort_pointer_tuple ')'	{ getInserter().predicate(*$1,*$3,@1); delete($3); }
				| identifier							{ getInserter().predicate(*$1,@1);  }
				;

func_decl		: PARTIAL full_func_decl				{ $$ = $2; getInserter().partial($$);	}
				| full_func_decl						{ $$ = $1;								}
				| PARTIAL arit_func_decl				{ $$ = $2; getInserter().partial($$);	}
				| arit_func_decl						{ $$ = $1;								}
				;

full_func_decl	: identifier '(' sort_pointer_tuple ')' ':' sort_pointer	{ $$ = getInserter().function(*$1,*$3,$6,@1);
																			  delete($3); }
				| identifier ':' sort_pointer								{ $$ = getInserter().function(*$1,$3,@1); }	
				; 														

arit_func_decl	: '-' binary_arit_func_sorts				{ $$ = getInserter().aritfunction("-/2",*$2,@1); delete($2);	}
                | '+' binary_arit_func_sorts				{ $$ = getInserter().aritfunction("+/2",*$2,@1); delete($2);	}
                | '*' binary_arit_func_sorts				{ $$ = getInserter().aritfunction("*/2",*$2,@1); delete($2);	}
                | '/' binary_arit_func_sorts				{ $$ = getInserter().aritfunction("//2",*$2,@1); delete($2);	}
                | '%' binary_arit_func_sorts				{ $$ = getInserter().aritfunction("%/2",*$2,@1); delete($2);	}
                | '^' binary_arit_func_sorts				{ $$ = getInserter().aritfunction("^/2",*$2,@1); delete($2);	}
                | '-' unary_arit_func_sorts  %prec UMINUS	{ $$ = getInserter().aritfunction("-/1",*$2,@1); delete($2);	}
                | ABS unary_arit_func_sorts					{ $$ = getInserter().aritfunction("abs/1",*$2,@1); delete($2);	}
				;

binary_arit_func_sorts	: '(' sort_pointer ',' sort_pointer ')' ':' sort_pointer	
							{ $$ = new std::vector<Sort*>(3); (*$$)[0] = $2; (*$$)[1] = $4; (*$$)[2] = $7; }
						;

unary_arit_func_sorts	: '(' sort_pointer ')' ':' sort_pointer	
							{ $$ = new std::vector<Sort*>(2); (*$$)[0] = $2; (*$$)[1] = $5; }
						;

/** Symbol pointers **/

sort_pointer		: pointer_name				{ $$ = getInserter().sortpointer(*$1,@1); delete($1); }
					;

intern_pointer		: pointer_name '[' sort_pointer_tuple ']'	{ $$ = getInserter().internpredpointer(*$1,*$3,@1);
																  delete($1); delete($3); }
					| pointer_name '[' sort_pointer_tuple ':' sort_pointer ']'	
																{ $$ = getInserter().internfuncpointer(*$1,*$3,$5,@1);
																  delete($1); delete($3); }
					| pointer_name								{ $$ = getInserter().internpointer(*$1,@1);
																  delete($1); }
					;

pointer_name		: pointer_name "::" identifier	{ $$ = $1; $$->push_back(*$3); 		}
					| identifier					{ $$ = new std::vector<std::string>(1,*$1);	}
					;

/**************
	Queries
**************/

namedquery	: QUERY_HEADER query_name ':' vocab_pointer '{' query '}'	{ getInserter().closequery($6);	}
			;

query_name	: identifier	{ getInserter().openquery(*$1,@1);	}
			;

query		: '{' query_vars ':' formula '}'		{ $$ = getInserter().query(*$2,$4,@1); delete($2);	}
			;

query_vars	: /* empty */			{ $$ = new std::vector<Variable*>(0);	}
			| query_vars variable	{ $$ = $1; $$->push_back($2);		}
			;

/******************
	Named terms
******************/

namedterm	: TERM_HEADER term_name ':' vocab_pointer '{' term '}'	{ getInserter().closeterm($6);	}
			;

term_name	: identifier	{ getInserter().openterm(*$1,@1);	}
			;

/*************
	Theory	
*************/

theory		: THEORY_HEADER theory_name ':' vocab_pointer '{' def_forms '}'		{ getInserter().closetheory();	}
			| THEORY_HEADER theory_name '=' function_call						{ getInserter().assigntheory($4,@2); 
																				  getInserter().closetheory();	}
			;

theory_name	: identifier	{ getInserter().opentheory(*$1,@1);  }
			;

function_call	: pointer_name '(' pointer_names ')'	{ $$ = getInserter().call(*$1,*$3,@1); delete($1); delete($3);	}
				| pointer_name '(' ')'					{ $$ = getInserter().call(*$1,@1); delete($1);					}
				;

pointer_names	: pointer_names ',' pointer_name	{ $$ = $1; $$->push_back(*$3);
													  delete($3); }
				| pointer_name						{ $$ = new std::vector<std::vector<std::string> >(1,*$1);
													  delete($1); }
				;

def_forms	: /* empty */
			| def_forms definition		{ getInserter().definition($2);	}
			| def_forms formula '.'		{ getInserter().sentence($2);		}
			| def_forms fixpdef			{ getInserter().fixpdef($2);		}
			| def_forms using
			| def_forms error '.' 
			| def_forms '{' error '}'
			;

/** Definitions **/

definition	: '{' rules '}'		{ $$ = getInserter().definition(*$2); delete($2);	}
			;

rules		: rules rule '.'	{ $$ = $1; $1->push_back($2);	}				
			| rule '.'			{ $$ = new std::vector<Rule*>(1,$1);	}			
			;

rule		: '!' variables ':' head "<-" formula	{ $$ = getInserter().rule(*$2,$4,$6,@1); delete($2);	}
			| '!' variables ':' head "<-"			{ $$ = getInserter().rule(*$2,$4,@1); delete($2);		}
			| '!' variables ':' head				{ $$ = getInserter().rule(*$2,$4,@1); delete($2);		}
			| head "<-" formula						{ $$ = getInserter().rule($1,$3,@1);	}
			| head "<-"								{ $$ = getInserter().rule($1,@1);		}
			| head									{ $$ = getInserter().rule($1,@1);		}
			;

head		: predicate										{ $$ = $1;												}
			| intern_pointer '(' term_tuple ')' '=' term	{ $$ = getInserter().funcgraphform($1,*$3,$6,@1); delete($3);	}
			| intern_pointer '(' ')' '=' term				{ $$ = getInserter().funcgraphform($1,$5,@1);					}
			| intern_pointer '=' term						{ $$ = getInserter().funcgraphform($1,$3,@1);					}
			;


/** Fixpoint definitions **/

fixpdef		: LFD '[' fd_rules ']'		{ $$ = $3; getInserter().makeLFD($3,true); 	}
			| GFD '[' fd_rules ']'		{ $$ = $3; getInserter().makeLFD($3,false);	}
			;

fd_rules	: fd_rules rule	'.'			{ $$ = $1; getInserter().addRule($$,$2);					}
			| fd_rules fixpdef			{ $$ = $1; getInserter().addDef($$,$2);					}
			| rule '.'					{ $$ = getInserter().createFD(); getInserter().addRule($$,$1);	}
			| fixpdef					{ $$ = getInserter().createFD(); getInserter().addDef($$,$1);		}
			;	

/** Formulas **/

formula		: '!' variables ':' formula					{ $$ = getInserter().univform(*$2,$4,@1); delete($2);		}
            | '?' variables ':' formula					{ $$ = getInserter().existform(*$2,$4,@1); delete($2);	}
			| '?' INTEGER  variables ':' formula		{ $$ = getInserter().bexform(CompType::EQ,$2,*$3,$5,@1);
														  delete($3);										}
			| '?' '=' INTEGER variables ':' formula		{ $$ = getInserter().bexform(CompType::EQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '<' INTEGER variables ':' formula		{ $$ = getInserter().bexform(CompType::LT,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '>' INTEGER variables ':' formula		{ $$ = getInserter().bexform(CompType::GT,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' "=<" INTEGER variables ':' formula	{ $$ = getInserter().bexform(CompType::LEQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' ">=" INTEGER variables ':' formula	{ $$ = getInserter().bexform(CompType::GEQ,$3,*$4,$6,@1);
														  delete($4);										}
			| '~' formula								{ $$ = $2; getInserter().negate($$);						}
            | formula '&' formula						{ $$ = getInserter().conjform($1,$3,@1);					}
            | formula '|' formula						{ $$ = getInserter().disjform($1,$3,@1);					}
            | formula "=>" formula						{ $$ = getInserter().implform($1,$3,@1);					}
            | formula "<=" formula						{ $$ = getInserter().revimplform($1,$3,@1);				}
            | formula "<=>" formula						{ $$ = getInserter().equivform($1,$3,@1);					}
            | '(' formula ')'							{ $$ = $2;											}
			| TRUE										{ $$ = getInserter().trueform(@1);						}
			| FALSE										{ $$ = getInserter().falseform(@1);						}
			| eq_chain									{ $$ = $1;											}
            | predicate									{ $$ = $1;											}
            ;

predicate   : intern_pointer							{ $$ = getInserter().predform($1,@1);					}
			| intern_pointer '(' ')'					{ $$ = getInserter().predform($1,@1);					}
            | intern_pointer '(' term_tuple ')'			{ $$ = getInserter().predform($1,*$3,@1); delete($3); }
            ;

eq_chain	: eq_chain '='  term	{ $$ = getInserter().eqchain(CompType::EQ,$1,$3,@1);	}
			| eq_chain "~=" term	{ $$ = getInserter().eqchain(CompType::NEQ,$1,$3,@1);	}
			| eq_chain '<'  term	{ $$ = getInserter().eqchain(CompType::LT,$1,$3,@1);	}	
			| eq_chain '>'  term    { $$ = getInserter().eqchain(CompType::GT,$1,$3,@1);	}
			| eq_chain "=<" term	{ $$ = getInserter().eqchain(CompType::LEQ,$1,$3,@1);	}		
			| eq_chain ">=" term	{ $$ = getInserter().eqchain(CompType::GEQ,$1,$3,@1);	}		
			| term '='  term		{ $$ = getInserter().eqchain(CompType::EQ,$1,$3,@1);	}
            | term "~=" term		{ $$ = getInserter().eqchain(CompType::NEQ,$1,$3,@1);	}
            | term '<'  term		{ $$ = getInserter().eqchain(CompType::LT,$1,$3,@1);	}
            | term '>'  term		{ $$ = getInserter().eqchain(CompType::GT,$1,$3,@1);	}
            | term "=<" term		{ $$ = getInserter().eqchain(CompType::LEQ,$1,$3,@1);	}
            | term ">=" term		{ $$ = getInserter().eqchain(CompType::GEQ,$1,$3,@1);	}
			;

variables   : variables variable	{ $$ = $1; $$->insert($2);						}		
            | variable				{ $$ = new std::set<Variable*>;	$$->insert($1);	}
			;

variable	: identifier							{ $$ = getInserter().quantifiedvar(*$1,@1);		}
			| identifier '[' theosort_pointer ']'	{ $$ = getInserter().quantifiedvar(*$1,$3,@1);	}
			;

theosort_pointer	:	pointer_name		{ $$ = getInserter().theosortpointer(*$1,@1); delete($1);	}
					;

/** Terms **/                                            

term		: function		{ $$ = $1;	}		
			| arterm		{ $$ = $1;	}
			| domterm		{ $$ = $1;	}
			| aggterm		{ $$ = $1;	}
			;

function	: intern_pointer '(' term_tuple ')'		{ $$ = getInserter().functerm($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = getInserter().functerm($1);					}
			| intern_pointer						{ $$ = getInserter().functerm($1);					}
			;

arterm		: term '-' term				{ $$ = getInserter().arterm('-',$1,$3,@1);	}				
			| term '+' term				{ $$ = getInserter().arterm('+',$1,$3,@1);	}
			| term '*' term				{ $$ = getInserter().arterm('*',$1,$3,@1);	}
			| term '/' term				{ $$ = getInserter().arterm('/',$1,$3,@1);	}
			| term '%' term				{ $$ = getInserter().arterm('%',$1,$3,@1);	}
			| term '^' term				{ $$ = getInserter().arterm('^',$1,$3,@1);	}
			| '-' term %prec UMINUS		{ $$ = getInserter().arterm("-",$2,@1);		}
			| ABS '(' term ')'			{ $$ = getInserter().arterm("abs",$3,@1);		}
			| '(' arterm ')'			{ $$ = $2;								}
			;

domterm		: INTEGER									{ $$ = getInserter().domterm($1,@1);		}
			| FLNUMBER									{ $$ = getInserter().domterm($1,@1);		}
			| STRINGCONS								{ $$ = getInserter().domterm($1,@1);		}
			| CHARCONS									{ $$ = getInserter().domterm($1,@1);		}
			| '@' identifier '[' theosort_pointer ']'	{ $$ = getInserter().domterm($2,$4,@1);	}
			| '@' identifier							{ $$ = getInserter().domterm($2,0,@1);	}
			;

aggterm		: P_CARD formulaset	{ $$ = getInserter().aggregate(AggFunction::CARD,$2,@1);	}
			| P_SOM termset		{ $$ = getInserter().aggregate(AggFunction::SUM,$2,@1);		}
			| P_PROD termset		{ $$ = getInserter().aggregate(AggFunction::PROD,$2,@1);	}
			| P_MINAGG termset	{ $$ = getInserter().aggregate(AggFunction::MIN,$2,@1);		}
			| P_MAXAGG termset	{ $$ = getInserter().aggregate(AggFunction::MAX,$2,@1);		}
			;

formulaset		: '{' variables ':' formula '}'				{ $$ = getInserter().set(*$2,$4,@1); delete($2);	}
				| '[' form_list ']'							{ $$ = getInserter().set($2);						}
				;

termset			: '{' variables ':' formula ':' term '}'	{ $$ = getInserter().set(*$2,$4,$6,@1); delete($2);	}	
				| '[' form_term_list ']'					{ $$ = getInserter().set($2);							}
				;

form_list		: form_list ';' formula						{ $$ = $1; getInserter().addFormula($$,$3);						}
				| formula									{ $$ = getInserter().createEnum(@1);	getInserter().addFormula($$,$1);	}		
				;

form_term_list	: form_term_list ';' '(' formula ',' term ')'	{ $$ = $1; getInserter().addFT($$,$4,$6);						}
				| '(' formula ',' term ')'						{ $$ = getInserter().createEnum(@1); getInserter().addFT($$,$2,$4);	}
				;


/**********************
	Input structure
**********************/

structure		: STRUCT_HEADER struct_name ':' vocab_pointer '{' interpretations '}'	{ getInserter().closestructure();	}
				| STRUCT_HEADER struct_name '=' function_call							{ getInserter().assignstructure($4,@2);
																						  getInserter().closestructure();	}
				;

struct_name		: identifier	{ getInserter().openstructure(*$1,@1); }
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

empty_inter	: intern_pointer '=' '{' '}'				{ getInserter().emptyinter($1); }
			;

/** Interpretations with arity 1 **/

sort_inter	: intern_pointer '=' '{' elements_es '}'		{ getInserter().sortinter($1,$4);  }	
			;

elements_es		: elements ';'						{ $$ = $1;	}
				| elements							{ $$ = $1;	}
				;

elements		: elements ';' charrange			{ $$ = $1; getInserter().addElement($$,$3->first,$3->second); delete($3);	}
				| elements ';' intrange				{ $$ = $1; getInserter().addElement($$,$3->first,$3->second); delete($3);	}
				| elements ';' '(' strelement ')'	{ $$ = $1; getInserter().addElement($$,$4);						}
				| elements ';' '(' integer ')'		{ $$ = $1; getInserter().addElement($$,$4);						}
				| elements ';' '(' compound ')'		{ $$ = $1; getInserter().addElement($$,$4);						}
				| elements ';' '(' floatnr ')'		{ $$ = $1; getInserter().addElement($$,$4);						}
				| elements ';' integer				{ $$ = $1; getInserter().addElement($$,$3);						}
				| elements ';' strelement			{ $$ = $1; getInserter().addElement($$,$3);						}
				| elements ';' floatnr				{ $$ = $1; getInserter().addElement($$,$3);						}
				| elements ';' compound				{ $$ = $1; getInserter().addElement($$,$3);						}
				| charrange							{ $$ = getInserter().createSortTable(); 
													  getInserter().addElement($$,$1->first,$1->second); delete($1);	}
				| intrange							{ $$ = getInserter().createSortTable(); 
													  getInserter().addElement($$,$1->first,$1->second); delete($1);	}
				| '(' strelement ')'				{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$2);	}
				| '(' integer ')'					{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$2);	}
				| '(' floatnr ')'                   { $$ = getInserter().createSortTable(); getInserter().addElement($$,$2);	}
				| '(' compound ')'					{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$2);	}
				| strelement						{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$1);	}
				| integer							{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$1);	}
				| floatnr							{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$1);	}
				| compound							{ $$ = getInserter().createSortTable(); getInserter().addElement($$,$1);	}
				;

strelement		: identifier	{ $$ = $1;									}
				| STRINGCONS	{ $$ = $1;									}
				| CHARCONS		{ $$ = StringPointer(std::string(1,$1));	}
				;

/** Interpretations with arity not 1 **/

pred_inter		: intern_pointer '=' '{' ptuples_es '}'		{ getInserter().predinter($1,$4);		}
				| intern_pointer '=' TRUE					{ getInserter().truepredinter($1);		}
				| intern_pointer '=' FALSE					{ getInserter().falsepredinter($1);	}
				;

ptuples_es		: ptuples ';'					{ $$ = $1;	}	
				| ptuples						{ $$ = $1;	}
				;

ptuples			: ptuples ';' '(' ptuple ')'	{ $$ = $1; getInserter().addTuple($$,*$4,@4); delete($4);	}
				| ptuples ';' ptuple			{ $$ = $1; getInserter().addTuple($$,*$3,@3); delete($3);	}
				| ptuples ';' emptyptuple		{ $$ = $1; getInserter().addTuple($$,@3);					}
				| '(' ptuple ')'				{ $$ = getInserter().createPredTable($2->size()); getInserter().addTuple($$,*$2,@2); delete($2);	}
				| ptuple						{ $$ = getInserter().createPredTable($1->size()); getInserter().addTuple($$,*$1,@1); delete($1);	}
				| emptyptuple					{ $$ = getInserter().createPredTable(0); getInserter().addTuple($$,@1);					}
				;

emptyptuple		: '(' ')'										
				;

ptuple			: ptuple ',' pelement			{ $$ = $1; $$->push_back($3);	}			
				| pelement ',' pelement			{ $$ = new std::vector<const DomainElement*>(); 
												  $$->push_back($1); $$->push_back($3); }
				;

pelement		: integer		{ $$ = getInserter().element($1);	}
				| identifier	{ $$ = getInserter().element($1);	}
				| CHARCONS		{ $$ = getInserter().element($1);	}
				| STRINGCONS	{ $$ = getInserter().element($1);	}
				| floatnr		{ $$ = getInserter().element($1);	}
				| compound		{ $$ = getInserter().element($1);	}
				;

/** Interpretations for functions **/

func_inter	: intern_pointer '=' '{' ftuples_es '}'	{ getInserter().funcinter($1,$4); }	
			| intern_pointer '=' pelement			{ FuncTable* ft = getInserter().createFuncTable(1);
													  getInserter().addTupleVal(ft,$3,@3);
													  getInserter().funcinter($1,ft); }
			| intern_pointer '=' CONSTRUCTOR		{ getInserter().constructor($1);	}
			;

ftuples_es		: ftuples ';'					{ $$ = $1;	}
				| ftuples						{ $$ = $1;	}
				;

ftuples			: ftuples ';' ftuple			{ $$ = $1; getInserter().addTupleVal($$,*$3,@3); delete($3);					    }
				| ftuple						{ $$ = getInserter().createFuncTable($1->size()); getInserter().addTupleVal($$,*$1,@1);	delete($1);	}
				;

ftuple			: ptuple "->" pelement			{ $$ = $1; $$->push_back($3);	}			
				| pelement "->" pelement		{ $$ = new std::vector<const DomainElement*>(1,$1); $$->push_back($3);	}
				| emptyptuple "->" pelement		{ $$ = new std::vector<const DomainElement*>(1,$3);	}		
				| "->" pelement					{ $$ = new std::vector<const DomainElement*>(1,$2);	}
				;

f3tuples_es		: f3tuples ';'					{ $$ = $1;	}
				| f3tuples						{ $$ = $1;	}
				;

f3tuples		: f3tuples ';' ftuple			{ $$ = $1; getInserter().addTuple($$,*$3,@3); delete($3);							}
				| ftuple						{ $$ = getInserter().createPredTable($1->size()); getInserter().addTuple($$,*$1,@1); delete($1);	}
				;

/** Procedural interpretations **/

proc_inter		: intern_pointer '=' PROCEDURE pointer_name	{ getInserter().inter($1,*$4,@1); delete($4);	}
				;

/** Three-valued interpretations **/

three_inter		: threepred_inter
				| threefunc_inter
				| threeempty_inter
				;

threeempty_inter	: intern_pointer '<' identifier '>' '=' '{' '}'			{ getInserter().emptythreeinter($1,*$3); }
					;

threepred_inter : intern_pointer '<' identifier '>' '=' '{' ptuples_es '}'	{ getInserter().threepredinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' '{' elements_es '}'	{ getInserter().threepredinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' TRUE				{ getInserter().truethreepredinter($1,*$3);	}
				| intern_pointer '<' identifier '>' '=' FALSE				{ getInserter().falsethreepredinter($1,*$3);	}
				;

threefunc_inter	: intern_pointer '<' identifier '>' '=' '{' f3tuples_es '}'	{ getInserter().threefuncinter($1,*$3,$7);		}
				| intern_pointer '<' identifier '>' '=' pelement			{ PredTable* ft = getInserter().createPredTable(1);
																			  std::vector<const DomainElement*> vd(1,$6);
																			  getInserter().addTuple(ft,vd,@6);
																			  getInserter().threefuncinter($1,*$3,ft);		}
				;

/** Ranges **/

intrange	: integer ".." integer			{ $$ = getInserter().range($1,$3,@1); }
			;
charrange	: CHARACTER ".." CHARACTER		{ $$ = getInserter().range($1,$3,@1);	}
			| CHARCONS ".." CHARCONS		{ $$ = getInserter().range($1,$3,@1);	}
			;

/** Compound elements **/

compound	: intern_pointer '(' compound_args ')'	{ $$ = getInserter().compound($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = getInserter().compound($1);					}
			;

compound_args	: compound_args ',' floatnr		{ $$ = $1; $$->push_back(getInserter().element($3));	}
				| compound_args ',' integer		{ $$ = $1; $$->push_back(getInserter().element($3));	}
				| compound_args ',' strelement	{ $$ = $1; $$->push_back(getInserter().element($3));	}
				| compound_args ',' compound	{ $$ = $1; $$->push_back(getInserter().element($3));	}
				| floatnr						{ $$ = new std::vector<const DomainElement*>(1,getInserter().element($1));	}
				| integer						{ $$ = new std::vector<const DomainElement*>(1,getInserter().element($1));	}
				| strelement					{ $$ = new std::vector<const DomainElement*>(1,getInserter().element($1));	}
				| compound						{ $$ = new std::vector<const DomainElement*>(1,getInserter().element($1));	}
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

asp_structure	: ASP_HEADER struct_name ':' vocab_pointer '{' atoms '}'	{ getInserter().closestructure();	}
				;

atoms	: /* empty */
		| atoms atom '.'
		| atoms using
		;

atom	: predatom
		| funcatom
		;

predatom	: intern_pointer '(' domain_tuple ')'		{ getInserter().predatom($1,*$3,true);	delete($3);		}
			| intern_pointer '(' ')'					{ getInserter().predatom($1,true);						}
			| intern_pointer							{ getInserter().predatom($1,true);						}
			| '-' intern_pointer '(' domain_tuple ')'	{ getInserter().predatom($2,*$4,false); delete($4);	}
			| '-' intern_pointer '(' ')'				{ getInserter().predatom($2,false);					}
			| '-' intern_pointer						{ getInserter().predatom($2,false);					}
			; 

funcatom	: intern_pointer '(' domain_tuple ')' '=' domain_element		{ getInserter().funcatom($1,*$3,$6,true); delete($3);	}
			| intern_pointer '(' ')' '=' domain_element				        { getInserter().funcatom($1,$5,true);		}
			| intern_pointer '=' domain_element						        { getInserter().funcatom($1,$3,true);		}
			| '-' intern_pointer '(' domain_tuple ')' '=' domain_element	{ getInserter().funcatom($2,*$4,$7,false);	delete($4);	}
			| '-' intern_pointer '(' ')' '=' domain_element				    { getInserter().funcatom($2,$6,false);		}
			| '-' intern_pointer '=' domain_element						    { getInserter().funcatom($2,$4,false);		}
			;

domain_tuple	: domain_tuple ',' domain_element	{ $$ = getInserter().domaintuple($1,$3);				}
				| domain_tuple ',' intrange			{ $$ = getInserter().domaintuple($1,$3); delete($3);	}
				| domain_tuple ',' charrange		{ $$ = getInserter().domaintuple($1,$3); delete($3);	}
				| domain_element					{ $$ = getInserter().domaintuple($1);					}
				| intrange							{ $$ = getInserter().domaintuple($1); delete($1);		}
				| charrange							{ $$ = getInserter().domaintuple($1); delete($1);		}
				;

domain_element	: strelement	{ $$ = getInserter().element($1); }
				| integer		{ $$ = getInserter().element($1); }
				| floatnr		{ $$ = getInserter().element($1); }
				| compound		{ $$ = getInserter().element($1); }
				;

term_tuple		: term_tuple ',' term						{ $$ = $1; $$->push_back($3);		}	
				| term										{ $$ = new std::vector<Term*>(1,$1);		}	
				;

sort_pointer_tuple	: /* empty */							{ $$ = new std::vector<Sort*>(0);		}
					| nonempty_spt							{ $$ = $1;							}
					;
					
nonempty_spt		: nonempty_spt ',' sort_pointer	{ $$ = $1; $$->push_back($3);		}
					| sort_pointer							{ $$ = new std::vector<Sort*>(1,$1);		}
					;


/*******************
	Instructions
*******************/

instructions		: PROCEDURE_HEADER proc_name proc_sig '{' LUACHUNK 		{ getInserter().closeprocedure($5); delete($5);	}
					;

proc_name			: identifier	{ getInserter().openprocedure(*$1,@1);	}
					;

proc_sig			: '(' ')'		
					| '(' args ')'
					;

args				: args ',' identifier	{ getInserter().procarg(*$3);		}
					| identifier			{ getInserter().procarg(*$1);		}
					;


/**************
	Options
**************/

options	: OPTION_HEADER option_name '{' optassigns '}'	{ getInserter().closeoptions();	}
		| OPTION_HEADER option_name 
			EXTERN pointer_name { getInserter().externoption(*$4,@4);	delete($4);	}
			'{' optassigns '}'	{ getInserter().closeoptions();	}
		; // Second rule allows to define an option as using by default the values of the provided option

option_name	: identifier	{ getInserter().openoptions(*$1,@1);	}
			;

optassigns	: /* empty */
			| optassigns optassign	
			;

optassign	: identifier '=' strelement		{ getInserter().option(*$1,*$3,@1);	}
			| identifier '=' floatnr		{ getInserter().option(*$1,$3,@1);		}
			| identifier '=' integer		{ getInserter().option(*$1,$3,@1);		}
			| identifier '=' TRUE			{ getInserter().option(*$1,true,@1);	}
			| identifier '=' FALSE			{ getInserter().option(*$1,false,@1);	}
			;


%%

#include <iostream>
#include "error.hpp"

extern FILE* yyin;
extern void reset();

void yyerror(const char* s) {
	ParseInfo pi(yylloc.first_line,yylloc.first_column,getInserter().currfile());
	Error::error(pi);
	std::cerr << s << std::endl;
}

void parsefile(const std::string& str) {
	reset();
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yylloc.descr = 0;
	yyin = GlobalData::instance()->openFile(str.c_str(),"r");
	if(yyin) {
		getInserter().currfile(str);
		std::cerr <<"parsing " <<str <<"\n";
		yyparse();
		GlobalData::instance()->closeFile(yyin);
	}
	else{
		Error::unknfile(str);
	}
}

void parsestdin() {
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yylloc.descr = 0;
	yyin = stdin;
	getInserter().currfile(NULL);
	yyparse();
}

Insert& getInserter(){
	return GlobalData::instance()->getInserter();
}
