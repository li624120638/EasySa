/*************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *************************************************************************/

#ifndef FRAMEWORK_CORE_INCLUDE_EASYSA_ERROR_HPP_
#define FRAMEWORK_CORE_INCLUDE_EASYSA_ERROR_HPP_

/*
* @brief this file contains a declartion of EasysaError
*/

#include <stdexcept>
#include <string>

/*
* Registor exception class derived from EasysaError
*/
#define EASYSA_REGISTOR_EXCEPTION(CNAME) \
  class CAME##Error : public easysa::EasysaError { \
	public:                                      \
     explicit CNAME##Error(const std::string& msg) : EasysaError(msg){} \ 
  };
  namespace easysa {
	  class EasysaError : public std::runtime_error {
	  public:
		  explicit EasysaError(const std::string& msg) : std::runtime_error(msg) {}
	  }; // class EasysaError
  }// namespace easysa
#endif // FRAMEWORK_CORE_INCLUDE_EASYSA_ERROR_HPP_