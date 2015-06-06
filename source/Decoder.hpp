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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <math.h>
#include <sstream>
#include <ctype.h>
#include <vector>
#include <algorithm>
#include "Lists.hpp"
#include "Functions.hpp"
#include "Parameters.hpp"
#include "NumberGen.hpp"
#include "ProbabilityDistro.hpp"

using namespace std;

#ifndef __Decoder_hpp
#define __Decoder_hpp
class Decoder
{
	public:
		////////////   Main variables   /////////////////
		Instructions INS;
		Operands *OPR;
		string *REG;
		string *IMM;
		NumberGen *NUMG;
		////////////   Temporary variables   ////////////
		vector<string> lookup;
		vector<string> lookupR;
		vector<string> lookupI;
		////////////   Length variables   ///////////////
		int INS_len, REG_len, IMM_len, OPR_len;
		////////////   Masks and Unuseds ////////////////
		BYTE2 INS_mask;
		BYTE1 REG_mask;
		BYTE1 IMM_mask;
		int INS_unused, REG_unused, IMM_unused;
		////////////   Functions   //////////////////////
						Decoder();
						Decoder(string filename);
						~Decoder();
		Decoder&		operator=(Decoder& dec); 
		void			read_instructions(string filename);
		vector<string>	build_lookup(string filename);
		void			update_lookup(vector<string> *lookup, Operands ops);
		BYTE4			get_mask4(int length, int *unused);
		BYTE2			get_mask2(int length, int *unused);
		BYTE1			get_mask1(int length, int *unused);
		void			finalize_lookup(vector<string> lookup, string *sarr);
		BYTE4			random_instruction();
		BYTE4			random_instruction(ProbabilityDistro *pd);
		BYTE4			specific_instruction(int number);
		BYTE4			specific_instruction(int number, int param1, int param2);
		BYTE4			random_parameter(int ops_index, int *size);
		BYTE4			specific_parameter(int ops_index, int *size, int param);
		int				operandIdx_lookup(string *array, string target, int array_len); 
		string			decode_instruction(BYTE4 inst);
		bool			validate_instruction(BYTE4 inst);
};
#endif




