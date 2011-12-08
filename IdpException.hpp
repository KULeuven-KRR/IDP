#ifndef IDP_IDPEXCEPTION_HPP_
#define IDP_IDPEXCEPTION_HPP_

#include <sstream>
#include <string>

class Exception{
public:
	~Exception(){}
	virtual std::string getMessage() const = 0;
};

class AssertionException: public Exception{
private:
	std::string message;
public:
	AssertionException(std::string message): message(message){

	}
	std::string getMessage() const{
		std::stringstream ss;
		ss <<"AssertionException: " <<message;
		return ss.str();
	}
};

class IdpException: public Exception{
private:
	std::string message;
public:
	IdpException(std::string message): message(message){

	}
	std::string getMessage() const{
		std::stringstream ss;
		ss <<"IdpException: " <<message;
		return ss.str();
	}
};

#endif /* IDP_IDPEXCEPTION_HPP_ */
