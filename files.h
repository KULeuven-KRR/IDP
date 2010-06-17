/************************************
	files.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FILES_H
#define FILES_H

#include <cstdio>

struct Files {

		// Attributes
		FILE* 			_outputfile;
		FILE* 			_errorfile;
//		vector<FILE*> 	_inputfiles;

		// Constructor
		Files() {
			_outputfile = stdout;
			_errorfile = stderr;
//			_inputfiles.push_back(stdin);
		}

		// Destructor (close files)
		~Files() {
			fclose(_outputfile);
			fclose(_errorfile);
//			for(unsigned int n = 0; n < _inputfiles.size(); ++n)
//				fclose(_inputfiles[n]);
		}

};

#endif
