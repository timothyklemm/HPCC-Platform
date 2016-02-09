/* A simple header to use SHA2 hash algorithms */

#ifndef __SHA2_HPP__
#define __SHA2_HPP__

#include "jstring.hpp"

jlib_decl StringBuffer& SHA256_string(const char* in, StringBuffer& out);
jlib_decl StringBuffer& SHA384_string(const char* in, StringBuffer& out);
jlib_decl StringBuffer& SHA512_string(const char* in, StringBuffer& out);

#endif /* __SHA2_HPP__ */

