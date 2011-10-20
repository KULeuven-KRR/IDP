/************************************
 Generators.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef INSTGENERATOR_HPP_
#define INSTGENERATOR_HPP_

class InstGenerator {
public:
	virtual ~InstGenerator(){}

	//FIXME: it seems that first does NOT set the first generated instance, but merely CHECKS whether the currently set instance is a valid one
	//	so apparantly assuming that it is already set?
	/**
	 * Sets the variable to the first generated instance.
	 * Returns true iff such an instance can be generated.
	 */
	virtual bool first() const = 0;

	/**
	 * Sets the variable to the next generated instance.
	 * Returns true iff such an instance can be generated.
	 */
	virtual bool next() const = 0;
};

#endif /* INSTGENERATOR_HPP_ */
