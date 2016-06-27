/**
 * @file Errata.h
 * @author  Timur Sizov <timorgizer@gmail.com>
 * @version 0.6
 *
 * @section LICENSE
 *
 * Copyright (c) 2016 Timur Sizov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
 * to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Part of the OpenCL wrapper framework. Contains basic error handling structs and macros,
 * that will be used throughout the libraries
 *
 */

#ifndef CL_RT_ERRATA_H
#define CL_RT_ERRATA_H
#include <sstream>

/**Macro for fast filling of Errata object*/
#define FILL_ERRATA(err,errorMessage) {std::stringstream msg; msg << errorMessage; err = CLRayTracer::Common::Errata(std::string((msg.str())),std::string(__FILE__),std::string(__FUNCTION__),__LINE__,NULL);}
/**Macro for fast filling of Errata object*/
#define FILL_ERRATA(err,errorMessage,xcpt) {std::stringstream msg; msg << errorMessage; err = CLRayTracer::Common::Errata(std::string((msg.str())),std::string(__FILE__),std::string(__FUNCTION__),__LINE__,new std::exception(xcpt));}
/**Macro for fast throwing of CL exception on error*/
#define THROW_CL_EXCEPTION(errorMessage,innerException) {std::stringstream msg; msg << errorMessage; throw new CLRayTracer::Common::CLInterfaceException(std::string((msg.str())),std::string(__FILE__),std::string(__FUNCTION__),__LINE__,innerException);}

namespace CLRayTracer
{
	namespace Common
	{
		/**enum Result indicates operation result - Success or Error*/
		enum Result
		{
			Success = 0,
			Error = 1
		};

		class Errata;
		/**Allows easy output of Errata object*/
		std::ostream& operator<<(std::ostream& o,const Errata& err);

		/**class Errata contains error information*/
		class Errata
		{
			friend std::ostream& operator<<(std::ostream& o,const Errata& err);
		public:
			/**Constructor
			* @param errorMessage The error message
			* @param file Code file name, in which the error had occurred
			* @param function Function in which the error occurred
			* @param line Line in which the error had occurred
			* @param [optional]exception Exception that caused the error
			*/
			Errata(const std::string& errorMessage, const std::string&file, const std::string& function,int line, const std::exception* ex = NULL)
				:_message(errorMessage),_file(file),_function(function),_line(line),_exception(ex)
			{
			}

			/**Default constructor*/
			Errata():_exception(NULL)
			{

			}

			/**Returns the error message
			*@return Error message
			*/
			std::string message()const {return _message;}
			
			/**Returns the code file name in which the error occurred
			*@return Code file name in which the error occurred
			*/
			std::string file() const {return _file;}
			
			/**Returns the function in which the error occurred
			*@return Function in which the error occurred
			*/
			std::string function() const {return _function;}
			
			/**Returns the line number in code file, where the error occurred
			*@return Line number in code file, where the error occurred
			*/
			int line() const {return _line;}
			
			/**Returns the exception object that caused the error 
			*@return Exception object that caused the error 
			*/
			const std::exception* exception() const {return _exception;}
		protected:
			std::string _message;
			std::string _file;
			std::string _function;
			int _line;
			const std::exception* _exception;
		};

		/** class CLInterfaceException extends class Errata to be throwable as exception*/
		class CLInterfaceException : public std::exception, public Errata
		{
		public: 
			/**Constructor
			* @param errorMessage The error message
			* @param file Code file name, in which the error had occurred
			* @param function Function in which the error occurred
			* @param line Line in which the error had occurred
			* @param [optional]exception Exception that caused the error
			*/
			CLInterfaceException(const std::string& errorMessage, const std::string&file, const std::string& function,int line, std::exception* ex = NULL):
				Errata(errorMessage,file,function,line,ex),std::exception(errorMessage.c_str())
			{
			}

			/**Constructor that creates Exception from Errata object*/
			CLInterfaceException(const Errata& err):Errata(err.message(),err.file(),err.function(),err.line(),err.exception()),std::exception(err.message().c_str()){}
		};

		/**Operator for easy output of error objects*/
		inline std::ostream& operator<<(std::ostream& o,const Errata& err)
		{
			o << err._file << '(' << err._line << ')' << ' ' << err._function << ' ' << err._message << ' ' << (err._exception ? err._exception->what() : "") << std::endl; 
			return o;
		}

	}
}


#endif