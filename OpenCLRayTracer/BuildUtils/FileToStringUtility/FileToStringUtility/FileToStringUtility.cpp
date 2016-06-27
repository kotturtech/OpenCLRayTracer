// FileToStringUtility.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <boost\algorithm\string\replace.hpp>

using namespace std;

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		cout << "Expected arguments: filetostring <source file> <target file> <variable name>" << endl;
		return 0;
	}
	else 
	{
		cout << "Source: " << argv[1] << " Target: " << argv[2] << " Variable: " << argv[3] << endl;
	}

	ifstream* in = NULL;
	ofstream* out = NULL;
	try
	{
		in = new ifstream(argv[1]);

	}
	catch (exception e)
	{
		cout << "Error opening source file: " << e.what() << endl;
		return -1;
	}

	try
	{
		out = new ofstream(argv[2]);
	}
	catch (exception e)
	{
		cout << "Error opening source file: " << e.what() << endl;
		return -1;
	}

	if (!in->is_open())
	{
		cout << "Error opening source file! " << endl;
		return -1;
	}

	if (!out->is_open())
	{
		cout << "Error opening target file! " << endl;
		return -1;
	}

	*out << "const char* " << argv[3] << " = \n";
	string line;
	
	while (getline(*in,line))
	{
		line = boost::replace_all_copy(line,"\n","");
		line = boost::replace_all_copy(line,"\r","");
		line = boost::replace_all_copy(line,"\\","\\\\");
		line = boost::replace_all_copy(line,"\"","\\\"");
		*out << "\"" << line << "\\n\"\n";
	}
	*out << ";\n";

	in->close();
	out->close();
	
	return 0;
}

