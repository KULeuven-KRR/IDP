/**
*	The lexer tokenizes an IDP input string to make the parser's life is easier ;)
*/

%{

#include "parser/yyltype.hpp"

#include <string>
#include <iostream>
#include <climits>
#include <sstream>
#include <typeinfo>
#include "insert.hpp"
#include "parser.hh"
#include "errorhandling/error.hpp"
#include "common.hpp"
#include "GlobalData.hpp" 
using namespace std;

extern YYSTYPE yylval;
extern YYLTYPE yylloc;
extern int yyparse();
extern std::string state;
extern std::string getInstalledFilePath(std::string filename);

Insert& data();

extern void setclconst(string,string);

struct ParserData{

	ParserData(): tablen(1), bracketcounter(0), prevlength(0), stdin_included(false){
	}
	
	// Return to right mode after a comment
	int commentcaller;	
	int	includecaller;

	// Tab length
	int tablen;

	// Bracket counter for closing blocks
	int bracketcounter;

	// Copy lua code
	stringstream* luacode;

	// Handle line and column numbers
	//	call advanceline() each time a newline is read
	//	call advancecol() each time a token is read
	int prevlength;
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
	void parsestring(const string& str) {
		YY_BUFFER_STATE old = YY_CURRENT_BUFFER;
		YY_BUFFER_STATE buffer = yy_scan_string(str.c_str());
		yyparse();
		yy_delete_buffer(buffer);
		yy_switch_to_buffer(old);
	}

	// Handle includes
	vector<YY_BUFFER_STATE>	include_buffer_stack;		// currently opened buffers
	vector<string*>			include_buffer_filenames;	// currently opened files
	vector<unsigned int>	include_line_stack;			// line number of the corresponding buffer
	vector<unsigned int>	include_col_stack;			// column number of the corresponding buffer
	bool					stdin_included;
	set<string>				earlierincludes;			// Files that have already been included
	vector<string>			earlierstates;
	void start_include(string s) {
		// check if we are including from the included file
		for(unsigned int n = 0; n < include_buffer_filenames.size(); ++n) {
			auto filen = include_buffer_filenames[n];
			if( filen != NULL && *filen == s) {
				return;
			}
		}
		// Check if we already included it
		if(earlierincludes.find(s)!=earlierincludes.cend()){
			return;
		}
		earlierincludes.insert(s);
		
		// store the current buffer
		include_buffer_stack.push_back(YY_CURRENT_BUFFER);
		include_buffer_filenames.push_back(data().currfile());
		include_line_stack.push_back(yylloc.first_line);
		include_col_stack.push_back(yylloc.first_column + prevlength);
		// open the new buffer
		FILE* oldfile = yyin;
		
		if(earlierstates.size()==0){
			earlierstates.push_back(state);
		}
		std::string temp(earlierstates.back()+s);
//		std::cerr <<"Opening LEX" <<temp <<"\n";
		auto filep = fopen(temp.c_str(),"r");
		auto origstate = state;
		if(filep){
			auto index = temp.find_last_of('/');
			if(index==std::string::npos){
				state = "";
			}else{
				state = temp.substr(0, index+1);
			}
//			std::cerr <<"State set to " <<state <<"\n";
			earlierstates.push_back(state);
		}
		
		yyin = filep;
		if(yyin==NULL){
			yyin = fopen(s.c_str(),"r");
			earlierstates.push_back("");
		}
		if(!yyin) {
			ParseInfo pi(yylloc.first_line,yylloc.first_column,data().currfile());
			Error::unexistingfile(s,pi);
			include_buffer_stack.pop_back();
			include_buffer_filenames.pop_back();
			include_line_stack.pop_back();
			include_col_stack.pop_back();
			yyin = oldfile;
			state = origstate;
		}
		else {
			yylloc.first_line = 1;
			yylloc.first_column = 1;
			yylloc.descr = 0;
			prevlength = 0;
			data().currfile(s);
			yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
		}
	}
	void start_stdin_include() {
		if(stdin_included) {
			ParseInfo pi(yylloc.first_line,yylloc.first_column,data().currfile());
			Error::twicestdin(pi);
		}
		else {
			// store the current buffer
			include_buffer_stack.push_back(YY_CURRENT_BUFFER);
			include_buffer_filenames.push_back(data().currfile());
			include_line_stack.push_back(yylloc.first_line);
			include_col_stack.push_back(yylloc.first_column + prevlength);
			// open then new buffer
			Warning::readingfromstdin();
			stdin_included = true;
			yyin = stdin;
			data().currfile(0);
			yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
		}
	}
	void end_include() {
		if(yyin != stdin){
			fclose(yyin);
			yyin = NULL;
		}
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(include_buffer_stack.back());
		yylloc.first_line = include_line_stack.back();
		yylloc.first_column = include_col_stack.back();
		prevlength = 0;
		data().currfile(include_buffer_filenames.back());
		include_buffer_stack.pop_back();
		include_buffer_filenames.pop_back();
		include_line_stack.pop_back();
		include_col_stack.pop_back();
		earlierstates.pop_back();
	}
};

ParserData parser;

void resetParser(){
	parser = ParserData();
	BEGIN(0);
}
bool alreadyParsed(const std::string& filename){
	return parser.earlierincludes.find(filename)!=parser.earlierincludes.cend();
}


%}

%option noyywrap
%option noinput
%option nounput
%option never-interactive

%x comment
%x onelinecomment
%x vocabulary
%x constructed
%x structure
%x aspstructure
%x theory
%x include
%x procedure
%x lua
%x description
%x descontent
%x spacename

ID				_*[A-Za-z][a-zA-Z0-9_\']*	
CH				[A-Za-z]
	// TODO More logical name for CH and CHR? (letter en character)
INT				[0-9]+
FL				[0-9]*"."[0-9]+
STR				\"[^\"]*\"
CHR				\'.\'
WHITESPACE		[\r ]*
COMMENTLINE		"//"|"--"

%%


	/*************** 
		Comments  
	***************/


	/* When encountering nested comment starts, ignore them (removes the need for bookkeeping lexer states with a stack)*/
<comment>"/*"				{ parser.advancecol(); }
<description>"/*"			{ parser.advancecol(); }
<comment>"/**"/[^*]			{ parser.advancecol(); }
<description>"/**"/[^*]		{ parser.advancecol(); }

<*>"/**"/[^*]				{ parser.commentcaller = YY_START;
							  BEGIN(description);
							  parser.advancecol();
							  yylloc.descr = new stringstream();
							}
<description>[^*\n \r\t]*	{ parser.advancecol(); (*yylloc.descr) << yytext; BEGIN(descontent);	}
<description>[*]*			{ parser.advancecol();													}
<description>"*"+"/"		{ BEGIN(parser.commentcaller);           
							  parser.advancecol();				}
<descontent>[^*\n]*			{ parser.advancecol(); (*yylloc.descr) << yytext;			}
<descontent>[^*\n]*\n		{ parser.advanceline(); (*yylloc.descr) << yytext << "        "; BEGIN(description);		}
<descontent>"*"+[^*/\n]*	{ parser.advancecol();	(*yylloc.descr) << yytext;			}
<descontent>"*"+[^*/\n]*\n	{ parser.advanceline(); (*yylloc.descr) << yytext << "        ";	BEGIN(description);		}
<descontent>"*"+"/"			{ BEGIN(parser.commentcaller);           
							  parser.advancecol();				}

<*>"/*"						{ parser.commentcaller = YY_START;
							  BEGIN(comment);
							  parser.advancecol();				}
<comment>[^*\n]*			{ parser.advancecol();				}
<comment>[^*\n]*\n			{ parser.advanceline();			}
<comment>"*"+[^*/\n]*		{ parser.advancecol();				}
<comment>"*"+[^*/\n]*\n		{ parser.advanceline();			}
<comment>"*"+"/"			{ BEGIN(parser.commentcaller);           
							  parser.advancecol();				}


<*>{COMMENTLINE}			{ parser.commentcaller = YY_START; BEGIN(onelinecomment);	}
<onelinecomment>.*			{ BEGIN(parser.commentcaller); }
	/*************
		Include
	*************/

<*>"include"				{ parser.advancecol();
							  parser.includecaller = YY_START;
							  BEGIN(include);
							}

	/**********
		Lua
	**********/

<procedure>"..."			{ parser.advancecol();
							  return LUAVARARG;	}
<procedure>"{"				{ parser.advancecol();
							  BEGIN(lua);
							  ++parser.bracketcounter;		
							  parser.luacode = new stringstream();
							  return *yytext;
							}
							
	/* NOTE: important to have these before matches which work on lua syntax */
<lua>{STR}				{ parser.advancecol(); (*parser.luacode) << yytext;	}
<lua>{CHR}				{ parser.advancecol(); (*parser.luacode) << yytext;	}

<lua>"::"				{ parser.advancecol(); (*parser.luacode) << '.'; }
<lua>"{"				{ parser.advancecol();
							 ++parser.bracketcounter;
							 (*parser.luacode) << '{';
						}
<lua>"}"				{ parser.advancecol();
						  --parser.bracketcounter;
				 		 if(parser.bracketcounter == 0) { 
							  yylval.sstr = parser.luacode;
							  BEGIN(INITIAL); 
							  delete(yylloc.descr);
							  yylloc.descr = 0;
							  return LUACHUNK;	
						  }
						  else (*parser.luacode) << '}';
						}
<lua>\n					{ parser.advanceline(); (*parser.luacode) << '\n';	}
<lua>[^/{}:\n\"\'*]* 	{
							// \', \", //, :, { and } and \n are matched before (single / FALLS THROUGH to last one (.) )
							parser.advancecol();	
							(*parser.luacode) << yytext;
						}
<lua>.					{ parser.advancecol(); (*parser.luacode) << *yytext;	}


	/***************
		Headers 
	***************/

"vocabulary"			{ BEGIN(vocabulary);
						  parser.advancecol();
						  return VOCAB_HEADER;		}
"LTCvocabulary"			{ BEGIN(vocabulary);
						  parser.advancecol();
						  return LTC_VOCAB_HEADER;		}
"theory"				{ BEGIN(theory);
						  parser.advancecol();
						  return THEORY_HEADER;		}
"structure"				{ BEGIN(structure);
						  parser.advancecol();
						  return STRUCT_HEADER;		}
"factlist"				{ BEGIN(aspstructure);
						  parser.advancecol();
						  return ASP_HEADER;		}
"namespace"				{ BEGIN(spacename);
						  parser.advancecol();
						  return NAMESPACE_HEADER;	}
"procedure"				{ BEGIN(procedure);
						  parser.advancecol();
						  return PROCEDURE_HEADER;	}
"query"					{ BEGIN(theory);
						  parser.advancecol();
						  return QUERY_HEADER;
						}
"term"					{ BEGIN(theory);
						  parser.advancecol();
						  return TERM_HEADER;
						}
"Vocabulary"			{ BEGIN(vocabulary);
						  parser.advancecol();
						  return VOCAB_HEADER;		}
"Theory"				{ BEGIN(theory);
						  parser.advancecol();
						  return THEORY_HEADER;		}
"Structure"				{ BEGIN(structure);
						  parser.advancecol();
						  return STRUCT_HEADER;		}
"Aspstructure"			{ BEGIN(aspstructure);
						  parser.advancecol();
						  return ASP_HEADER;		}
"Namespace"				{ BEGIN(spacename);
						  parser.advancecol();
						  return NAMESPACE_HEADER;	}
"Procedure"				{ BEGIN(procedure);
						  parser.advancecol();
						  return PROCEDURE_HEADER;	}
"Query"					{ BEGIN(theory);
						  parser.advancecol();
						  return QUERY_HEADER;
						}
"fobdd"					{ BEGIN(theory);
						  parser.advancecol();
						  return FOBDD_HEADER;
						}
"Term"					{ BEGIN(theory);
						  parser.advancecol();
						  return TERM_HEADER;
						}

	/**************
		Include
	**************/

<include>"stdin"				{ parser.advancecol();
								  parser.start_stdin_include();
								  BEGIN(parser.includecaller);
								}
<include>"<"[a-zA-Z0-9_/]*">"	{ parser.advancecol();
								  char* temp = yytext; ++temp;								  
								  string str = getInstalledFilePath(string(temp,yyleng-2));
								  parser.start_include(str);	
								  BEGIN(parser.includecaller);
								}
<include>{STR}					{ parser.advancecol();
								  char* temp = yytext; ++temp;
								  string str(temp,yyleng-2);
								  parser.start_include(str);	
								  BEGIN(parser.includecaller);
								}

	/****************
		Vocabulary
	****************/

	/** Keywords **/

<vocabulary>"type"              { parser.advancecol();
								  return TYPE;				}
<vocabulary>"partial"			{ parser.advancecol();
								  return PARTIAL;			}
<vocabulary>"isa"				{ parser.advancecol();
								  return ISA;				}
<vocabulary>"contains"			{ parser.advancecol();
								  return EXTENDS;			}
<vocabulary>"extern"			{ parser.advancecol();
								  return EXTERN;			}
<vocabulary>"extern vocabulary"	{ parser.advancecol();
								  return EXTERNVOCABULARY;	}
<vocabulary>\n                  { parser.advanceline(); 
									return NEWLINE; 		}
<vocabulary>"constructed from"	{ parser.advancecol();
								  BEGIN(constructed);
								  return CONSTRUCTED;
								}
<vocabulary>".."				{ parser.advancecol();
								  return RANGE;				}
<constructed>\n					{ parser.advanceline(); }		
<constructed>"}"				{ parser.advancecol();
								  --parser.bracketcounter;
								  BEGIN(vocabulary);
								  return *yytext;}								

	/*************
		Theory
	*************/


	/** Aggregates **/

<theory>"card"				{ parser.advancecol();
							  return P_CARD;				}
<theory>"#"					{ parser.advancecol();
							  return P_CARD;				}
<theory>"sum"				{ parser.advancecol();
							  return P_SOM;				}
<theory>"prod"				{ parser.advancecol();
							  return P_PROD;				}
<theory>"min"				{ parser.advancecol();
							  return P_MINAGG;			}
<theory>"max"				{ parser.advancecol();
							  return P_MAXAGG;			}

	/** Fixpoint definitions **/

<theory>"LFD"				{ parser.advancecol(); 
							  return LFD;				}
<theory>"GFD"				{ parser.advancecol();
							  return GFD;				}

	/** Arrows **/

<theory>"=>"                { parser.advancecol();
							  return P_IMPL;				}
<theory>"<="				{ parser.advancecol();
							  return P_RIMPL;				}
<theory>"<=>"				{ parser.advancecol();
							  return EQUIV;				}
<theory>"<-"				{ parser.advancecol();
							  return DEFIMP;			}
	/** True and false **/

<theory>"true"				{ parser.advancecol();
							  return TRUE;				}
<theory>"false"				{ parser.advancecol();
							  return FALSE;				}

	/** Comparison **/

<theory>"=<"                { parser.advancecol();
						   	  return P_LEQ;				}
<theory>">="                { parser.advancecol();
						   	  return P_GEQ;				}
<theory>"~="                { parser.advancecol();
							  return P_NEQ;				}
	/** Ranges **/

<theory>".."				{ parser.advancecol();
							  return RANGE;				}
							  
<theory>"define"			{parser.advancecol();
						  		return DEF_HEADER;		}
<theory>"Define"			{parser.advancecol();
						  		return DEF_HEADER;		}
	/** Binary Quantifications **/
<theory>"in"				{parser.advancecol();
								return IN;				}
<theory>"sat"				{parser.advancecol();
								return SAT;				}

	/** Fobdds **/
<theory>"FALSE BRANCH:"	{ parser.advancecol();
							  return FALSEBRANCH;		}					

<theory>"TRUE BRANCH:"		{ parser.advancecol();
							  return TRUEBRANCH;		}
							  
<theory>"EXISTS:"			{ parser.advancecol();
							  return EXISTS;		}						
							  
							  								


	/****************
		Structure 
	****************/

<structure>"->"				{ parser.advancecol();
							  return MAPS;				}
<structure>".."				{ parser.advancecol();
							  return RANGE;				}
<structure>"true"			{ parser.advancecol();
							  return TRUE;				}
<structure>"false"			{ parser.advancecol();
							  return FALSE;				}
<structure>"procedure"		{ parser.advancecol();
							  return PROCEDURE;			}
<structure>"generate"		{ parser.advancecol();
							  return CONSTRUCTOR;		}
<aspstructure>"%".*			{							}
<aspstructure>".."			{ parser.advancecol();
							  return RANGE;				}


	/******************
		Identifiers
	******************/

<*>"using vocabulary"		{ parser.advancecol();
							  return USINGVOCABULARY;				
							}
<*>"using namespace"		{ parser.advancecol();
							  return USINGNAMESPACE;			
							}
<*>"using"					{
								Error::error("Can only use the keyword \"using\" as \"using vocabulary\" or \"using namespace\".\n");
								yyterminate();
							}
<*>{CH}						{ parser.advancecol();
							  yylval.chr = *yytext;
							  return CHARACTER;			}
<*>{ID}						{ parser.advancecol();
							  yylval.str = new std::string(yytext);
							  return IDENTIFIER;		}
<*>{STR}					{ parser.advancecol();
							  char* temp = yytext; ++temp;
							  yylval.str = new std::string(temp,yyleng-2);
							  return STRINGCONS;		}
<*>{INT}					{ parser.advancecol();
							  auto val = strtol(yytext, NULL, 10);
							  if(errno==ERANGE || val>INT_MAX || val<INT_MIN){
							  		Error::error("numeric value out of integer bounds");
							  }
							  yylval.nmr = val;
							  return INTEGER;		    
							}
<*>{FL}						{ parser.advancecol();
							  auto val = strtod(yytext, NULL);
							  if(errno==ERANGE){
							  		Error::error("numeric value out of double bounds");
							  }
							  yylval.dou = val;
							  return FLNUMBER;			}
<*>{CHR}					{ parser.advancecol();
							  yylval.chr = (yytext)[1];
							  return CHARCONS;
							}


	/*************************************
		Whitespaces, newlines and rest
	*************************************/

<*>{WHITESPACE}             { parser.advancecol();				}
<*>"\t"						{ parser.advancecol(); 
							  parser.prevlength = parser.tablen;		}
<*>"::"						{ parser.advancecol();
							  return NSPACE;			}
<spacename>"{"				{ parser.advancecol();
							  BEGIN(INITIAL);
							  return *yytext;
							}
"{"							{ parser.advancecol(); 
							  return *yytext;
							}
"}"							{ parser.advancecol(); 
							  return *yytext;
							}
<*>"{"						{ parser.advancecol(); 
							  ++parser.bracketcounter;
							  return *yytext;
							}
<*>"}"						{ parser.advancecol();
							  --parser.bracketcounter;
							  if(parser.bracketcounter == 0) {
								  BEGIN(INITIAL);
								  delete(yylloc.descr);
								  yylloc.descr = 0;
							  }
							  return *yytext;
							}
<*>.                        { parser.advancecol();
							  return *yytext;			}
<*>\n                       { parser.advanceline(); 		}

<<EOF>>						{ BEGIN(INITIAL);
							  if(not parser.include_buffer_stack.empty())	
								  parser.end_include();			
							  else yyterminate();
							}

%%

