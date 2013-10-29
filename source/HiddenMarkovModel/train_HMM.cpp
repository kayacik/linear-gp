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
#include <float.h>
#include <vector>
#include "../Functions.hpp"
// Kernel 2.6 has less than 300 syscalls, 400 is therefore a conservative number
#define SYSCALL_COUNT 300
int main(int argc, char **argv)
{
	if (argc != 4)
	{		//                        1            	2            3          4	 	
		cout<<"Usage: "<<argv[0]<<"  <input normal.db (Use N/A to start without file)> <output normal.db> <trace>"<<endl;
		//if ( (argc == 2) && (string(argv[1]) == "-h") ) documentation();
		exit(1);
	}
	string in_db_file = argv[1];
	string out_db_file = argv[2];
	string trace = argv[3];
	vector<int> sliding_win;
	// we don't know how many system calls the application makes so start from a large number (like 400)
	//int normalDB[SYSCALL_COUNT][SYSCALL_COUNT][win_size];
	//for (int i=0;i<SYSCALL_COUNT; ++i)
	//	for (int j=0;j<SYSCALL_COUNT; ++j)
	//		for (int k=0;k<win_size; ++k)
	//			normalDB[i][j][k] = 0;
	// read in the existing normal db (if provided)
	int **normalDB;
	normalDB = new int *[SYSCALL_COUNT];
	for (int i=0;i<SYSCALL_COUNT; ++i)
	{
		normalDB[i] = new int [SYSCALL_COUNT];
		for (int j=0;j<SYSCALL_COUNT; ++j)
		{
			normalDB[i][j] = 0;
		}
	}

	if (in_db_file.find("N/A") == string::npos)
	{
		// read the normal db into a 2d array observing the max of last column (i.e. window_size)
		ifstream fin(in_db_file.c_str());
		assert(fin);
		char ch;
		string line = "";
		string anomaly;
		int i = 0;
		int j = 0;
		while (fin.get(ch))
		{
			if( ch !='\n') 
				line+= ch;
			else // process the line
			{
				j = 0;
				int idx1 = 0;
				int idx2 = line.find(",", idx1 + 1);
				while (idx2 != string::npos)
				{
					string part = line.substr(idx1, idx2 - idx1);
					normalDB[i][j] = atoi(part.c_str());
					idx1 = idx2 + 2;
					idx2 = line.find(",", idx1 + 1);
					j++;
				}
				// there is still one more
				string part = line.substr(idx1);
				normalDB[i][j] = atoi(part.c_str());
				line = "";
				i++;
			}
		
		}
		fin.close();

	

	}

	// Start working on the trace
	ifstream fin(trace.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int syscall_curr = -1;
	int syscall_past = -1;
	while (fin.get(ch))
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			int idx = line.find(" ");
			string part1 = line.substr(0, idx);
			string part2 = line.substr(idx + 1);
			int process_no = atoi(part1.c_str());
			int syscall = atoi(part2.c_str());			
			//cout<<process_no<<"---"<<syscall<<endl;
			syscall_curr = syscall;
			
			if (syscall_past != -1)
			{
				normalDB[syscall_past][syscall_curr]++;
			} 
			syscall_past = syscall_curr;
			line = "";
		}
	}
	fin.close();
	// we have the normal db, write it to a file
	ofstream fout(out_db_file.c_str());
	for (int i=0;i<SYSCALL_COUNT; ++i)
	{
		for (int j=0;j<SYSCALL_COUNT; ++j)
		{
			if (j !=0) fout<<", ";
			fout<<normalDB[i][j];
		
		}
		fout<<endl;
	}
	fout.close();
	//deallocate array
	for (int i=0;i<SYSCALL_COUNT; ++i)
	{
		delete[] normalDB[i];
	}
	delete[] normalDB;
	
}
