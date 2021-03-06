/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKEDN_EXCEPTION_H
#define LINKEDN_EXCEPTION_H

#include <exception>
#include <string>

class LnException : public std::exception
{
public:
  LnException(const std::string & errMsg) throw();
  
  ~LnException() throw()
  {}
  
  const char* what() const throw()
  {
    return m_errMsg.c_str();
  }
  
private:
  const std::string m_errMsg;
};

#endif
