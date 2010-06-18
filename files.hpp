/************************************
	files.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FILES_HPP
#define FILES_HPP

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
