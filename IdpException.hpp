#ifndef IDP_IDPEXCEPTION_HPP_
#define IDP_IDPEXCEPTION_HPP_

#include <exception>
#include <string>
#include <sstream>

class IdpException: public std::exception{
private:
	std::string message;

public:
	IdpException(const std::string& message): message(message){

	}

	virtual ~IdpException() throw(){}

	const char* what() const throw(){
		std::stringstream ss;
		ss <<"IdpException: " <<message;
		return ss.str().c_str();
	}
};

#endif /* IDP_IDPEXCEPTION_HPP_ */
