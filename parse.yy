/************************************
	parse.yy	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

%{

#include <iostream>
#include "insert.hpp"
#include "error.hpp"
#include "builtin.hpp"

// Lexer
extern int yylex();

// Parsing tables efficiently
FinitePredTable*	_currtable = 0;
unsigned int		_currrow(0);
unsigned int		_currcol(0);
bool				_wrongarity = false;
void				addEmpty();
void				addInt(int,YYLTYPE);
void				addFloat(double,YYLTYPE);
void				addString(string*,YYLTYPE);
void				addCompound(compound*,YYLTYPE);
void				closeRow(YYLTYPE);
void				closeTable();

// Parsing ASP structures efficiently
vector<Element>				_currelements;
vector<FiniteSortTable*>	_currranges;
vector<ElementType>			_currtypes;
vector<bool>				_iselement;
void						closeTuple();

// Errors
void yyerror(const char* s);

// Common 
extern string itos(int);
extern string dtos(double);

%}

/** Produce readable error messages **/
%debug
%locations
%error-verbose

/** Data structures **/
%union{
	int					nmr;
	char				chr;
	double				dou;
	string*				str;
	InfArgType			iat;
	compound*			cpo;

	Sort*				sor;
	Predicate*			pre;
	Function*			fun;
	Term*				ter;
	PredForm*			prf;
	Formula*			fom;
	EqChainForm*		eqc;
	Rule*				rul;
	FiniteSortTable*	sta;
	FixpDef*			fpd;
	Definition*			def;
	Variable*			var;
	FTTuple*			ftt;
	NSTuple*			nst;
	SetExpr*			set;

	vector<int>*			vint;
	vector<char>*			vcha;
	vector<string>*			vstr;
	vector<Sort*>*			vsor;
	vector<Variable*>*		vvar;
	vector<Term*>*			vter;
	vector<Formula*>*		vfom;
	vector<Rule*>*			vrul;
	vector<FTTuple*>*		vftt;
	vector<TypedElement*>*	vtpe;

	vector<pair<Rule*,FixpDef*> >*	vprf;
}

/** Headers  **/
%token VOCAB_HEADER
%token THEORY_HEADER
%token STRUCT_HEADER
%token ASP_HEADER
%token ASP_BELIEF
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
%token CONSTR
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

/** Aliases **/
%token <operator> MAPS			"->"
%token <operator> EQUIV			"<=>"
%token <operator> IMPL			"=>"
%token <operator> RIMPL			"<="
%token <operator> DEFIMP		"<-"
%token <operator> NEQ			"~="
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
%type <sor> theosort_pointer
%type <pre> pred_decl
%type <nst>	intern_pointer
%type <fun> func_decl
%type <fun> full_func_decl
%type <fun> arit_func_decl
%type <ter> term
%type <ter> domterm
%type <ter> function
%type <ter> arterm
%type <ter> aggterm
%type <prf> predicate
%type <prf> head
%type <fom> formula
%type <eqc>	eq_chain
%type <rul> rule
%type <sta> elements_es
%type <sta> elements
%type <fpd> fixpdef
%type <def> definition
%type <ftt> form_term_tuple
%type <set> set
%type <cpo> compound

%type <vint> intrange
%type <vcha> charrange
%type <vter> term_tuple
%type <vvar> variables
%type <vfom> form_list
%type <vrul> rules
%type <vstr> pointer_name
%type <vsor> sort_pointer_tuple
%type <vsor> nonempty_spt
%type <vsor> binary_arit_func_sorts
%type <vsor> unary_arit_func_sorts
%type <vprf> fd_rules
%type <vftt> form_term_list
%type <vtpe> compound_args

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
	
namespace		: NAMESPACE_HEADER namespace_name '{' idp '}'	{ Insert::closespace();	}
				;

namespace_name	: identifier									{ Insert::openspace(*$1,@1); }
				;

using			: USING VOCABULARY pointer_name					{ Insert::usingvocab(*$3,@1); delete($3);	}
				| USING NAMESPACE pointer_name					{ Insert::usingspace(*$3,@1); delete($3);	}
				;

 
/***************************
	Vocabulary declaration
***************************/

/** Structure of vocabulary declaration **/

vocabulary			: VOCAB_HEADER vocab_name '{' vocab_content '}'		{ Insert::closevocab();	}	
					| VOCAB_HEADER vocab_name '=' function_call			{ /* TODO */ 
																		  Insert::closevocab();	}
					;


vocab_name			: identifier	{ Insert::openvocab(*$1,@1); }
					;

vocab_pointer		: pointer_name	{ Insert::setvocab(*$1,@1); delete($1); }
					;

vocab_content		: /* empty */
					| vocab_content symbol_declaration
					| vocab_content EXTERN extern_symbol
					| vocab_content EXTERN VOCABULARY pointer_name	{ Insert::externvocab(*$4,@4); delete($4);	}
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

extern_sort			: TYPE sort_pointer	{ Insert::sort($2);			}
					;

extern_predicate	: pointer_name '[' sort_pointer_tuple ']'	{ Insert::predicate(Insert::predpointer(*$1,*$3,@1));
																  delete($1); delete($3);
																}
					| pointer_name '/' INTEGER					{ $1->back() = $1->back() + '/' + itos($3);
																  Insert::predicate(Insert::predpointer(*$1,@1));
																  delete($1);
																}
					;

extern_function		: pointer_name '[' sort_pointer_tuple ':' sort_pointer ']' 
																	{ Insert::function(Insert::funcpointer(*$1,*$3,@1));
																	  delete($1); delete($3);
																	}
					| pointer_name '/' INTEGER ':' INTEGER			{ $1->back() = $1->back() + '/' + itos($3 - 1);
																	  Insert::function(Insert::funcpointer(*$1,@1));
																	  delete($1);
																	}
					;

/** Symbol declarations **/

sort_decl		: TYPE identifier											{ Insert::sort(*$2,@2);				}
				| TYPE identifier ISA nonempty_spt							{ Insert::sort(*$2,*$4,true,@2);	
																			  delete($4);
																			}
				| TYPE identifier EXTENDS nonempty_spt						{ Insert::sort(*$2,*$4,false,@2);	
																			  delete($4);
																			}
				| TYPE identifier ISA nonempty_spt EXTENDS nonempty_spt		{ Insert::sort(*$2,*$4,*$6,@2);		
																			  delete($4); delete($6);
																			}
				| TYPE identifier EXTENDS nonempty_spt ISA nonempty_spt		{ Insert::sort(*$2,*$6,*$4,@2);		
																			  delete($4); delete($6);
																			}
				;

pred_decl		: identifier '(' sort_pointer_tuple ')'	{ Insert::predicate(*$1,*$3,@1); delete($3); }
				| identifier							{ Insert::predicate(*$1,@1);  }
				;

func_decl		: PARTIAL CONSTR full_func_decl			{ $$ = $3; if($$) { $$->partial(true); $$->constructor(true); }	}
				| PARTIAL full_func_decl				{ $$ = $2; if($$) $$->partial(true);							}
				| CONSTR full_func_decl					{ $$ = $2; if($$) $$->constructor(true);						}
				| full_func_decl						{ $$ = $1;														}
				| PARTIAL arit_func_decl				{ $$ = $2; if($$) $$->partial(true);							}
				| arit_func_decl						{ $$ = $1;														}
				;

full_func_decl	: identifier '(' sort_pointer_tuple ')' ':' sort_pointer	{ Insert::function(*$1,*$3,$6,@1);
																			  delete($3); }
				| identifier ':' sort_pointer								{ Insert::function(*$1,$3,@1); }	
				; 														

arit_func_decl	: '-' binary_arit_func_sorts				{ $$ = Insert::function("-/2",*$2,@1); delete($2);		}
                | '+' binary_arit_func_sorts				{ $$ = Insert::function("+/2",*$2,@1); delete($2);		}
                | '*' binary_arit_func_sorts				{ $$ = Insert::function("*/2",*$2,@1); delete($2);		}
                | '/' binary_arit_func_sorts				{ $$ = Insert::function("//2",*$2,@1); delete($2);		}
                | '%' binary_arit_func_sorts				{ $$ = Insert::function("%/2",*$2,@1); delete($2);		}
                | '^' binary_arit_func_sorts				{ $$ = Insert::function("^/2",*$2,@1); delete($2);		}
                | '-' unary_arit_func_sorts  %prec UMINUS	{ $$ = Insert::function("-/1",*$2,@1); delete($2);		}
                | ABS unary_arit_func_sorts					{ $$ = Insert::function("abs/1",*$2,@1); delete($2);	}
				;

binary_arit_func_sorts	: '(' sort_pointer ',' sort_pointer ')' ':' sort_pointer	{ $$ = new vector<Sort*>(3);
																					  (*$$)[0] = $2;
																					  (*$$)[1] = $4;
																					  (*$$)[2] = $7;
																					}
						;

unary_arit_func_sorts	: '(' sort_pointer ')' ':' sort_pointer	{ $$ = new vector<Sort*>(2);
																  (*$$)[0] = $2; (*$$)[1] = $5;
																}
						;

/** Symbol pointers **/

sort_pointer		: pointer_name				{ $$ = Insert::sortpointer(*$1,@1); delete($1); }
					;

intern_pointer		: pointer_name '[' sort_pointer_tuple ']'	{ $$ = Insert::internpointer(*$1,*$3,@1);
																  delete($1); delete($3);
																}
					| pointer_name '[' sort_pointer_tuple ':' sort_pointer ']'	
																{ $3->push_back($5);
																  $$ = Insert::internpointer(*$1,*$3,@1);
																  $$->func(true);
																  delete($1); delete($3);
																}
					| pointer_name								{ $$ = Insert::internpointer(*$1,@1);
																  delete($1); 
																}
					;

pointer_name		: pointer_name "::" identifier	{ $$ = $1; $$->push_back(*$3); 		}
					| identifier					{ $$ = new vector<string>(1,*$1);	}
					;

/*************
	Theory	
*************/

theory		: THEORY_HEADER theory_name ':' vocab_pointer '{' def_forms '}'		{ Insert::closetheory();	}
			| THEORY_HEADER theory_name '=' function_call						{ /* TODO */ 
																				  Insert::closetheory();	}
			;

theory_name	: identifier	{ Insert::opentheory(*$1,@1);  }
			;

function_call	: pointer_name '(' pointer_names ')'	/* TODO */
				| pointer_name '(' ')'					/* TODO */
				;

pointer_names	: pointer_names ',' pointer_name	/* TODO */
				| pointer_name						/* TODO */
				;

def_forms	: /* empty */
			| def_forms definition		{ Insert::definition($2);	}
			| def_forms formula '.'		{ Insert::sentence($2);		}
			| def_forms fixpdef			{ Insert::fixpdef($2);		}
			| def_forms using
			| def_forms error '.' 
			| def_forms '{' error '}'
			;

/** Definitions **/

definition	: '{' rules '}'		{ $$ = Insert::definition(*$2); delete($2);	}
			;

rules		: rules rule '.'	{ $$ = $1; $1->push_back($2);	}				
			| rule '.'			{ $$ = new vector<Rule*>(1,$1);	}			
			;

rule		: '!' variables ':' head "<-" formula	{ $$ = Insert::rule(*$2,$4,$6,@1); delete($2);	}
			| '!' variables ':' head "<-"			{ $$ = Insert::rule(*$2,$4,@1); delete($2);		}
			| '!' variables ':' head				{ $$ = Insert::rule(*$2,$4,@1); delete($2);		}
			| head "<-" formula						{ $$ = Insert::rule($1,$3,@1);	}
			| head "<-"								{ $$ = Insert::rule($1,@1);		}
			| head									{ $$ = Insert::rule($1,@1);		}
			;

head		: predicate										{ $$ = $1;	}				
			| intern_pointer '(' term_tuple ')' '=' term	{ $$ = Insert::funcgraphform($1,*$3,$6,@1); delete($3);	}
			| intern_pointer '(' ')' '=' term				{ $$ = Insert::funcgraphform($1,$5,@1);					}
			| intern_pointer '=' term						{ $$ = Insert::funcgraphform($1,$3,@1);					}
			;


/** Fixpoint definitions **/

fixpdef		: LFD '[' fd_rules ']'		{ $$ = Insert::fixpdef(true,*$3); delete($3);	}
			| GFD '[' fd_rules ']'		{ $$ = Insert::fixpdef(false,*$3); delete($3);	}
			;

fd_rules	: fd_rules rule	'.'			{ $$ = $1; $$->push_back(pair<Rule*,FixpDef*>($2,0));					}
			| fd_rules fixpdef			{ $$ = $1; $$->push_back(pair<Rule*,FixpDef*>(0,$2));					}
			| rule '.'					{ $$ = new vector<pair<Rule*,FixpDef*> >(1,pair<Rule*,FixpDef*>($1,0));	}
			| fixpdef					{ $$ = new vector<pair<Rule*,FixpDef*> >(1,pair<Rule*,FixpDef*>(0,$1));	}
			;	

/** Formulas **/

formula		: '!' variables ':' formula					{ $$ = Insert::univform(*$2,$4,@1); delete($2);		}
            | '?' variables ':' formula					{ $$ = Insert::existform(*$2,$4,@1); delete($2);	}
			| '?' INTEGER  variables ':' formula		{ $$ = Insert::bexform('=',true,$2,*$3,$5,@1);
														  delete($3);										}
			| '?' '=' INTEGER variables ':' formula		{ $$ = Insert::bexform('=',true,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '<' INTEGER variables ':' formula		{ $$ = Insert::bexform('<',true,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' '>' INTEGER variables ':' formula		{ $$ = Insert::bexform('>',true,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' "=<" INTEGER variables ':' formula	{ $$ = Insert::bexform('>',false,$3,*$4,$6,@1);
														  delete($4);										}
			| '?' ">=" INTEGER variables ':' formula	{ $$ = Insert::bexform('<',false,$3,*$4,$6,@1);
														  delete($4);										}
			| '~' formula								{ $$ = $2; $$->swapsign();							}
            | formula '&' formula						{ $$ = Insert::conjform($1,$3,@1);					}
            | formula '|' formula						{ $$ = Insert::disjform($1,$3,@1);					}
            | formula "=>" formula						{ $$ = Insert::implform($1,$3,@1);					}
            | formula "<=" formula						{ $$ = Insert::revimplform($1,$3,@1);				}
            | formula "<=>" formula						{ $$ = Insert::equivform($1,$3,@1);					}
            | '(' formula ')'							{ $$ = $2;											}
			| TRUE										{ $$ = Insert::trueform(@1);						}
			| FALSE										{ $$ = Insert::falseform(@1);						}
			| eq_chain									{ $$ = $1;											}
            | predicate									{ $$ = $1;											}
            ;

predicate   : intern_pointer							{ $$ = Insert::predform($1,@1);					}
			| intern_pointer '(' ')'					{ $$ = Insert::predform($1,@1);					}
            | intern_pointer '(' term_tuple ')'			{ $$ = Insert::predform($1,*$3,@1); delete($3); }
            ;

eq_chain	: eq_chain '='  term	{ $$ = Insert::eqchain('=',true,$1,$3,@1);	}
			| eq_chain "~=" term	{ $$ = Insert::eqchain('=',false,$1,$3,@1);	}
			| eq_chain '<'  term	{ $$ = Insert::eqchain('<',true,$1,$3,@1);	}	
			| eq_chain '>'  term    { $$ = Insert::eqchain('>',true,$1,$3,@1);	}
			| eq_chain "=<" term	{ $$ = Insert::eqchain('>',false,$1,$3,@1);	}		
			| eq_chain ">=" term	{ $$ = Insert::eqchain('<',false,$1,$3,@1);	}		
			| term '='  term		{ $$ = Insert::eqchain('=',true,$1,$3,@1);	}
            | term "~=" term		{ $$ = Insert::eqchain('=',false,$1,$3,@1);	}
            | term '<'  term		{ $$ = Insert::eqchain('<',true,$1,$3,@1);	}
            | term '>'  term		{ $$ = Insert::eqchain('>',true,$1,$3,@1);	}
            | term "=<" term		{ $$ = Insert::eqchain('>',false,$1,$3,@1);	}
            | term ">=" term		{ $$ = Insert::eqchain('<',false,$1,$3,@1);	}
			;

variables   : variables variable	{ $$ = $1; $$->push_back($2);		}		
            | variable				{ $$ = new vector<Variable*>(1,$1);	}
			;

variable	: identifier							{ $$ = Insert::quantifiedvar(*$1,@1);		}
			| identifier '[' theosort_pointer ']'	{ $$ = Insert::quantifiedvar(*$1,$3,@1);	}
			;

theosort_pointer	:	pointer_name		{ $$ = Insert::theosortpointer(*$1,@1); delete($1);	}
					;

/** Terms **/                                            

term		: function		{ $$ = $1;	}		
			| arterm		{ $$ = $1;	}
			| domterm		{ $$ = $1;	}
			| aggterm		{ $$ = $1;	}
			;

function	: intern_pointer '(' term_tuple ')'		{ $$ = Insert::functerm($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = Insert::functerm($1);					}
			| intern_pointer						{ $$ = Insert::functerm($1);					}
			;

arterm		: term '-' term				{ $$ = Insert::arterm('-',$1,$3,@1);	}				
			| term '+' term				{ $$ = Insert::arterm('+',$1,$3,@1);	}
			| term '*' term				{ $$ = Insert::arterm('*',$1,$3,@1);	}
			| term '/' term				{ $$ = Insert::arterm('/',$1,$3,@1);	}
			| term '%' term				{ $$ = Insert::arterm('%',$1,$3,@1);	}
			| term '^' term				{ $$ = Insert::arterm('^',$1,$3,@1);	}
			| '-' term %prec UMINUS		{ $$ = Insert::arterm("-",$2,@1);		}
			| ABS '(' term ')'			{ $$ = Insert::arterm("abs",$3,@1);		}
			| '(' arterm ')'			{ $$ = $2;								}
			;

domterm		: INTEGER									{ $$ = Insert::domterm($1,@1);		}
			| FLNUMBER									{ $$ = Insert::domterm($1,@1);		}
			| STRINGCONS								{ $$ = Insert::domterm($1,@1);		}
			| CHARCONS									{ $$ = Insert::domterm($1,@1);		}
			| '@' identifier '[' theosort_pointer ']'	{ $$ = Insert::domterm($2,$4,@1);	}
			| '@' identifier							{ $$ = Insert::domterm($2,0,@1);	}
			;

aggterm		: CARD set							{ $$ = Insert::aggregate(AGGCARD,$2,@1);	}
			| SOM set							{ $$ = Insert::aggregate(AGGSUM,$2,@1);		}
			| PROD set							{ $$ = Insert::aggregate(AGGPROD,$2,@1);	}
			| MINAGG set						{ $$ = Insert::aggregate(AGGMIN,$2,@1);		}
			| MAXAGG set						{ $$ = Insert::aggregate(AGGMAX,$2,@1);		}
			;

set			: '{' variables ':' formula '}'		{ $$ = Insert::set(*$2,$4,@1); delete($2);	}	
			| '[' form_term_list ']'			{ $$ = Insert::set(*$2,@1); delete($2);		}
			| '[' form_list ']'					{ $$ = Insert::set(*$2,@1); delete($2);		}
			;


/**********************
	Input structure
**********************/

structure		: STRUCT_HEADER struct_name ':' vocab_pointer '{' interpretations '}'	{ Insert::closestructure();	}
				| STRUCT_HEADER struct_name '=' function_call							{ /* TODO */ 
																						  Insert::closestructure();	}
				;

struct_name		: identifier	{ Insert::openstructure(*$1,@1); }
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

empty_inter	: intern_pointer '=' '{' '}'				{ Insert::emptyinter($1); }
			;

/** Interpretations with arity 1 **/

sort_inter	: intern_pointer '=' '{' elements_es '}'		{ Insert::sortinter($1,$4);  }	
			;

elements_es		: elements ';'						{ $$ = $1;	}
				| elements							{ $$ = $1;	}
				;

elements		: elements ';' charrange			{ $$ = $1->add((*$3)[0],(*$3)[1]); delete($3);
													  if($$ != $1) delete($1);
													}
				| elements ';' intrange				{ $$ = $1->add((*$3)[0],(*$3)[1]); delete($3);	
													  if($$ != $1) delete($1);
													}
				| elements ';' '(' strelement ')'	{ $$ = $1->add($4);				
													  if($$ != $1) delete($1);
													}
				| elements ';' '(' integer ')'		{ $$ = $1->add($4);								
													  if($$ != $1) delete($1);
													}
				| elements ';' '(' compound ')'		{ $$ = $1->add($4);
													  if($$ != $1) delete($1);
													}
				| elements ';' '(' floatnr ')'		{ $$ = $1->add($4);
													  if($$ != $1) delete($1);
													}
				| elements ';' integer				{ $$ = $1->add($3);								
													  if($$ != $1) delete($1);
													}
				| elements ';' strelement			{ $$ = $1->add($3);
													  if($$ != $1) delete($1);
													}
				| elements ';' floatnr				{ $$ = $1->add($3); 
													  if($$ != $1) delete($1);
													}
				| elements ';' compound				{ $$ = $1->add($3);
													  if($$ != $1) delete($1);
													}
				| charrange							{ $$ = new StrSortTable(); $$ = $$->add((*$1)[0],(*$1)[1]); delete($1);	}
				| intrange							{ $$ = new RanSortTable((*$1)[0],(*$1)[1]); delete($1);					}
				| '(' strelement ')'				{ $$ = new StrSortTable(); $$ = $$->add($2);							}
				| '(' integer ')'					{ $$ = new IntSortTable(); $$ = $$->add($2);							}
				| '(' floatnr ')'                   { $$ = new FloatSortTable(); $$ = $$->add($2);							}
				| '(' compound ')'					{ $$ = new MixedSortTable(); $$ = $$->add($2);							}
				| strelement						{ $$ = new StrSortTable(); $$ = $$->add($1);							}
				| integer							{ $$ = new IntSortTable(); $$ = $$->add($1);							}
				| floatnr							{ $$ = new FloatSortTable(); $$ = $$->add($1);							}
				| compound							{ $$ = new MixedSortTable(); $$ = $$->add($1);							}
				;

strelement		: identifier	{ $$ = $1;	}
				| STRINGCONS	{ $$ = $1;	}
				| CHARCONS		{ $$ = IDPointer(string(1,$1));	}
				;

/** Interpretations with arity not 1 **/

pred_inter		: intern_pointer '=' '{' ptuples_es '}'		{ if(!_wrongarity) Insert::predinter($1,_currtable);
															  closeTable();
															}
				| intern_pointer '=' TRUE					{ Insert::truepredinter($1);		}
				| intern_pointer '=' FALSE					{ Insert::falsepredinter($1);	}
				;

ptuples_es		: ptuples ';'						
				| ptuples
				;

ptuples			: ptuples ';' '(' ptuple ')'	{ closeRow(@4);	}
				| ptuples ';' ptuple			{ closeRow(@3);	}
				| ptuples ';' emptyptuple		{ addEmpty(); closeRow(@3);	}
				| '(' ptuple ')'				{ closeRow(@2);	}
				| ptuple						{ closeRow(@1);	}
				| emptyptuple					{ addEmpty(); closeRow(@1);	}				
				;

emptyptuple		: '(' ')'										
				;

ptuple			: ptuple ',' pelement						
				| pelement ',' pelement	
				;

pelement		: integer		{ addInt($1,@1);											}
				| identifier	{ addString($1,@1);											}
				| CHARCONS		{ string* str = IDPointer(string(1,$1)); addString(str,@1);	}
				| STRINGCONS	{ addString($1,@1);											}
				| floatnr		{ addFloat($1,@1);											}
				| compound		{ addCompound($1,@1);										}
				;

/** Interpretations for functions **/

func_inter	: intern_pointer '=' '{' ftuples_es '}'	{ if(!_wrongarity) Insert::funcinter($1,_currtable);
													  closeTable();	
													}	
			| intern_pointer '=' pelement			{ if(!_wrongarity) Insert::funcinter($1,_currtable);
													  closeTable();
													}
			;

ftuples_es		: ftuples ';'
				| ftuples
				;

ftuples			: ftuples ';' ftuple			{ closeRow(@3);	}	
				| ftuple						{ closeRow(@1);	}	
				;

ftuple			: ptuple "->" pelement						
				| pelement "->" pelement
				| emptyptuple "->" pelement				
				| "->" pelement
				;

/** Procedural interpretations **/

proc_inter		: intern_pointer '=' PROCEDURE pointer_name		/* TODO */
				;

/** Three-valued interpretations **/

three_inter		: threepred_inter
				| threefunc_inter
				| threeempty_inter
				;

threeempty_inter	: intern_pointer '<' identifier '>' '=' '{' '}'			{ Insert::emptythreeinter($1,*$3); }
					;

threepred_inter : intern_pointer '<' identifier '>' '=' '{' ptuples_es '}'	{ if(!_wrongarity) 
																			    Insert::threepredinter($1,*$3,_currtable);
																			  closeTable(); 
																			}
				| intern_pointer '<' identifier '>' '=' '{' elements_es '}'	{ Insert::threeinter($1,*$3,$7); }
				| intern_pointer '<' identifier '>' '=' TRUE				{ Insert::truethreepredinter($1,*$3);	}
				| intern_pointer '<' identifier '>' '=' FALSE				{ Insert::falsethreepredinter($1,*$3);	}
				;

threefunc_inter	: intern_pointer '<' identifier '>' '=' '{' ftuples_es '}'	{ if(!_wrongarity) 
																				Insert::threefuncinter($1,*$3,_currtable);
																			  closeTable();	
																			}
				| intern_pointer '<' identifier '>' '=' pelement			{ if(!_wrongarity) 
																				Insert::threefuncinter($1,*$3,_currtable);
																			  closeTable();	
																			}
				;

/** Ranges **/

intrange	: integer ".." integer			{ $$ = new vector<int>(2,$1); 
											  if($1 > $3) { 
												  ParseInfo pi(@1.first_line,@1.first_column,Insert::currfile());
												  Error::invalidrange($1,$3,pi); 
											  }
											  else { (*$$)[1] = $3; }
											}
			;

charrange	: CHARACTER ".." CHARACTER		{ $$ = new vector<char>(2,$1); 
											  if($1 > $3) { 
												  ParseInfo pi(@1.first_line,@1.first_column,Insert::currfile());
												  Error::invalidrange($1,$3,pi); 
											  }
											  else { (*$$)[1] = $3; }
											}	
			| CHARCONS ".." CHARCONS		{ $$ = new vector<char>(2,$1); 
											  if($1 > $3) { 
												  ParseInfo pi(@1.first_line,@1.first_column,Insert::currfile());
												  Error::invalidrange($1,$3,pi); 
											  }
											  else { (*$$)[1] = $3; }
											}
			;

/** Compound elements **/

compound	: intern_pointer '(' compound_args ')'	{ $$ = Insert::makecompound($1,*$3); delete($3);	}
			| intern_pointer '(' ')'				{ $$ = Insert::makecompound($1);					}
			;

compound_args	: compound_args ',' floatnr		{ TypedElement* t = new TypedElement($3);	
												  $$->push_back(t);							}
				| compound_args ',' integer		{ TypedElement* t = new TypedElement($3);	
												  $$->push_back(t);							}
				| compound_args ',' strelement	{ TypedElement* t = new TypedElement($3);	
												  $$->push_back(t);							}
				| compound_args ',' compound	{ TypedElement* t = new TypedElement($3);	
												  $$->push_back(t);							}
				| floatnr						{ TypedElement* t = new TypedElement($1);
												  $$ = new vector<TypedElement*>(1,t);		}
				| integer						{ TypedElement* t = new TypedElement($1);
												  $$ = new vector<TypedElement*>(1,t);		}
				| strelement					{ TypedElement* t = new TypedElement($1);
												  $$ = new vector<TypedElement*>(1,t);		}
				| compound						{ TypedElement* t = new TypedElement($1);
												  $$ = new vector<TypedElement*>(1,t);		}
				;
	         
/** Terminals **/

integer			: INTEGER		{ $$ = $1;		}
				| '-' INTEGER	{ $$ = -$2;		}
				;

floatnr			: FLNUMBER			{ $$ = $1;		}
				| '-' FLNUMBER		{ $$ = -($2);	}
				;

identifier		: IDENTIFIER	{ $$ = $1;	}
				| CHARACTER		{ $$ = IDPointer(string(1,$1)); } 
				| VOCABULARY	{ $$ = IDPointer(string("vocabulary"));	}
				| NAMESPACE		{ $$ = IDPointer(string("namespace"));	}
				;

/********************
	ASP structure
********************/

asp_structure	: ASP_HEADER struct_name ':' vocab_pointer '{' atoms '}'	{ Insert::closeaspstructure();	}
				| ASP_BELIEF struct_name ':' vocab_pointer '{' atoms '}'	{ Insert::closeaspbelief();		}
				;

atoms	: /* empty */
		| atoms atom '.'
		| atoms using
		;

atom	: predatom
		| funcatom
		;

predatom	: intern_pointer '(' domain_tuple ')'	
					{ Insert::predatom($1,_currelements,_currranges,_currtypes,_iselement,true); closeTuple(); }
			| intern_pointer '(' ')'				
					{ Insert::predatom($1,_currelements,_currranges,_currtypes,_iselement,true);	}
			| intern_pointer						
					{ Insert::predatom($1,_currelements,_currranges,_currtypes,_iselement,true);	}
			| '-' intern_pointer '(' domain_tuple ')'	
					{ Insert::predatom($2,_currelements,_currranges,_currtypes,_iselement,false); closeTuple(); }
			| '-' intern_pointer '(' ')'				
					{ Insert::predatom($2,_currelements,_currranges,_currtypes,_iselement,false);	}
			| '-' intern_pointer						
					{ Insert::predatom($2,_currelements,_currranges,_currtypes,_iselement,false);	}
			; 

funcatom	: intern_pointer '(' domain_tuple ')' '=' domain_element	
					{ Insert::funcatom($1,_currelements,_currranges,_currtypes,_iselement,true); closeTuple(); }
			| intern_pointer '(' ')' '=' domain_element				
					{ Insert::funcatom($1,_currelements,_currranges,_currtypes,_iselement,true); closeTuple(); }
			| intern_pointer '=' domain_element						
					{ Insert::funcatom($1,_currelements,_currranges,_currtypes,_iselement,true); closeTuple(); }
			| '-' intern_pointer '(' domain_tuple ')' '=' domain_element	
					{ Insert::funcatom($2,_currelements,_currranges,_currtypes,_iselement,false); closeTuple(); }
			| '-' intern_pointer '(' ')' '=' domain_element				
					{ Insert::funcatom($2,_currelements,_currranges,_currtypes,_iselement,false); closeTuple(); }
			| '-' intern_pointer '=' domain_element						
					{ Insert::funcatom($2,_currelements,_currranges,_currtypes,_iselement,false); closeTuple(); }
			;

domain_tuple	: domain_tuple ',' domain_element
				| domain_tuple ',' domain_range
				| domain_element
				| domain_range	
				;

domain_range	: intrange		{ RanSortTable* rst = new RanSortTable((*$1)[0],(*$1)[1]);
								  _currranges.push_back(rst);
								  _iselement.push_back(false);
								  delete($1);
								}
				| charrange		{ StrSortTable* sst = new StrSortTable();
								  sst->add((*$1)[0],(*$1)[1]);
								  _currranges.push_back(sst);
								  _iselement.push_back(false);
								  delete($1);
								}
				;

domain_element	: strelement	{ Element e; e._string = $1;
								  _currelements.push_back(e);
								  _currtypes.push_back(ELSTRING);
								  _iselement.push_back(true);
								}
				| integer		{ Element e; e._int = $1; 
								  _currelements.push_back(e);
								  _currtypes.push_back(ELINT);
								  _iselement.push_back(true);
								}
				| floatnr		{ Element e; e._double = $1; 
								  _currelements.push_back(e);
								  _currtypes.push_back(ELDOUBLE);
								  _iselement.push_back(true);
								}
				| compound		{ Element e; e._compound = $1;
								  _currelements.push_back(e);
								  _currtypes.push_back(ELCOMPOUND);
								  _iselement.push_back(true);
								}
				;

/*********************
	Lists and Tuples
*********************/

/** Tuples **/

term_tuple		: term_tuple ',' term						{ $$ = $1; $$->push_back($3);		}	
				| term										{ $$ = new vector<Term*>(1,$1);		}	
				;

sort_pointer_tuple	: /* empty */							{ $$ = new vector<Sort*>(0);		}
					| nonempty_spt							{ $$ = $1;							}
					;
					
nonempty_spt		: sort_pointer_tuple ',' sort_pointer	{ $$ = $1; $$->push_back($3);		}
					| sort_pointer							{ $$ = new vector<Sort*>(1,$1);		}
					;

/** Lists **/

form_list		: form_list ';' formula						{ $$ = $1; $$->push_back($3);		}
				| formula									{ $$ = new vector<Formula*>(1,$1);	}		
				;

form_term_list	: form_term_list ';' form_term_tuple		{ $$ = $1; $$->push_back($3);		}
				| form_term_tuple							{ $$ = new vector<FTTuple*>(1,$1);	}
				;

form_term_tuple	: '(' formula ',' term ')'					{ $$ = Insert::fttuple($2,$4);		}
				;

/*******************
	Instructions
*******************/

instructions		: PROCEDURE_HEADER proc_name proc_sig '{' lua_block '}'		{ Insert::closeproc();	}
					;

proc_name			: identifier	{ Insert::openproc(*$1,@1);	}
					;

proc_sig			: '(' ')'		{ Insert::luacode(string(")"));	}
					| '(' args ')'	{ Insert::luacode(string(")"));	}
					;

lua_block			: /* empty */
					| lua_block identifier		{ Insert::luacode(*$2);	}
					| lua_block STRINGCONS		{ string str = string("\"") + *$2 + string("\""); Insert::luacode(str);	}
					| lua_block CHARCONS		{ string str = string("'") + $2 + string("'"); Insert::luacode(str);	}
					| lua_block INTEGER			{ Insert::luacode(itos($2));							}
					| lua_block FLNUMBER		{ Insert::luacode(dtos($2));							}
					| lua_block pointer_name "::" identifier	{ $2->push_back(*$4); Insert::luacode(*$2); delete($2);	}
					;

args				: args ',' identifier	{ Insert::procarg(*$3);		}
					| identifier			{ Insert::procarg(*$1);		}
					;

execstatement	: EXEC_HEADER openexec lua_block closeexec	
				;

openexec		: '{'											{ Insert::openexec();	}
				;

closeexec		: '}'
				;


/**************
	Options
**************/

options	: OPTION_HEADER option_name '{' optassigns '}'	{ Insert::closeoptions();	}
		;

option_name	: identifier	{ Insert::openoptions(*$1,@1);	}
			;

optassigns	: /* empty */
			| optassigns optassign	
			| optassigns EXTERN OPTIONS pointer_name	{ Insert::externoption(*$4,@4);	delete($4);	}
			;

optassign	: identifier '=' strelement		{ Insert::option(*$1,*$3,@1);	}
			| identifier '=' floatnr		{ Insert::option(*$1,$3,@1);	}
			| identifier '=' integer		{ Insert::option(*$1,$3,@1);	}
			;


%%

/** Parsing tables **/

void closeRow(YYLTYPE l) {
	++_currrow;
	if(_currcol != _currtable->arity()) {
		if(!_wrongarity) {
			ParseInfo pi(l.first_line,l.first_column,Insert::currfile());
			Error::wrongarity(pi);
			_wrongarity = true;
		}
	}
	_currcol = 0;
}

void closeTable() {
	_currtable = 0;
	_currrow = 0;
	_currcol = 0;
	_wrongarity = false;
}

void closeTuple() {
	_currelements.clear();
	_currelements.clear();
	_currtypes.clear();
	_iselement.clear();
}

void addEmpty() {
	if(!_currtable) _currtable = new FinitePredTable(vector<ElementType>(0));
	_currtable->addRow();
}

void addInt(int n, YYLTYPE l) {
	if(!_currtable) // we start parsing a new table
		_currtable = new FinitePredTable(vector<ElementType>(0)); 
	if(_currrow) {	// the table already contains at least one row
		if(!_currcol)	// start parsing a new row 
			_currtable->addRow();
		if(_currcol < _currtable->arity()) {	// OK, we are within the number of columns of the table
			switch(_currtable->type(_currcol)) {
				case ELINT:
					(*_currtable)[_currrow][_currcol]._int = n;
					break;
				case ELDOUBLE:
					(*_currtable)[_currrow][_currcol]._double = double(n);
					break;
				case ELSTRING:
					(*_currtable)[_currrow][_currcol]._string = IDPointer(itos(n));
					break;
				case ELCOMPOUND:
					(*_currtable)[_currrow][_currcol]._compound = CPPointer(TypedElement(n));
					break;
				default:
					assert(false);
			}
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo pi(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				_wrongarity = true;
			}
		}
	}
	else {	// we are parsing the first row of the table
		if(!_currcol) _currtable->addRow();
		_currtable->addColumn(ELINT);
		(*_currtable)[0][_currcol]._int = n;
	}
	_currcol++;
}

void addString(string* s, YYLTYPE l) {
	if(!_currtable) // we start parsing a new table
		_currtable = new FinitePredTable(vector<ElementType>(0)); 
	if(_currrow) {	// the table already contains at least one row
		if(!_currcol)	// start parsing a new row 
			_currtable->addRow();
		if(_currcol < _currtable->arity()) {	// OK, we are within the number of columns of the table
			switch(_currtable->type(_currcol)) {
				case ELINT:
				case ELDOUBLE:
					_currtable->changeElType(_currcol,ELSTRING);
					(*_currtable)[_currrow][_currcol]._string = s;
					break;
				case ELSTRING:
					(*_currtable)[_currrow][_currcol]._string = s;
					break;
				case ELCOMPOUND:
					(*_currtable)[_currrow][_currcol]._compound = CPPointer(TypedElement(s));
					break;
				default:
					assert(false);
			}
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo pi(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				_wrongarity = true;
			}
		}
	}
	else {	// we are parsing the first row of the table
		if(!_currcol) _currtable->addRow();
		_currtable->addColumn(ELSTRING);
		(*_currtable)[0][_currcol]._string = s;
	}
	_currcol++;
}

void addCompound(compound* s, YYLTYPE l) {
	if(!_currtable) // we start parsing a new table
		_currtable = new FinitePredTable(vector<ElementType>(0)); 
	if(_currrow) {	// the table already contains at least one row
		if(!_currcol)	// start parsing a new row 
			_currtable->addRow();
		if(_currcol < _currtable->arity()) {	// OK, we are within the number of columns of the table
			switch(_currtable->type(_currcol)) {
				case ELINT:
				case ELDOUBLE:
				case ELSTRING:
					_currtable->changeElType(_currcol,ELCOMPOUND);
					break;
				case ELCOMPOUND:
					break;
			}
			(*_currtable)[_currrow][_currcol]._compound = s;
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo pi(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				_wrongarity = true;
			}
		}
	}
	else {	// we are parsing the first row of the table
		if(!_currcol) _currtable->addRow();
		_currtable->addColumn(ELCOMPOUND);
		(*_currtable)[0][_currcol]._compound = s;
	}
	_currcol++;
}

void addFloat(double d, YYLTYPE l) {
	if(!_currtable) // we start parsing a new table
		_currtable = new FinitePredTable(vector<ElementType>(0)); 
	if(_currrow) {	// the table already contains at least one row
		if(!_currcol)	// start parsing a new row 
			_currtable->addRow();
		if(_currcol < _currtable->arity()) {	// OK, we are within the number of columns of the table
			switch(_currtable->type(_currcol)) {
				case ELINT:
					_currtable->changeElType(_currcol,ELDOUBLE);
					(*_currtable)[_currrow][_currcol]._double = d;
					break;
				case ELDOUBLE:
					(*_currtable)[_currrow][_currcol]._double = d;
					break;
				case ELSTRING:
					(*_currtable)[_currrow][_currcol]._string = IDPointer(dtos(d));
					break;
				case ELCOMPOUND:
					(*_currtable)[_currrow][_currcol]._compound = CPPointer(TypedElement(d));
					break;
				default:
					assert(false);
			}
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo pi(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				_wrongarity = true;
			}
		}
	}
	else {	// we are parsing the first row of the table
		if(!_currcol) _currtable->addRow();
		_currtable->addColumn(ELDOUBLE);
		(*_currtable)[0][_currcol]._double = d;
	}
	_currcol++;
}

/** Error messages **/

void yyerror(const char* s) {
	ParseInfo pi(yylloc.first_line,yylloc.first_column,Insert::currfile());
	Error::error(pi);
	cerr << s << endl;
}
