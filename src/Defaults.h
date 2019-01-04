/*
 * Defaults.h
 */

#ifndef SRC_DEFAULTS_H_
#define SRC_DEFAULTS_H_

#include <stdint.h>
//#include <cstdint>
#include <string>

// Audio sample rate
const int SAMPLE_RATE = 44100; //!< audio sample rate
typedef int16_t audio_t; //!< audio sample data type
const audio_t audio_A = 32767; //!< amplitude for audio samples

#if DEBUGOMP == 1
#include <iostream>
#include <fstream>
#include <string.h>
#include <typeinfo>
#include <assert.h>
#include <stdlib.h>

template<typename T>
void save_debug_data( T * data, unsigned int n, std::string comment = "")
{
	std::cout << "saving debug data : " << comment << std::endl;
	std::ofstream f("./debug_data.dat", std::ios_base::binary );
	if( f.is_open() == false )
	{
		std::cerr << "Error outputing debug data" << std::endl;
		return;
	}
	// Write the datatype in numpy friendly format in the first 10 bytes (space padded):
	const char * my_type = typeid(T).name();
	std::cout << "Encoding to type : " << my_type << std::endl;
	if( strcmp(my_type,typeid(unsigned int).name()) == 0 )
	{
		if(sizeof(unsigned int) == 4 )
			f.write( "uint32          ", 10);
		else if(sizeof(unsigned int) == 2)
			f.write( "uint16          ", 10);
		else if(sizeof(unsigned int) == 8)
			f.write( "uint64          ", 10);
		else
		{
			std::cout << "Unhandled size of unsigned int : " << sizeof(unsigned int) << std::endl;
			f.write("                   ", 10);
		}
	}
	else if( strcmp(my_type,typeid(int16_t).name()) == 0)
	{
		f.write( "int16          ", 10);
	}
	else if( strcmp(my_type,typeid(float).name()) == 0 )
	{
		f.write( "float32          ", 10);
	}
	else
	{
		std::cout << "Unhandled file type" << std::endl;
		f.write("                   ", 10);
	}
	f.write( (const char *) data, n*sizeof(T) );
	f.close();
	exit(EXIT_SUCCESS);
}
#else
template<typename T>
void save_debug_data( T * data, unsigned int n, std::string comment = "" )
{}
#endif

#endif /* SRC_DEFAULTS_H_ */
