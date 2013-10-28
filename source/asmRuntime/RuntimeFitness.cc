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

#include "RuntimeFitness.hpp"

RuntimeFitness::RuntimeFitness():asmRuntime() { };
RuntimeFitness::~RuntimeFitness() { };
RuntimeFitness::RuntimeFitness(string program):asmRuntime(program) { };
RuntimeFitness::RuntimeFitness(char *file):asmRuntime(file) { };
/* Run the code and determine which 
 * fitness function to use. This is
 * a placeholder for fitness func. selection
 */
double RuntimeFitness::fitness()
{
	run();
	return fitness_fn1();
};

double RuntimeFitness::fitness_fn1()
{
	int dummy;
	double ret = 10;
	unsigned int* edx = findRegister("EDX", &dummy);
	if (*edx != 0) --ret;
	unsigned int* eax = findRegister("EAX", &dummy);
	if (*eax != 0x0000000b) --ret;
	string upstr = toUpper(program);
	int intloc =  upstr.find("INT");
	if ( intloc == string::npos ) --ret;
	unsigned int pattern[3];
	pattern[0] = 0x00000000; pattern[1] = 0x68732f2f; pattern[2] = 0x6e69622f;
	//cout<<"Before check 1 ret="<<ret<<endl;
	// CHECK 1 : Does "/bin//sh0" exist in stack? 
	int check1 = 0;
	// Three byte combination check
	int patloc = stack_search (pattern, 3, 0);
	if ( patloc == -1 )
	{
		// Check for two byte combinations {0 str2} and {str2 str1}
		if ( ( stack_search (pattern, 2, 0) == -1 ) && ( stack_search (pattern + 1, 2, 0) == -1 ) )
		{
			// One byte combinations
			if (stack_search (pattern    , 1 , 0 ) == -1 && \
				stack_search (pattern + 1, 1 , 0 ) == -1 && \
				stack_search (pattern + 2, 1 , 0 ) == -1 ) { check1 = 3; ret -= 3; } // Needs to push the entire string
			else { check1 = 2; ret -= 2; } // Push 2 pieces instead of three
		}
		else
		{
			check1 = 1; // Needs to push only 1 piece
			--ret;
		} 
	}
	//cout<<"Before check 2 ret="<<ret<<endl;
	// CHECK 2 : does ebx point to "/bin//sh0"
	int check2 = 0;
	unsigned int* ebx = findRegister("EBX", &dummy);
	if (check1 != 0 ) { --ret; check2 = 1; }// Check 1 failed, don't bother with further check
	else
	{
		int sloc = stack_search (pattern, 3, 0);
		int is_correct = 0;
		while (sloc != -1 && is_correct == 0)
		{
			if ( (unsigned int) &stack[sloc + 2] == *ebx ) {is_correct = 1;}// Needs nothing
			sloc = stack_search (pattern, 3, sloc + 1);
		}
		if (is_correct == 0) { ret--; check2 = 2; }  // Needs to store str esp in ebx
	} 
	//cout<<"Before check 3 ret="<<ret<<endl;
	// CHECK 3 : Does the argument array pushed properly?
	int check3 = 0;
	unsigned int* ecx = findRegister("ECX", &dummy);
	if (check2 != 0 ) { ret -= 3; check3 = 1; }// Check 2 failed, don't bother with further check
	else
	{
		unsigned int argpatt[2];
		int argloc = -1;
		argpatt[0] = 0x00000000; argpatt[1] = *ebx;
		argloc = stack_search (argpatt, 2, 0);
		if (argloc != -1)
		{
			//printf("%d Argloc pos : %.8x ebx=%.8x\n", argloc, (unsigned int) &stack[argloc], argpatt[1] );
			if ( (unsigned int) &stack[argloc + 1] != *ecx ) --ret; // just need to store esp in ecx
		}
		else
		{
			int l1 = stack_search(argpatt, 1, 0 );
			while (l1 == patloc && l1 != -1)
				l1 = stack_search(argpatt, 1, l1 + 1 );
			int l2 = stack_search(argpatt + 1, 1, 0 );
			while (l2 == patloc && l2 != -1)
				l2 = stack_search(argpatt + 1, 1, l2 + 1 );
			if ( (l1 != -1 && l1 != patloc) && \
				 (l2 != -1 && l2 != patloc) ) ret -= 2; // needs to push one more and store esp
			else
				ret -=3; // needs the entire 3
		}
	}
	//cout<<"Before return ret="<<ret<<endl;
	return ret;
};


