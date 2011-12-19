/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*
* Use of this software is governed by the GNU LGPLv3.0 license
*
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef FILEMANAGEMENT_HPP_
#define FILEMANAGEMENT_HPP_

#include <vector>
#include <string>

std::vector<std::string> getAllFilesInDirs(const std::string& prefix, const std::vector<std::string>& dirs);

#endif /* FILEMANAGEMENT_HPP_ */
