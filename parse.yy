/************************************
	parse.yy	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

%{

#include <iostream>
#include "insert.h"
#include "error.h"
#include "builtin.h"

// Lexer
extern int yylex();

// Parsing tables efficiently
UserPredTable*		_currtable = 0;
unsigned int		_currrow(0);
unsigned int		_currcol(0);
bool				_wrongarity = false;
void				addEmpty();
void				addInt(int,YYLTYPE);
void				addFloat(double*,YYLTYPE);
void				addString(string*,YYLTYPE);
void				closeRow(YYLTYPE);
void				closeTable();

// Parsing ASP structures efficiently
vector<Element>			_currtuple;
vector<UserSortTable*>	_currexpanders;
vector<ElementType>		_currtypes;
void					closeTuple();

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
	double*				dou;
	string*				str;
	InfArgType			iat;

	Sort*				sor;
	Predicate*			pre;
	Function*			fun;
	Term*				ter;
	PredForm*			prf;
	Formula*			fom;
	EqChainForm*		eqc;
	Rule*				rul;
	UserSortTable*		sta;
	FixpDef*			fpd;
	Definition*			def;
	Variable*			var;
	FTTuple*			ftt;
	SetExpr*			set;

	vector<int>*		vint;
	vector<char>*		vcha;
	vector<string>*		vstr;
	vector<Sort*>*		vsor;
	vector<Variable*>*	vvar;
	vector<Term*>*		vter;
	vector<Formula*>*	vfom;
	vector<Rule*>*		vrul;
	vector<FTTuple*>*	vftt;

	vector<pair<Rule*,FixpDef*> >*	vprf;
}

/** Headers  **/
%token VOCAB_HEADER
%token THEORY_HEADER
%token STRUCT_HEADER
%token ASP_HEADER
%token NAMESPACE_HEADER
%token EXECUTE_HEADER

/** Keywords **/
%token STRINGSORT
%token FLOATSORT
%token CHARSORT
%token INTSORT
%token PARTIAL
%token OPTION
%token MINAGG
%token MAXAGG
%token FALSE
%token CARD
%token SUCC
%token TYPE
%token PROD
%token TRUE
%token ABS
%token ISA
%token MIN
%token MAX
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
%token <operator> MAPS		"->"
%token <operator> EQUIV		"<=>"
%token <operator> IMPL		"=>"
%token <operator> RIMPL		"<="
%token <operator> DEFIMP	"<-"
%token <operator> NEQ		"~="
%token <operator> LEQ		"=<"
%token <operator> GEQ		">="
%token <operator> RANGE		".."
%token <operator> NSPACE	"::"

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
%type <iat> outtype
%type <nmr> integer
%type <dou> floatnr
%type <str> strelement
%type <str>	identifier
%type <str> command_arg
%type <str> command_name
%type <var> variable
%type <sor> sort_pointer
%type <sor> theosort_pointer
%type <sor> basesort_pointer
%type <pre> pred_decl
%type <pre>	pred_pointer
%type <fun> func_decl
%type <fun> full_func_decl
%type <fun> func_pointer
%type <ter> term
%type <ter> domterm
%type <ter> function
%type <ter> funcvar
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

%type <vint> intrange
%type <vcha> charrange
%type <vter> term_tuple
%type <vvar> variables
%type <vfom> form_list
%type <vrul> rules
%type <vstr> pointer_name
%type <vsor> sort_pointer_tuple
%type <vprf> fd_rules
%type <vstr> command_args
%type <vftt> form_term_list

%%

/*********************
	Global structure
*********************/

idp		        : /* empty */
				| idp namespace 
				| idp vocabulary 
		        | idp theory
				| idp structure
				| idp asp_structure
				| idp instructions
		        ;
	
namespace		: NAMESPACE_HEADER namespace_name '{' idp '}'	{ Insert::closespace();	}
				;

namespace_name	: identifier	{ Insert::openspace(*$1,@1); delete($1);	}
				;
 
/***************************
	Vocabulary declaration
***************************/

/** Structure of vocabulary declaration **/

vocabulary			: VOCAB_HEADER vocab_name '{' vocab_content '}'						{ Insert::closevocab();	}	
					| VOCAB_HEADER vocab_name ':' vocab_pointers '{' vocab_content '}'	{ Insert::closevocab();	}
					;

vocab_name			: identifier	{ Insert::openvocab(*$1,@1); delete($1);	}
					;

vocab_pointers		: vocab_pointers vocab_pointer
					| vocab_pointer
					;

vocab_pointer		: pointer_name	{ Insert::usingvocab(*$1,@1); delete($1); }
					;

vocab_content		: /* empty */
					| vocab_content symbol_declaration
					| vocab_content symbol_copy
					| vocab_content symbol_pointer
					| vocab_content error
					;

symbol_declaration	: sort_decl
					| pred_decl
					| func_decl
					;

symbol_pointer		: pred_pointer	{ Insert::predicate($1);	}
					| func_pointer	{ Insert::function($1);		}
					;


/** Symbol declarations **/

sort_decl		: TYPE identifier					{ Insert::sort(*$2,@2); delete($2);	}
				| TYPE identifier ISA sort_pointer	{ Insert::sort(*$2,$4,@2); delete($2); }
				;

pred_decl		: identifier '(' sort_pointer_tuple ')'	{ Insert::predicate(*$1,*$3,@1); delete($1); delete($3); }
				| identifier '(' ')'					{ Insert::predicate(*$1,@1); delete($1); }
				| identifier							{ Insert::predicate(*$1,@1); delete($1); }
				;

func_decl		: PARTIAL full_func_decl				{ $$ = $2; if($$) $$->partial(true);	}
				| full_func_decl						{ $$ = $1;								}
				;

full_func_decl	: identifier '(' sort_pointer_tuple ')' ':' sort_pointer	{ Insert::function(*$1,*$3,$6,@1);
																			  delete($1); delete($3); }
				| identifier '(' ')' ':' sort_pointer						{ Insert::function(*$1,$5,@1); 
																			  delete($1); }
				| identifier ':' sort_pointer								{ Insert::function(*$1,$3,@1); 
																			  delete($1); }	
				; 														

/** Symbol copy **/

symbol_copy		: TYPE identifier '=' sort_pointer	{ Insert::copysort(*$2,$4,@2); delete($2); }
				| identifier '=' pred_pointer		{ Insert::copypred(*$1,$3,@1); delete($1); }
				| identifier '=' func_pointer		{ Insert::copyfunc(*$1,$3,@1); delete($1); }
				;

/** Symbol pointers **/

sort_pointer		: pointer_name				{ $$ = Insert::sortpointer(*$1,@1); delete($1); }
					| basesort_pointer			{ $$ = $1;	}
					;

pred_pointer		: pointer_name '/' INTEGER	{ $1->back() = $1->back() + '/' + itos($3);
												  $$ = Insert::predpointer(*$1,@1); 
												  delete($1);
												}
					;

func_pointer		: pointer_name '/' INTEGER ':'	{ $1->back() = $1->back() + '/' + itos($3);
													  $$ = Insert::funcpointer(*$1,@1); 
													  delete($1);
													}
					;

basesort_pointer	: INTSORT		{ $$ = Builtin::intsort();		}			
					| FLOATSORT		{ $$ = Builtin::floatsort();	}
					| CHARSORT		{ $$ = Builtin::charsort();		}
					| STRINGSORT	{ $$ = Builtin::stringsort();	}	
					;

pointer_name		: pointer_name "::" identifier	{ $$ = $1; $$->push_back(*$3); delete($3);		}
					| identifier					{ $$ = new vector<string>(1,*$1); delete($1);	}
					;

/*************
	Theory	
*************/

theory		: THEORY_HEADER theory_name ':' vocab_pointer struct_pointer '{' def_forms '}'	{ Insert::closetheory();	}
			| THEORY_HEADER theory_name ':' vocab_pointer '{' def_forms '}'					{ Insert::closetheory();	}
			;

theory_name	: identifier	{ Insert::opentheory(*$1,@1); delete($1); }
			;

struct_pointer	: pointer_name	{ Insert::usingstruct(*$1,@1); delete($1); }
				;

def_forms	: /* empty */
			| def_forms definition		{ Insert::definition($2);	}
			| def_forms formula '.'		{ Insert::sentence($2);		}
			| def_forms fixpdef			{ Insert::fixpdef($2);		}
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

head		: predicate									{ $$ = $1;	}				
			| pointer_name '(' term_tuple ')' '=' term	{ $1->back() = $1->back() + '/' + itos($3->size());
														  $$ = Insert::funcgraphform(*$1,*$3,$6,@1);	}
			| pointer_name '(' ')' '=' term				{ $1->back() = $1->back() + "/0";
														  $$ = Insert::funcgraphform(*$1,$5,@1);		}
			| pointer_name '=' term						{ $1->back() = $1->back() + "/0";
														  $$ = Insert::funcgraphform(*$1,$3,@1);		}
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
			| SUCC '(' term ',' term ')'				{ $$ = Insert::succform($3,$5,@1);					}
			| TRUE										{ $$ = Insert::trueform(@1);						}
			| FALSE										{ $$ = Insert::falseform(@1);						}
			| eq_chain									{ $$ = $1;											}
            | predicate									{ $$ = $1;											}
            ;

predicate   : pointer_name								{ $1->back() = $1->back() + "/0";
														  $$ = Insert::predform(*$1,@1); delete($1); }
			| pointer_name '(' ')'						{ $1->back() = $1->back() + "/0";
														  $$ = Insert::predform(*$1,@1); delete($1); }
            | pointer_name '(' term_tuple ')'			{ $1->back() = $1->back() + '/' + itos($3->size());
														  $$ = Insert::predform(*$1,*$3,@1); delete($1); delete($3); }
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

variable	: identifier							{ $$ = Insert::quantifiedvar(*$1,@1); delete($1);		}
			| identifier '[' theosort_pointer ']'	{ $$ = Insert::quantifiedvar(*$1,$3,@1); delete($1);	}
			;

theosort_pointer	:	pointer_name		{ $$ = Insert::theosortpointer(*$1,@1); delete($1);	}
                    |   basesort_pointer	{ $$ = $1;	}
					;

/** Terms **/                                            

term		: function		{ $$ = $1;	}		
			| arterm		{ $$ = $1;	}
			| domterm		{ $$ = $1;	}
			| funcvar		{ $$ = $1;	}
			| aggterm		{ $$ = $1;	}
			;

function	: pointer_name '(' term_tuple ')'	{ $1->back() = $1->back() + '/' + itos($3->size());				
												  $$ = Insert::functerm(*$1,*$3,@1); delete($1); delete($3);	}
			| pointer_name '(' ')'				{ $1->back() = $1->back() + "/0";
												  $$ = Insert::functerm(*$1,@1); delete($1);	}
			| MIN								{ $$ = Insert::minterm(@1);		}
			| MAX								{ $$ = Insert::maxterm(@1);		}
			| MIN '[' theosort_pointer ']'		{ $$ = Insert::minterm($3,@1);	}
			| MAX '[' theosort_pointer ']'		{ $$ = Insert::maxterm($3,@1);	}
			;

funcvar		: pointer_name	{ $$ = Insert::funcvar(*$1,@1); delete($1);	}
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

domterm		: INTEGER								{ $$ = Insert::domterm($1,@1);		}
			| FLNUMBER								{ $$ = Insert::domterm($1,@1);		}
			| STRINGCONS							{ $$ = Insert::domterm($1,@1);		}
			| CHARCONS								{ $$ = Insert::domterm($1,@1);		}
			| identifier '[' theosort_pointer ']'	{ $$ = Insert::domterm($1,$3,@1);	}
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
				;

struct_name		: identifier	{ Insert::openstructure(*$1,@1); delete($1);	}
				;

interpretations	:  //empty
				| interpretations interpretation
				;

interpretation	: empty_inter
				| sort_inter
				| pred_inter
				| func_inter
				| three_inter
				| error
				;

/** Empty interpretations **/

empty_inter	: pointer_name '=' '{' '}'				{ Insert::emptyinter(*$1,@1); delete($1); }
			;

/** Interpretations with arity 1 **/

sort_inter	: pointer_name '=' '{' elements_es '}'	{ Insert::sortinter(*$1,$4,@1); delete($1); }	
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
				| elements ';' '(' strelement ')'	{ $$ = $$->add(*$4); delete($4);				
													  if($$ != $1) delete($1);
													}
				| elements ';' '(' integer ')'		{ $$ = $$->add($4);								
													  if($$ != $1) delete($1);
													}
				| elements ';' '(' floatnr ')'		{ $$ = $$->add(*$4); delete($4);				
													  if($$ != $1) delete($1);
													}
				| elements ';' integer				{ $$ = $$->add($3);								
													  if($$ != $1) delete($1);
													}
				| elements ';' strelement			{ $$ = $$->add(*$3); delete($3);				
													  if($$ != $1) delete($1);
													}
				| elements ';' floatnr				{ $$ = $$->add(*$3); delete($3);				
													  if($$ != $1) delete($1);
													}
				| charrange							{ $$ = new StrSortTable(); $$ = $$->add((*$1)[0],(*$1)[1]); delete($1);	}
				| intrange							{ $$ = new RanSortTable((*$1)[0],(*$1)[1]); delete($1);					}
				| '(' strelement ')'				{ $$ = new StrSortTable(); $$ = $$->add(*$2); delete($2);				}
				| '(' integer ')'					{ $$ = new IntSortTable(); $$ = $$->add($2);							}
				| '(' floatnr ')'                   { $$ = new FloatSortTable(); $$ = $$->add(*$2); delete($2);				}
				| strelement						{ $$ = new StrSortTable(); $$ = $$->add(*$1); delete($1);				}
				| integer							{ $$ = new IntSortTable(); $$ = $$->add($1);							}	
				| floatnr							{ $$ = new FloatSortTable(); $$ = $$->add(*$1); delete($1);				}
				;

strelement		: identifier	{ $$ = $1;	}
				| STRINGCONS	{ $$ = $1;	}
				| CHARCONS		{ $$ = new string(1,$1);	}
				;

/** Interpretations with arity not 1 **/

pred_inter		: pointer_name '=' '{' ptuples_es '}'	{ if(!_wrongarity) {
															  $1->back() = $1->back() + '/' + itos(_currtable->arity());
															  Insert::predinter(*$1,_currtable,@1); 
														  }
														  closeTable(); delete($1); 
														}
				| pointer_name '=' TRUE					{ $1->back() = $1->back() + "/0";
														  Insert::truepredinter(*$1,@1); delete($1);	}
				| pointer_name '=' FALSE				{ $1->back() = $1->back() + "/0";
														  Insert::falsepredinter(*$1,@1); delete($1);	}
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

pelement		: integer		{ addInt($1,@1); 										}
				| identifier	{ addString($1,@1);										}
				| CHARCONS		{ string* str = new string(1,$1); addString(str,@1);	}
				| STRINGCONS	{ addString($1,@1);										}
				| floatnr		{ addFloat($1,@1);										}
				;

/** Interpretations for functions **/

func_inter	: pointer_name '=' '{' ftuples_es '}'	{ if(!_wrongarity) {
														  $1->back() = $1->back() + '/' + itos(_currtable->arity()-1);
														  Insert::funcinter(*$1,_currtable,@1);
													  }
													  closeTable();	delete($1);
													}	
			| pointer_name '=' pelement				{ if(!_wrongarity) {
														  $1->back() = $1->back() + '/' + itos(_currtable->arity()-1);
														  Insert::funcinter(*$1,_currtable,@1);
													  }
													  closeTable();	delete($1);	
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

/** Three-valued interpretations **/

three_inter		: threepred_inter
				| threefunc_inter
				| threeempty_inter
				;

threeempty_inter	: pointer_name '[' identifier ']' '=' '{' '}'			{ Insert::emptythreeinter(*$1,*$3,@1);
																			  delete($1); delete($3);
																			}
					;

threepred_inter : pointer_name '[' identifier ']' '=' '{' ptuples_es '}'	{ if(!_wrongarity) {
																				  $1->back() = $1->back() + '/' + itos(_currtable->arity());
																				  Insert::threepredinter(*$1,*$3,_currtable,@1);
																			  }
																			  closeTable(); delete($1); delete($3);
																			}
				| pointer_name '[' identifier ']' '=' '{' elements_es '}'	{ Insert::threeinter(*$1,*$3,$7,@1); 
																			  delete($1); delete($3); 
																			}
				| pointer_name '[' identifier ']' '=' TRUE					{ $1->back() = $1->back() + "/0";
																			  Insert::truethreepredinter(*$1,*$3,@1);
																			}
				| pointer_name '[' identifier ']' '=' FALSE					{ $1->back() = $1->back() + "/0";
																			  Insert::falsethreepredinter(*$1,*$3,@1);
																			}
				;

threefunc_inter	: pointer_name '[' identifier ']' '=' '{' ftuples_es '}'	{ if(!_wrongarity) {
																				  $1->back() = $1->back()+'/'+ itos(_currtable->arity()-1);
																				  Insert::threefuncinter(*$1,*$3,_currtable,@1);
																			  }
																			  closeTable();	delete($1); delete($3);
																			}
				| pointer_name '[' identifier ']' '=' pelement				{ if(!_wrongarity) {
																				  $1->back() = $1->back()+'/'+ itos(_currtable->arity()-1);
																				  Insert::threefuncinter(*$1,*$3,_currtable,@1);
																			  }
																			  closeTable();	delete($1); delete($3);	
																			}
				;

/** Ranges **/

intrange	: integer ".." integer			{ $$ = new vector<int>(2,$1); 
											  if($1 > $3) { 
												  ParseInfo* pi = new ParseInfo(@1.first_line,@1.first_column,Insert::currfile());
												  Error::invalidrange($1,$3,pi); 
												  delete(pi);
											  }
											  else { (*$$)[1] = $3; }
											}
			;

charrange	: CHARACTER ".." CHARACTER		{ $$ = new vector<char>(2,$1); 
											  if($1 > $3) { 
												  ParseInfo* pi = new ParseInfo(@1.first_line,@1.first_column,Insert::currfile());
												  Error::invalidrange($1,$3,pi); 
												  delete(pi);
											  }
											  else { (*$$)[1] = $3; }
											}	
			| CHARCONS ".." CHARCONS		{ $$ = new vector<char>(2,$1); 
											  if($1 > $3) { 
												  ParseInfo* pi = new ParseInfo(@1.first_line,@1.first_column,Insert::currfile());
												  Error::invalidrange($1,$3,pi); 
												  delete(pi);
											  }
											  else { (*$$)[1] = $3; }
											}
			;
	         
/** Terminals **/

integer			: INTEGER		{ $$ = $1;		}
				| '-' INTEGER	{ $$ = -$2;		}
				;

floatnr			: FLNUMBER			{ $$ = $1;								}
				| '-' FLNUMBER		{ $$ = new double(-(*$2)); delete($2);	}
				;

identifier		: IDENTIFIER	{ $$ = $1;	}
				| CHARACTER		{ $$ = new string(1,$1); } 
				;

/********************
	ASP structure
********************/

asp_structure	: ASP_HEADER struct_name ':' vocab_pointer '{' atoms '}'	{ Insert::closeaspstructure();	}
				;

atoms	: /* empty */
		| atoms atom '.'
		;

atom	: predatom
		| funcatom
		;

predatom	: pointer_name '(' domain_tuple ')'	{ $1->back() = $1->back() + "/" + itos(_currtuple.size()); 
												  Insert::predatom(*$1,_currtypes,_currtuple,_currexpanders,@1); 
												  closeTuple(); delete($1);	
												}
			| pointer_name '(' ')'				{ $1->back() = $1->back() + "/0"; Insert::predatom(*$1,@1); delete($1);	}
			| pointer_name						{ $1->back() = $1->back() + "/0"; Insert::predatom(*$1,@1); delete($1);	}
			; 

funcatom	: pointer_name '(' domain_tuple ')' '=' domain_element	{ $1->back() = $1->back() + "/" + itos(_currtuple.size()-1); 
																	  Insert::funcatom(*$1,_currtypes,_currtuple,_currexpanders,@1); 
																	  closeTuple(); delete($1);	
																	}
			| pointer_name '(' ')' '=' domain_element				{ $1->back() = $1->back() + "/0"; 
																	  Insert::funcatom(*$1,_currtypes,_currtuple,_currexpanders,@1); 
																	  closeTuple(); delete($1);	
																	}
			| pointer_name '=' domain_element						{ $1->back() = $1->back() + "/0"; 
																	  Insert::funcatom(*$1,_currtypes,_currtuple,_currexpanders,@1); 
																	  closeTuple(); delete($1);	
																	}
			;

domain_tuple	: domain_tuple ',' domain_element
				| domain_tuple ',' domain_range
				| domain_element
				| domain_range	
				;

domain_range	: intrange		{ Element e; e._double = 0; _currtuple.push_back(e);
								  _currtypes.push_back(ELDOUBLE);
								  RanSortTable* rst = new RanSortTable((*$1)[0],(*$1)[1]);
								  _currexpanders.push_back(rst);
								  delete($1);
								}
				| charrange		{ Element e; e._string = 0; _currtuple.push_back(e);
								  _currtypes.push_back(ELSTRING);
								  StrSortTable* sst = new StrSortTable();
								  sst->add((*$1)[0],(*$1)[1]);
								  _currexpanders.push_back(sst);
								  delete($1);
								}
				;

domain_element	: strelement	{ Element e; e._string = $1;
								  _currtuple.push_back(e);
								  _currtypes.push_back(ELSTRING);
								}
				| integer		{ Element e; e._int = $1; 
								  _currtuple.push_back(e);
								  _currtypes.push_back(ELINT);
								}
				| floatnr		{ Element e; e._double = $1; 
								  _currtuple.push_back(e);	
								  _currtypes.push_back(ELDOUBLE);
								}
				;

/*********************
	Lists and Tuples
*********************/

/** Tuples **/

term_tuple		: term_tuple ',' term						{ $$ = $1; $$->push_back($3);		}	
				| term										{ $$ = new vector<Term*>(1,$1);		}	
				;

sort_pointer_tuple	: sort_pointer_tuple ',' sort_pointer	{ $$ = $1; $$->push_back($3);		}
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

instructions	: EXECUTE_HEADER '{' statements '}'
				;

statements		: /* empty */
				| statements statement
				;

statement		: option
				| command
				;

option			: OPTION identifier '=' identifier
				;

command			: void_command
				| nonvoid_command
				;
				
void_command	: command_name '(' command_args ')'	{ Insert::command(*$1,*$3,@1); delete($1); delete($3);	}
				| command_name '(' ')'				{ Insert::command(*$1,@1); delete($1);					}
				| command_name						{ Insert::command(*$1,@1); delete($1);					}
				;

nonvoid_command	: outtype identifier '=' command_name '(' command_args ')'	{ Insert::command($1,*$4,*$6,*$2,@1);
																			  delete($2); delete($4); delete($6);
																			}
				| outtype identifier '=' command_name '(' ')'				{ Insert::command($1,*$4,*$2,@1); 
																			  delete($2); delete($4);
																			}
				| outtype identifier '=' command_name						{ Insert::command($1,*$4,*$2,@1); 
																			  delete($2); delete($4);
																			}	
				;

outtype			: THEORY_HEADER		{ $$ = IAT_THEORY;		}
				| STRUCT_HEADER		{ $$ = IAT_STRUCTURE;	}
				| VOCAB_HEADER		{ $$ = IAT_VOCABULARY;	}
				;

command_name	: identifier						{ $$ = $1;	}
				;

command_args	: command_args ',' command_arg		{ $$ = $1; $$->push_back(*$3); delete($3);		}
				| command_arg						{ $$ = new vector<string>(1,*$1); delete($1);	}
				;

command_arg		: identifier						{ $$ = $1;	}
				;

%%

/** Parsing tables **/

void closeRow(YYLTYPE l) {
	++_currrow;
	if(_currcol != _currtable->arity()) {
		if(!_wrongarity) {
			ParseInfo* pi = new ParseInfo(l.first_line,l.first_column,Insert::currfile());
			Error::wrongarity(pi);
			delete(pi);
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
	_currtuple.clear();
	_currtypes.clear();
	_currexpanders.clear();
}

void addEmpty() {
	if(!_currtable) _currtable = new UserPredTable(vector<ElementType>(0));
	_currtable->addRow();
}

void addInt(int n, YYLTYPE l) {
	if(!_currtable) // we start parsing a new table
		_currtable = new UserPredTable(vector<ElementType>(0)); 
	if(_currrow) {	// the table already contains at least one row
		if(!_currcol)	// start parsing a new row 
			_currtable->addRow();
		if(_currcol < _currtable->arity()) {	// OK, we are within the number of columns of the table
			switch(_currtable->type(_currcol)) {
				case ELINT:
					(*_currtable)[_currrow][_currcol]._int = n;
					break;
				case ELDOUBLE:
					(*_currtable)[_currrow][_currcol]._double = new double(n);
					break;
				case ELSTRING:
					(*_currtable)[_currrow][_currcol]._string = new string(itos(n));
					break;
				default:
					assert(false);
			}
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo* pi = new ParseInfo(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				delete(pi);
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
		_currtable = new UserPredTable(vector<ElementType>(0)); 
	if(_currrow) {	// the table already contains at least one row
		if(!_currcol)	// start parsing a new row 
			_currtable->addRow();
		if(_currcol < _currtable->arity()) {	// OK, we are within the number of columns of the table
			switch(_currtable->type(_currcol)) {
				case ELINT:
				case ELDOUBLE:
					_currtable->changeElType(_currcol,ELSTRING);
					break;
				case ELSTRING:
					break;
				default:
					assert(false);
			}
			(*_currtable)[_currrow][_currcol]._string = s;
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo* pi = new ParseInfo(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				delete(pi);
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

void addFloat(double* d, YYLTYPE l) {
	if(!_currtable) // we start parsing a new table
		_currtable = new UserPredTable(vector<ElementType>(0)); 
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
					(*_currtable)[_currrow][_currcol]._string = new string(dtos(*d));
					break;
				default:
					assert(false);
			}
		}
		else {	// tuple with an incompatible arity
			if(!_wrongarity) {
				ParseInfo* pi = new ParseInfo(l.first_line,l.first_column,Insert::currfile());
				Error::wrongarity(pi);
				delete(pi);
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
	ParseInfo* pi = new ParseInfo(yylloc.first_line,yylloc.first_column,Insert::currfile());
	Error::error(pi);
	cerr << s << endl;
	delete(pi);
}
