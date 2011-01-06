/************************************
	lex.ll
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

%{

#include <typeinfo>
#include "common.hpp"
#include "data.hpp"
#include "insert.hpp"
#include "error.hpp"
#include "parse.h"
#include "clconst.hpp"

extern YYSTYPE yylval;
extern YYLTYPE yylloc;

extern void setclconst(string,string);

// Return to right mode after a comment
int commentcaller;	
int	includecaller;

// Tab length
int tablen = 4;

// Bracket counter for closing lua blocks
int bracketcounter;

// substitute command line constants
map<string,CLConst*> clconsts;	

// Handle line and column numbers
//	call advanceline() each time a newline is read
//	call advancecol() each time a token is read
int prevlength = 0;
void advanceline() { 
	yylloc.first_line++;	
	yylloc.first_column = 1;	
	prevlength = 0;
}
void advancecol()	{ 
	yylloc.first_column += prevlength;
	prevlength = yyleng;						
}

// Scan from string
extern int yyparse();
void parsestring(const string& str) {
	YY_BUFFER_STATE buffer = yy_scan_string(str.c_str());
	yyparse();
	yy_delete_buffer(buffer);
}

// Handle includes
vector<YY_BUFFER_STATE>	include_buffer_stack;	// currently opened buffers
vector<string*>			include_buffer_files;	// currently opened files
vector<unsigned int>	include_line_stack;		// line number of the corresponding buffer
vector<unsigned int>	include_col_stack;		// column number of the corresponding buffer
bool					stdin_included = false;
void start_include(string s) {
	// check if we are including from the included file
	for(unsigned int n = 0; n < include_buffer_files.size(); ++n) {
		if(*(include_buffer_files[n]) == s) {
			ParseInfo pi(yylloc.first_line,yylloc.first_column,Insert::currfile());
			Error::cyclicinclude(s,pi);
			return;
		}
	}
	// store the current buffer
	include_buffer_stack.push_back(YY_CURRENT_BUFFER);
	include_buffer_files.push_back(Insert::currfile());
	include_line_stack.push_back(yylloc.first_line);
	include_col_stack.push_back(yylloc.first_column + prevlength);
	// open the new buffer
	FILE* oldfile = yyin;
	yyin = fopen(s.c_str(),"r");
	if(!yyin) {
		ParseInfo pi(yylloc.first_line,yylloc.first_column,Insert::currfile());
		Error::unexistingfile(s,pi);
		include_buffer_stack.pop_back();
		include_buffer_files.pop_back();
		include_line_stack.pop_back();
		include_col_stack.pop_back();
		yyin = oldfile;
	}
	else {
		yylloc.first_line = 1;
		yylloc.first_column = 1;
		prevlength = 0;
		Insert::currfile(s);
		yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
	}
}
void start_stdin_include() {
	if(stdin_included) {
		ParseInfo pi(yylloc.first_line,yylloc.first_column,Insert::currfile());
		Error::twicestdin(pi);
	}
	else {
		// store the current buffer
		include_buffer_stack.push_back(YY_CURRENT_BUFFER);
		include_buffer_files.push_back(Insert::currfile());
		include_line_stack.push_back(yylloc.first_line);
		include_col_stack.push_back(yylloc.first_column + prevlength);
		// open then new buffer
		Warning::readingfromstdin();
		stdin_included = true;
		yyin = stdin;
		Insert::currfile(0);
		yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
	}
}
void end_include() {
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(include_buffer_stack.back());
	yylloc.first_line = include_line_stack.back();
	yylloc.first_column = include_col_stack.back();
	prevlength = 0;
	Insert::currfile(include_buffer_files.back());
	include_buffer_stack.pop_back();
	include_buffer_files.pop_back();
	include_line_stack.pop_back();
	include_col_stack.pop_back();
}


%}

%option noyywrap

%x comment
%x vocabulary
%x structure
%x theory 
%x option
%x include
%x procedure
%x lua

ID				_*[A-Za-z][a-zA-Z0-9_]*	
CH				[A-Za-z]
INT				[0-9]+
FL				[0-9]*"."[0-9]+
STR				\"[^\"]*\"
CHR				\'.\'
WHITESPACE		[\r ]*
COMMENTLINE		"//".*

%%


	/*************** 
		Comments  
	***************/

<*>{COMMENTLINE}			{							}
<*>"/*"						{ commentcaller = YY_START;
							  BEGIN(comment);	
							  advancecol();				}
<comment>[^*\n]*			{ advancecol();				}
<comment>[^*\n]*\n			{ advanceline();			}
<comment>"*"+[^*/\n]*		{ advancecol();				}
<comment>"*"+[^*/\n]*\n		{ advanceline();			}
<comment>"*"+"/"			{ BEGIN(commentcaller);           
							  advancecol();				}

	/*************
		Include
	*************/

<*>"#include"				{ advancecol();
							  includecaller = YY_START;
							  BEGIN(include);
							}

	/**********
		Lua
	**********/

<procedure>"{"				{ advancecol();
							  BEGIN(lua);
							  bracketcounter = 0;		
							  return *yytext;
							}
<lua>"{"					{ advancecol(); 
							  ++bracketcounter;			
							  yylval.chr = *yytext;
							  return CHARACTER;
							}
<lua>"}"					{ advancecol();
							  --bracketcounter;
							  if(bracketcounter < 1) {
								BEGIN(INITIAL);
								return *yytext;
							  }
							  else {
							    yylval.chr = *yytext;
							    return CHARACTER;
							  }
							}
<lua>"#"{ID}				{ advancecol();
							  yylval.str = IDPointer(yytext);
							  return IDENTIFIER;		
							}
<lua>{WHITESPACE}           { advancecol();				
							  yylval.str = IDPointer(yytext);
							  return IDENTIFIER;		
							}
<lua>"\t"					{ advancecol(); 
							  yylval.chr = *yytext;
							  return CHARACTER;
							}
<lua>.                      { advancecol();
							  yylval.chr = *yytext;
							  return CHARACTER;
							}
<lua>\n                     { advanceline();			
							  yylval.chr = *yytext;
							  return CHARACTER;
							}

	/***************
		Headers 
	***************/

<*>"#vocabulary"			{ BEGIN(vocabulary);
							  advancecol();
							  return VOCAB_HEADER;		}
<*>"#theory"				{ BEGIN(theory);
							  advancecol();
							  return THEORY_HEADER;		}
<*>"#structure"				{ BEGIN(structure);
							  advancecol();
							  return STRUCT_HEADER;		}
<*>"#asp_structure"			{ BEGIN(structure);
							  advancecol();
							  return ASP_HEADER;		}
<*>"#asp_belief"			{ BEGIN(structure);
							  advancecol();
							  return ASP_BELIEF;		}
<*>"#namespace"				{ BEGIN(INITIAL);
							  advancecol();
							  return NAMESPACE_HEADER;	}
<*>"#procedure"				{ BEGIN(procedure);
							  advancecol();
							  return PROCEDURE_HEADER;	}
<*>"#options"				{ BEGIN(option);
							  advancecol();
							  return OPTION_HEADER;
							}

	/**************
		Include
	**************/

<include>"stdin"			{ advancecol();
							  start_stdin_include();
							  BEGIN(includecaller);
							}
<include>"$"[a-zA-Z0-9_]*	{ advancecol();
							  string temp(yytext);
							  temp = temp.substr(1,temp.size()-1);
							  if(clconsts.find(temp) != clconsts.end()) {
								  CLConst* clc = clconsts[temp];
								  if(typeid(*clc) == typeid(StrClConst)) {
									  StrClConst* slc = dynamic_cast<StrClConst*>(clc);
									  start_include(slc->value());
								  }
								  else {
									  ParseInfo pi(yylloc.first_line,yylloc.first_column,Insert::currfile());
									  Error::stringconsexp(temp,pi);
								  }
								  BEGIN(includecaller);
							  }
							  else {
								  cerr << "Type a value for constant " << temp << endl << "> "; 
								  string str;
								  getline(cin,str);
								  start_include(str);
								  BEGIN(includecaller);
							  }
							}
<include>{STR}				{ advancecol();
							  char* temp = yytext; ++temp;
							  string str(temp,yyleng-2);
							  start_include(str);	
							  BEGIN(includecaller);
							}

	/****************
		Vocabulary
	****************/

	/** Keywords **/

<vocabulary>"type"              { advancecol();
								  return TYPE;				}
<vocabulary>"partial"			{ advancecol();
								  return PARTIAL;			}
<vocabulary>"constructor"		{ advancecol();
								  return CONSTR;			}
<vocabulary>"isa"				{ advancecol();
								  return ISA;				}
<vocabulary>"extends"			{ advancecol();
								  return EXTENDS;			}
<vocabulary>"extern"			{ advancecol();
								  return EXTERN;			}
<option>"extern"				{ advancecol();
								  return EXTERN;			}
<option>"options"				{ advancecol();	
								  return OPTIONS;			}

	/*************
		Theory
	*************/


	/** Aggregates **/

<theory>"card"				{ advancecol();
							  return CARD;				}
<theory>"#"					{ advancecol();
							  return CARD;				}
<theory>"sum"				{ advancecol();
							  return SOM;				}
<theory>"prod"				{ advancecol();
							  return PROD;				}
<theory>"min"				{ advancecol();
							  return MINAGG;			}
<theory>"max"				{ advancecol();
							  return MAXAGG;			}

	/** Fixpoint definitions **/

<theory>"LFD"				{ advancecol(); 
							  return LFD;				}
<theory>"GFD"				{ advancecol();
							  return GFD;				}

	/** Arrows **/

<theory>"=>"                { advancecol();
							  return IMPL;				}
<theory>"<="				{ advancecol();
							  return RIMPL;				}
<theory>"<=>"				{ advancecol();
							  return EQUIV;				}
<theory>"<-"				{ advancecol();
							  return DEFIMP;			}
	/** True and false **/

<theory>"true"				{ advancecol();
							  return TRUE;				}
<theory>"false"				{ advancecol();
							  return FALSE;				}

	/** Comparison **/

<theory>"=<"                { advancecol();
						   	  return LEQ;				}
<theory>">="                { advancecol();
						   	  return GEQ;				}
<theory>"~="                { advancecol();
							  return NEQ;				}
	/** Ranges **/

<theory>".."				{ advancecol();
							  return RANGE;				}


	/****************
		Structure 
	****************/

<structure>"->"				{ advancecol();
							  return MAPS;				}
<structure>".."				{ advancecol();
							  return RANGE;				}
<structure>"true"			{ advancecol();
							  return TRUE;				}
<structure>"false"			{ advancecol();
							  return FALSE;				}
<structure>"procedure"		{ advancecol();
							  return PROCEDURE;			}


	/******************
		Identifiers
	******************/

<*>"using"					{ advancecol();
							  return USING;				}
<*>"vocabulary"				{ advancecol();
							  return VOCABULARY;		}
<*>"namespace"				{ advancecol();
							  return NAMESPACE;			}
<*>{CH}						{ advancecol();
							  yylval.chr = *yytext;
							  return CHARACTER;			}
<*>{ID}						{ advancecol();
							  yylval.str = IDPointer(yytext);
							  return IDENTIFIER;		}
<*>{STR}					{ advancecol();
							  char* temp = yytext; ++temp;
							  yylval.str = IDPointer(string(temp,yyleng-2));
							  return STRINGCONS;		}
<*>{INT}					{ advancecol();
							  yylval.nmr = atoi(yytext);
							  return INTEGER;		    }
<*>{FL}						{ advancecol();
							  yylval.dou = atof(yytext);
							  return FLNUMBER;			}
<*>{CHR}					{ advancecol();
							  yylval.chr = (yytext)[1];
							  return CHARCONS;
							}
<*>"$"[a-zA-Z0-9_]*			{ advancecol();
							  string temp(yytext);
							  temp = temp.substr(1,temp.size()-1);
							  if(clconsts.find(temp) != clconsts.end()) {
								  return (clconsts[temp])->execute();
							  }
							  else {
								  cerr << "Type a value for constant " << temp << endl << "> "; 
								  string str;
								  getline(cin,str);
								  setclconst(temp,str);
								  return (clconsts[temp])->execute();
							  }
							}


	/*************************************
		Whitespaces, newlines and rest
	*************************************/

<*>{WHITESPACE}             { advancecol();				}
<*>"\t"						{ advancecol(); 
							  prevlength = tablen;		}
<*>"::"						{ advancecol();
							  return NSPACE;			}
<*>.                        { advancecol();
							  return *yytext;			}
<*>\n                       { advanceline();			}

<<EOF>>						{ if(!include_buffer_stack.empty())	
								  end_include();			
							  else yyterminate();
							}

%%

