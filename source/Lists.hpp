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
#include "Functions.hpp"
#include "ProbabilityDistro.hpp"
using namespace std;

#ifndef __Lists_hpp
#define __Lists_hpp
class Instructions
{
	public:
		double *probs;
		string *values;
		int *p1link;
		int *p2link;
		int size;
		BYTE4 mask;
		int unused;
		Instructions();
		~Instructions();
		Instructions(int len);
		Instructions (string file , vector<string> lookup);
		Instructions& operator=(const Instructions& in);
		void print();
		};

class Operands
{
	public:
		double *probs;
		string *values;
		int *length;
		int size;
		int type; // 0 regs, 1 imms
		int *op32, *op16, *op8;
		int op32_len, op16_len, op8_len;
		BYTE4 mask;
		int unused;
		Operands();
		~Operands();
		Operands(const Operands& op);
		Operands(int len);
		Operands(string file);
		Operands& operator=(const Operands& op);
		void print();
};
#endif




