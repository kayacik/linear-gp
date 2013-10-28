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
using namespace std;

#ifndef __ProbabilityDistro_hpp
#define __ProbabilityDistro_hpp
// This code is optimized. You should use initialize() to update the values and counts
// And use getValue. Don't invome calculate*() functions explicitly.

class ProbabilityDistro
{
	public:
		double total;
		vector<double> values;
		vector<double> counts;
		vector<double> PDF;
		vector<double> CDF;
		ProbabilityDistro();
		~ProbabilityDistro();
		void initialize(vector<double> vals, vector<double> cnts);
		void printPDF_CDF();
		double getValue(double number);		// sequential search
		double getValue2(double number);	// binary search
		ProbabilityDistro& operator=(ProbabilityDistro& pd);
		bool operator==(ProbabilityDistro& pd);  
	private:
		void calculate();
		void calculatePDF();
		void calculateCDF();
};
#endif