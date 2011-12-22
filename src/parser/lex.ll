%{

#include "parser/yyltype.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include "parser/clconst.hpp"
#include "insert.hpp"
#include "parser.hh"
#include "error.hpp"
#include "common.hpp"
#include "GlobalData.hpp"
using namespace std;

extern YYSTYPE yylval;
extern YYLTYPE yylloc;
extern int yyparse();

Insert& getInserter();

extern void setclconst(string,string);

struct ParserData{

	ParserData(): tablen(4), bracketcounter(0), prevlength(0), stdin_included(false){

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
	vector<FILE*>			include_buffer_files;		// currently opened files
	vector<unsigned int>	include_line_stack;			// line number of the corresponding buffer
	vector<unsigned int>	include_col_stack;			// column number of the corresponding buffer
	bool					stdin_included;
	void start_include(string s) {
		// check if we are including from the included file
		for(unsigned int n = 0; n < include_buffer_filenames.size(); ++n) {
			if(*(include_buffer_filenames[n]) == s) {
				ParseInfo pi(yylloc.first_line,yylloc.first_column,getInserter().currfile());
				Error::cyclicinclude(s,pi);
				return;
			}
		}
		// store the current buffer
		include_buffer_stack.push_back(YY_CURRENT_BUFFER);
		include_buffer_filenames.push_back(getInserter().currfile());
		include_buffer_files.push_back(yyin);
		include_line_stack.push_back(yylloc.first_line);
		include_col_stack.push_back(yylloc.first_column + prevlength);
		// open the new buffer
		FILE* oldfile = yyin;
		yyin = fopen(s.c_str(),"r");
		if(!yyin) {
			ParseInfo pi(yylloc.first_line,yylloc.first_column,getInserter().currfile());
			Error::unexistingfile(s,pi);
			include_buffer_stack.pop_back();
			include_buffer_filenames.pop_back();
			include_buffer_files.pop_back();
			include_line_stack.pop_back();
			include_col_stack.pop_back();
			yyin = oldfile;
		}
		else {
			yylloc.first_line = 1;
			yylloc.first_column = 1;
			yylloc.descr = 0;
			prevlength = 0;
			getInserter().currfile(s);
			yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
		}
	}
	void start_stdin_include() {
		if(stdin_included) {
			ParseInfo pi(yylloc.first_line,yylloc.first_column,getInserter().currfile());
			Error::twicestdin(pi);
		}
		else {
			// store the current buffer
			include_buffer_stack.push_back(YY_CURRENT_BUFFER);
			include_buffer_filenames.push_back(getInserter().currfile());
			include_buffer_files.push_back(yyin);
			include_line_stack.push_back(yylloc.first_line);
			include_col_stack.push_back(yylloc.first_column + prevlength);
			// open then new buffer
			Warning::readingfromstdin();
			stdin_included = true;
			yyin = stdin;
			getInserter().currfile(0);
			yy_switch_to_buffer(yy_create_buffer(yyin,YY_BUF_SIZE));
		}
	}
	void end_include() {
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(include_buffer_stack.back());
		yylloc.first_line = include_line_stack.back();
		yylloc.first_column = include_col_stack.back();
		prevlength = 0;
		getInserter().currfile(include_buffer_filenames.back());
		include_buffer_stack.pop_back();
		include_buffer_filenames.pop_back();
		//fclose(include_buffer_files.back());
		include_buffer_files.pop_back();
		include_line_stack.pop_back();
		include_col_stack.pop_back();
	}
};

ParserData data;

void reset(){
	data = ParserData();
	BEGIN(0);
}


%}

%option noyywrap
%option nounput
%option never-interactive

%x comment
%x vocabulary
%x structure
%x aspstructure
%x theory
%x include
%x procedure
%x lua
%x description
%x descontent
%x spacename

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

<*>"/**"/[^*]				{ data.commentcaller = YY_START;
							  BEGIN(description);
							  data.advancecol();
							  yylloc.descr = new stringstream();
							}
<description>[^*\n \r\t]*	{ data.advancecol(); (*yylloc.descr) << yytext; BEGIN(descontent);	}
<description>[*]*			{ data.advancecol();													}
<description>"*"+"/"		{ BEGIN(data.commentcaller);           
							  data.advancecol();				}
<descontent>[^*\n]*			{ data.advancecol(); (*yylloc.descr) << yytext;			}
<descontent>[^*\n]*\n		{ data.advanceline(); (*yylloc.descr) << yytext << "        "; BEGIN(description);		}
<descontent>"*"+[^*/\n]*	{ data.advancecol();	(*yylloc.descr) << yytext;			}
<descontent>"*"+[^*/\n]*\n	{ data.advanceline(); (*yylloc.descr) << yytext << "        ";	BEGIN(description);		}
<descontent>"*"+"/"			{ BEGIN(data.commentcaller);           
							  data.advancecol();				}

<*>"/*"						{ data.commentcaller = YY_START;
							  BEGIN(comment);	
							  data.advancecol();				}
<comment>[^*\n]*			{ data.advancecol();				}
<comment>[^*\n]*\n			{ data.advanceline();			}
<comment>"*"+[^*/\n]*		{ data.advancecol();				}
<comment>"*"+[^*/\n]*\n		{ data.advanceline();			}
<comment>"*"+"/"			{ BEGIN(data.commentcaller);           
							  data.advancecol();				}

	/*************
		Include
	*************/

<*>"#include"				{ data.advancecol();
							  data.includecaller = YY_START;
							  BEGIN(include);
							}

	/**********
		Lua
	**********/

<procedure>"{"				{ data.advancecol();
							  BEGIN(lua);
							  ++data.bracketcounter;		
							  data.luacode = new stringstream();
							  return *yytext;
							}
<lua>"::"					{ data.advancecol(); (*data.luacode) << '.'; }
<lua>"{"					{ data.advancecol(); 
							  ++data.bracketcounter;
							  (*data.luacode) << '{';
							}
<lua>"}"					{ data.advancecol();
							  --data.bracketcounter;
							  if(data.bracketcounter == 0) { 
								  yylval.sstr = data.luacode;
								  BEGIN(INITIAL); 
								  delete(yylloc.descr);
								  yylloc.descr = 0;
								  return LUACHUNK;	
							  }
							  else (*data.luacode) << '}';
							}
<lua>\n						{ data.advanceline(); (*data.luacode) << '\n';	}
<lua>[^/{}:\n*]*				{  // //, :, { and } and \n are matched before (single / FALLS THROUGH to last one (.) )
								data.advancecol();	
								(*data.luacode) << yytext;
							}
<lua>{STR}					{ data.advancecol(); (*data.luacode) << yytext;	}
<lua>{CHR}					{ data.advancecol();	(*data.luacode) << yytext;	}
<lua>.						{ data.advancecol(); (*data.luacode) << *yytext;	}


	/***************
		Headers 
	***************/

"vocabulary"			{ BEGIN(vocabulary);
						  data.advancecol();
						  return VOCAB_HEADER;		}
"theory"				{ BEGIN(theory);
						  data.advancecol();
						  return THEORY_HEADER;		}
"structure"				{ BEGIN(structure);
						  data.advancecol();
						  return STRUCT_HEADER;		}
"aspstructure"			{ BEGIN(aspstructure);
						  data.advancecol();
						  return ASP_HEADER;		}
"namespace"				{ BEGIN(spacename);
						  data.advancecol();
						  return NAMESPACE_HEADER;	}
"procedure"				{ BEGIN(procedure);
						  data.advancecol();
						  return PROCEDURE_HEADER;	}
"query"					{ BEGIN(theory);
						  data.advancecol();
						  return QUERY_HEADER;
						}
"term"					{ BEGIN(theory);
						  data.advancecol();
						  return TERM_HEADER;
						}

	/**************
		Include
	**************/

<include>"stdin"				{ data.advancecol();
								  data.start_stdin_include();
								  BEGIN(data.includecaller);
								}
<include>"<"[a-zA-Z0-9_/]*">"	{ data.advancecol();
								  char* temp = yytext; ++temp;
								  string str = string(IDPDATADIR) + "/std/" + string(temp,yyleng-2) + ".idp";
								  data.start_include(str);	
								  BEGIN(data.includecaller);
								}
<include>"$"[a-zA-Z0-9_]*		{ data.advancecol();
								  string temp(yytext);
								  temp = temp.substr(1,temp.size()-1);
								  if(GlobalData::instance()->getConstValues().find(temp) != GlobalData::instance()->getConstValues().end()) {
									  auto clc = GlobalData::instance()->getConstValues().at(temp);
									  if(typeid(*clc) == typeid(StrClConst)) {
										  auto slc = dynamic_cast<StrClConst*>(clc);
										  data.start_include(slc->value());
									  } else {
										  ParseInfo pi(yylloc.first_line,yylloc.first_column,getInserter().currfile());
										  Error::stringconsexp(temp,pi);
									  }
									  BEGIN(data.includecaller);
								  }
								  else {
									  clog << "Type a value for constant " << temp << endl << "> "; 
									  string str;
									  getline(cin,str);
									  data.start_include(str);
									  BEGIN(data.includecaller);
								  }
								}
<include>{STR}					{ data.advancecol();
								  char* temp = yytext; ++temp;
								  string str(temp,yyleng-2);
								  data.start_include(str);	
								  BEGIN(data.includecaller);
								}

	/****************
		Vocabulary
	****************/

	/** Keywords **/

<vocabulary>"type"              { data.advancecol();
								  return TYPE;				}
<vocabulary>"partial"			{ data.advancecol();
								  return PARTIAL;			}
<vocabulary>"isa"				{ data.advancecol();
								  return ISA;				}
<vocabulary>"contains"			{ data.advancecol();
								  return EXTENDS;			}
<vocabulary>"extern"			{ data.advancecol();
								  return EXTERN;			}
<vocabulary>"extern vocabulary"	{ data.advancecol();
								  return EXTERNVOCABULARY;	}

	/*************
		Theory
	*************/


	/** Aggregates **/

<theory>"card"				{ data.advancecol();
							  return P_CARD;				}
<theory>"#"					{ data.advancecol();
							  return P_CARD;				}
<theory>"sum"				{ data.advancecol();
							  return P_SOM;				}
<theory>"prod"				{ data.advancecol();
							  return P_PROD;				}
<theory>"min"				{ data.advancecol();
							  return P_MINAGG;			}
<theory>"max"				{ data.advancecol();
							  return P_MAXAGG;			}

	/** Fixpoint definitions **/

<theory>"LFD"				{ data.advancecol(); 
							  return LFD;				}
<theory>"GFD"				{ data.advancecol();
							  return GFD;				}

	/** Arrows **/

<theory>"=>"                { data.advancecol();
							  return P_IMPL;				}
<theory>"<="				{ data.advancecol();
							  return P_RIMPL;				}
<theory>"<=>"				{ data.advancecol();
							  return EQUIV;				}
<theory>"<-"				{ data.advancecol();
							  return DEFIMP;			}
	/** True and false **/

<theory>"true"				{ data.advancecol();
							  return TRUE;				}
<theory>"false"				{ data.advancecol();
							  return FALSE;				}

	/** Comparison **/

<theory>"=<"                { data.advancecol();
						   	  return P_LEQ;				}
<theory>">="                { data.advancecol();
						   	  return P_GEQ;				}
<theory>"~="                { data.advancecol();
							  return P_NEQ;				}
	/** Ranges **/

<theory>".."				{ data.advancecol();
							  return RANGE;				}


	/****************
		Structure 
	****************/

<structure>"->"				{ data.advancecol();
							  return MAPS;				}
<structure>".."				{ data.advancecol();
							  return RANGE;				}
<structure>"true"			{ data.advancecol();
							  return TRUE;				}
<structure>"false"			{ data.advancecol();
							  return FALSE;				}
<structure>"procedure"		{ data.advancecol();
							  return PROCEDURE;			}
<structure>"generate"		{ data.advancecol();
							  return CONSTRUCTOR;		}
<aspstructure>"%".*			{							}
<aspstructure>".."			{ data.advancecol();
							  return RANGE;				}


	/******************
		Identifiers
	******************/

<*>"using vocabulary"		{ data.advancecol();
							  return USINGVOCABULARY;				
							}
<*>"using namespace"		{ data.advancecol();
							  return USINGNAMESPACE;			
							}
<*>"using"					{
								cerr <<"Can only use the keyword \"using\" as \"using vocabulary\" or \"using namespace\".\n";
								yyterminate();
							}
<*>{CH}						{ data.advancecol();
							  yylval.chr = *yytext;
							  return CHARACTER;			}
<*>{ID}						{ data.advancecol();
							  yylval.str = StringPointer(yytext);
							  return IDENTIFIER;		}
<*>{STR}					{ data.advancecol();
							  char* temp = yytext; ++temp;
							  yylval.str = StringPointer(string(temp,yyleng-2));
							  return STRINGCONS;		}
<*>{INT}					{ data.advancecol();
							  yylval.nmr = atoi(yytext);
							  return INTEGER;		    }
<*>{FL}						{ data.advancecol();
							  yylval.dou = atof(yytext);
							  return FLNUMBER;			}
<*>{CHR}					{ data.advancecol();
							  yylval.chr = (yytext)[1];
							  return CHARCONS;
							}
<*>"$"[a-zA-Z0-9_]*			{ data.advancecol();
							  string temp(yytext);
							  temp = temp.substr(1,temp.size()-1);
							  if(GlobalData::instance()->getConstValues().find(temp)== GlobalData::instance()->getConstValues().end()) {
								  clog << "Type a value for constant " << temp << endl << "> "; 
								  string str;
								  getline(cin,str);
								  GlobalData::instance()->setConstValue(temp,str);
							  }
							  return (GlobalData::instance()->getConstValues().at(temp))->execute();
							}


	/*************************************
		Whitespaces, newlines and rest
	*************************************/

<*>{WHITESPACE}             { data.advancecol();				}
<*>"\t"						{ data.advancecol(); 
							  data.prevlength = data.tablen;		}
<*>"::"						{ data.advancecol();
							  return NSPACE;			}
<spacename>"{"				{ data.advancecol();
							  BEGIN(INITIAL);
							  return *yytext;
							}
"{"							{ data.advancecol(); 
							  return *yytext;
							}
"}"							{ data.advancecol(); 
							  return *yytext;
							}
<*>"{"						{ data.advancecol(); 
							  ++data.bracketcounter;
							  return *yytext;
							}
<*>"}"						{ data.advancecol();
							  --data.bracketcounter;
							  if(data.bracketcounter == 0) {
								  BEGIN(INITIAL);
								  delete(yylloc.descr);
								  yylloc.descr = 0;
							  }
							  return *yytext;
							}
<*>.                        { data.advancecol();
							  return *yytext;			}
<*>\n                       { data.advanceline();			}

<<EOF>>						{ BEGIN(INITIAL);
							  if(not data.include_buffer_stack.empty())	
								  data.end_include();			
							  else yyterminate();
							}

%%

