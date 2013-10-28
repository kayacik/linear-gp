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

#include "Decoder.hpp"
#include "Population.hpp"
#include "Operations.hpp"
#include "Parameters.hpp"
#include "Functions.hpp"

int main(int argc, char **argv)
{
long array[10];
string path = PRMDIR + "baremin.ins";
NumberGen ng;
//Decoder d (path);
//Population p (200, 100, 10, &d, &ng);
Population p (200, 100, 10, path, &ng);
string str;
for(int i=0;i<200;++i)
{
	str = p.print_individual(i);
	cout<<i<<" ------------------------------------"<<endl;
	cout<<str<<endl;
	cout<<"------------------------------------"<<endl;
	
}
for(int i=0;i<200;++i) if (p.decoder->validate_instruction(p.population[i]) == false) cout<<"There are invalid instructions"<<endl;
printf("instruction mask = Ox%.8x unused= %d size=%d\n",p.decoder->INS.mask, p.decoder->INS.unused, p.decoder->INS.size);
/*
//////////////////  XO CHECK ////////////////////////
p.pagebasedXO(199, 99, 198, 99);
str = p.print_individual(198);
cout<<"198 xo------------------------------------"<<endl;
cout<<str<<endl;
cout<<"*------------------------------------"<<endl; 
str = p.print_individual(199);
cout<<"199 xo ------------------------------------"<<endl;
cout<<str<<endl;
cout<<"*------------------------------------"<<endl; 
*/
 
/*
for (int i=0;i<d.OPR_len;++i)
{
	cout<<"---- "<<d.lookup.at(i)<<" ----"<<endl;
	d.OPR[i].print();
}
cout<<"---- Register Lookup ----"<<endl;
for (int i=0;i<d.lookupR.size(); ++i) cout<<i<<":"<<d.lookupR.at(i)<<endl;
cout<<"---- Immediate Lookup ----"<<endl;
for (int i=0;i<d.lookupI.size(); ++i) cout<<i<<":"<<d.lookupI.at(i)<<endl;	
cout<<"Size="<<sizeof(BYTE2)<<endl;
printf("For %d instructions, mask is 0x%.8x and unused is %d.\n", d.INS_len, d.INS_mask, d.INS_unused);
printf("For %d registers   , mask is 0x%.8x and unused is %d.\n", d.REG_len, d.REG_mask, d.REG_unused);
printf("For %d immediates  , mask is 0x%.8x and unused is %d.\n", d.IMM_len, d.IMM_mask, d.IMM_unused);

cout<<"Array initialized"<<endl;
*/
//////// rANDOM NO STUFF
/*
unsigned long bitseed = (unsigned long) time(NULL);
long pr = 0;
for(int i=0;i<10;++i) array[i] = 0;
for (int j=0;j<10000;j++)
{
cout<<"iter "<<j<<endl;
long seed = time(NULL);

for(int i=0; i< 100000; ++i)
{
	
	//cout<<"test1"<<endl;
	//float rf = ran3(&seed);
	//cout<<"rf="<<rf<<endl;
	//if  ( (rf > 1) || (rf < 0) ) cout<<"Invalid rf "<<rf<<endl;
	//int rn = (int) floor(rf*10);
	//cout<<rn<<endl;
	//if (rn != 10) ++array[rn];
	//cout<<"Bit="<<irbit2(&bitseed)<<endl;
	int s = uniform_intgen(4, 7, &seed);
	if (s < 4 || s > 7) { cout<<"--------------------------------Oops!:"<<s<<endl; pr++; }
	array[s]++;
}
}
cout<<"::::::::::::::::::::::::::::"<<endl;
for(int i=0;i<10;++i) cout<<array[i]<<endl;
cout<<"::::::::::::::::::::::::::::"<<endl;
cout<<"pr="<<pr<<endl;
/*
long seed = time(NULL);
for (int i=0;i<1000;++i)
{
	float rf = ran3(&seed);
	if  ( (rf > 1) || (rf < 0) ) cout<<"Invalid rf "<<rf<<endl; 
	//int s = uniform_intgen(4, 7, &seed);
	//if (s < 4 || s > 7) cout<<"--------------------------------Oops!:"<<s<<endl;
}
*/
return 0;
}

