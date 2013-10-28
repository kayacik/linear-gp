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

#include "NumberGen.hpp"

NumberGen::NumberGen () 
{ 
	seed = -20;//time(NULL); 
	MBIG = 1000000000;     
	MSEED = 161803398;     
	FAC = 1.0/MBIG;
	cout<<"Number generator initialized with seed "<<seed<<endl;
};
NumberGen::NumberGen (long sd) 
{ 
	seed = sd; 
	MBIG = 1000000000;     
	MSEED = 161803398;     
	FAC = 1.0/MBIG;
	cout<<"Number generator initialized with seed "<<seed<<endl;
};

NumberGen::~NumberGen() { };
/********************************************************************************
 * ran3 function from Numerical Recipes in C (p. 283). Generates floats between [0 1]                          
 */
float NumberGen::ran3()
{
		 static int inext,inextp;
         static long ma[56];
         static int iff=0;
         long mj,mk;
         int i,ii,k;
 
         if (seed < 0 || iff == 0) {
				 iff=1;
                 mj=MSEED-(seed < 0 ? -seed : seed);
                 mj = (long) mj % (long) MBIG;
                 ma[55]=mj;
                 mk=1;
                 for (i=1;i<=54;i++) {
                         ii=(21*i) % 55;
                         ma[ii]=mk;
                         mk=mj-mk;
                         if (mk < 0.0) mk += MBIG;
                         mj=ma[ii];
                 }
                 for (k=1;k<=4;k++)
                         for (i=1;i<=55;i++) {
                                 ma[i] -= ma[1+(i+30) % 55];
                                 if (ma[i] < 0.0) ma[i] += MBIG;
                         }
                 inext=0;
                 inextp=31;
                 seed=1;
         }
         if (++inext == 56) inext=1;
         if (++inextp == 56) inextp=1;
         mj=ma[inext]-ma[inextp];
         if (mj < 0.0) mj += MBIG;
         ma[inext]=mj;
         return mj*FAC;
};

/********************************************************************************
 * irbit2 function from Numerical Recipes in C (p. 298). Generates random bits.
 */
int NumberGen::irbit2(unsigned long *iseed)
{
	if (*iseed & IB18) {
		*iseed=((*iseed ^ MASK) << 1) | IB1;
		return 1;
	} else {
		*iseed <<= 1;
		return 0;
	}
};

/********************************************************************************
 * Using ran3, generates integers between lo and hi (inclusive)
 */
int NumberGen::uniform_intgen(int lo, int hi)
{
	float rf = 0;
	int rn =  0;
	int done = 0;
	while (!done)
	{
		rf = ran3();
		 
		rn = (int) floor( rf*(hi - lo + 1) + lo );
		//cout<<"rf="<<rf<<" rn="<<rn<<" seed="<<seed<<endl;
		if (rn != hi + 1) done = 1; // in case ran3() returns 1.0
	}
	return rn;
};

