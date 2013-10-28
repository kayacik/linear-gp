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
#include <stdlib.h>
/////////// Timimg: Preamble Code
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <time.h>
#define CPUTIME (getrusage(RUSAGE_SELF,&ruse),\
  ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec + \
  1e-6 * (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec))
struct rusage ruse;
extern int getrusage();
///////////

int main(int argc, char **argv)
{
string filename;
if (argc == 2) 
	filename = argv[1]; 
else {
		cout<<"Post training analysis of the population.\nUsage: "<<argv[0]<<" popX.dat"<<endl;
		return 1;} 
/**************************************
 * Set Initial Timer                  *
/**************************************/
double t0, t1;
time_t u1,u2;
time(&u1);
t0 = CPUTIME;
/*******************************************************************
 * Analysis Starts Here											 
/*******************************************************************/
int pop_size = 500;
int page_count = 10;
int page_size = 3;
int seed = 1; // DO NOT USE RANDOM NUMBERS IN THIS PROGRAM
FILE *fp;
string path = PRMDIR + "asm_barebones.inst";
NumberGen numgen (seed);
Population myPop (pop_size, page_count, page_size, path, &numgen);
// Now copy the binary file to the population
fp = fopen ( filename.c_str() , "rb" );
if (fp == NULL) {cout<<"Non existent file\n"; exit (1); }
// obtain file size.
fseek (fp , 0 , SEEK_END);
long lSize = ftell (fp);
rewind (fp);
cout<<"File is "<<lSize<<" bytes long which is about "<<lSize/4<<" instructions."<<endl;
if (pop_size*page_count*page_size != lSize/4) { cout<<"Population size does not match the file size"<<endl; exit(1);}
// copy the file into the buffer.
fread (myPop.population,4,lSize/4,fp);
///////// Actual Attack From Book /////////
string master[11];
master[0] = "XOR EAX, EAX";
master[1] = "CDQ";
master[2] = "PUSH EAX";
master[3] = "PUSH 0x68732f2f";
master[4] = "PUSH 0x6e69622f";
master[5] = "MOV EBX, ESP";
master[6] = "Placeholder for PuSh EaX";
master[7] = "PUSH EBX";
master[8] = "MOV ECX, ESP";
master[9] = "MOV AL, 0x0b";
master[10] ="INT 0x80"; 
///////////////////////////////////////////
double fit = 0;
for (int i=0; i<pop_size; ++i)
//for (int i=0; i<1; ++i)
{
	fit = myPop.calculateFitness(i);
	string prog0, prog1, prog2, save4later;
	if (fit >= 10) // Perform analysis only on succesfull individuals
	{
		
		
		//////////   Analysis 1. Placement of Introns   //////////
		prog0 = myPop.print_individual(i);
		save4later = "BITS 32\n" + prog0;
		// If you want to print ind to a file now is the time.
		// *** This is a temporary solution, need to fix linux bugs and 
		// remove the following blurb
		string fName = argv[1];
		fName += ".prog" + my_itoa(i);
		cout<<fName<<endl;
		ofstream fout(fName.c_str());
		assert(fout);
		fout<<save4later.c_str();
		fout.close();
		// end of blurb
		prog1 = "";
		prog2 = "";
		string instructions[page_count * page_size];
		int isIntron[page_count * page_size];
		for (int j = 0; j<page_count * page_size; ++j)
		{
			isIntron[j]=0;
			int idx = prog0.find('\n');
			prog1 = prog0.substr(idx + 1, prog0.length()-idx-1);
			prog2 = prog0.substr(0, idx);
			instructions[j] = prog2;
			prog0 = prog1;
			//cout<<"instructions["<<j<<"]="<<instructions[j]<<endl;  
		}  
		// Here we have the instruction array prepared
		string newprog;
		double npfit;
		for (int j = 0; j<page_count * page_size; ++j)
		{
			newprog = "";
			npfit = 0;
			for(int k = 0; k < j; ++k)
			{
				if (isIntron[k] == 0) { newprog +=instructions[k]; newprog += "\n"; }
			}
			
			for(int k = j + 1; k < page_count * page_size; ++k)
			{
				newprog +=instructions[k]; newprog += "\n";
			}
			// newprog has the program;
			RuntimeFitness r (newprog);
			npfit = r.fitness();
			if (npfit >= 10) isIntron[j] = 1;
			//cout<<"isIntron["<<j<<"]="<<isIntron[j]<<endl;
			// isIntron contains the raw data here 
		}
		//////////   Analysis 2. Number of different instructions   //////////
		int match[page_count * page_size];
		int matched = 0;
		for (int j = 0; j<page_count * page_size; ++j)
		{
			match[j] = -1;
			if (isIntron[j] == 0)
			{
				matched = 0;
				for(int k = 0; k < 11; ++k)
				{
					if (instructions[j].find(master[k]) != string::npos) 
						{ 
							match[j] = k; 
							++matched;
							if (matched > 1) cout<<"Individual "<<i<<" multiple matches. ("<<instructions[j]<<")"<<endl;
						}
				}
			}
		// Match contains the raw data here
		//cout<<"match["<<j<<"]="<<match[j]<<endl; 
		}
	//////////// GET THE FINAL RESULTS FROM ARRAYS ///////////
	// 1 //
	int min = -1; int min_found = 0;
	int max = -1;
	int diff_count = 0;
	int exon_count = 0;
	int mic[5]; // master prg. instruction counts [0] XOR, [1] CDQ, [2] PUSH, [3] MOV, [4] INT
	for (int x = 0; x < 5; ++x) mic[x] = 0;
	for (int j = 0; j<page_count * page_size; ++j)
	{
		if (isIntron[j] == 0)
		{
			++exon_count;
			if (!min_found) { min = j; min_found = 1;}
			max = j;
			if (match[j] == -1) diff_count++;
			if (instructions[j].find("XOR") != string::npos) mic[0]++;
			else if  (instructions[j].find("CDQ") != string::npos) mic[1]++;
			else if  (instructions[j].find("PUSH") != string::npos) mic[2]++;
			else if  (instructions[j].find("MOV") != string::npos) mic[3]++;
			else if  (instructions[j].find("INT") != string::npos) mic[4]++;
		} 
	}		
	int section = max - min;
	int instruction_wise = abs(mic[0] - 1) + abs(mic[1] - 1) + abs(mic[2] - 5) + abs(mic[3] - 3) + abs(mic[4] - 1); 	
	//////////// REPORTING TO STDOUT ////////////
	cout<<"Program"<<i<<", ";
	cout<<min<<", "; // Location of the first exon (break point)
	cout<<section<<", "; // In terms of # instructions, dist between the first and the last exon
	cout<<exon_count<<", "; // #exons
	cout<<instruction_wise<<", "; // number of diff instruction types (params ignored) compared with the master program
	cout<<diff_count<<endl;	// number of diff instructions (params considered) compared with the master program
	//cout<<save4later.c_str()<<endl; // The program
	}
}

fclose(fp);
/**************************************
 * Set Timer Again and Report         *
/**************************************/
t1 = CPUTIME;
time(&u2);
cout<<"CPU time = "<< t1-t0 << " secs.\nUser Time = "<<(int)(u2-u1)<<" secs.\n";
return 0;
}


