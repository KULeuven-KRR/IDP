/************************************
	ModelExpansion.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CALCDEF_HPP_
#define CALCDEF_HPP_



class AbstractStructure;
class Theory;
class Definition;


class CalculateDefinitions {
public:
	static bool doCalculateDefinitions(Theory* theory, AbstractStructure* structure){
		CalculateDefinitions c;
		return c.calculateKnownDefinitions( theory,  structure);
	}
private:

	bool calculateDefinition(Definition* definition, AbstractStructure* structure) const;

	bool calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) const;


};

#endif //CALCDEF_HPP_
