/*
 *  kayacik/linear-gp
 *	Linear GP for Evolving System Call Sequences
 *	Copyright 2006-2013, Gunes Kayacik
 *
 *	This file is part of kayacik/linear-gp.
 *
 *  kayacik/linear-gp is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  kayacik/linear-gp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with kayacik/linear-gp.  If not, see <http://www.gnu.org/licenses/>.
 */

// The variables defined in this header are fixed for all experiments
#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP
const std::string PRMDIR = "../instruction-sets/";
/* On a G4 iMac the values below correspond to 
 * 4, 2, and 1 byte respectively 
 */
typedef unsigned int   BYTE4;
typedef unsigned short BYTE2;
typedef unsigned char  BYTE1;
#define UNDEF_BYTE4 0xFFFFFFFF
#define UNDEF_BYTE2 0xFFFF
#define UNDEF_BYTE1 0xFF
#define DETAILS __FILE__<<":"<<__LINE__
const int DebugLevel = 5;
// decided to crreate a struct for training params so that I don't have to change how many params
// population constructor takes. We'll see how this works out.
typedef struct
{
	int enable_owc;
	int xo_type;
	int mut_type;
	int swp_type;
}  configuration;
#endif



