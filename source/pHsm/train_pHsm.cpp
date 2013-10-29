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
#define SYSCALL_COUNT 400
int main(int argc, char **argv)
{
	if (argc != 6)
	{		//                        1            	2            3          4	 	5 6
		cout<<"Usage: "<<argv[0]<<"  <win_size> <input normal.db (Use N/A to start without file)> <output normal.db> <trace> <schema_file>"<<endl;
		//if ( (argc == 2) && (string(argv[1]) == "-h") ) documentation();
		exit(1);
	}
	int win_size = atoi(argv[1]);
	string in_db_file = argv[2];
	string out_db_file = argv[3];
	string trace = argv[4];
	string schema_file = argv[5];
	vector<int> sliding_win;  // this is the regular sliding win that we use in pH
	vector<int> sliding_win2; // this is the resulting pattern
	vector<int> schema;
	// read in the schema file
	ifstream fin2(schema_file.c_str());
	assert(fin2);
	char ch2;
	string line = "";
	int numr;
	int max_schema_loc = 0;
	while (fin2.get(ch2))
	{
		if( ch2 !='\n') 
			line+= ch2;
		else // process the line
		{
			int idx1 = -1;
			int idx2 = line.find(",");
			while (idx2 != string::npos)
			{
				string str = line.substr(idx1 + 1, idx2 - idx1 - 1);
				numr = atoi(str.c_str());
				if (numr > max_schema_loc) max_schema_loc = numr;
				schema.push_back(numr);
				idx1 = idx2;
				idx2 = line.find(",", idx1 + 1);
			}
			string str = line.substr(idx1 + 1);
			numr = atoi(str.c_str());
			if (numr > max_schema_loc) max_schema_loc = numr;
			schema.push_back(numr);
			break; // once the first line is finished
		}
	}
	fin2.close();
	//for (int i=0;i<schema.size();++i)
	//{
	//	cout<<schema.at(i)<<endl;
	//}
	
	if(max_schema_loc >= win_size)
	{
		cout<<"Error: Maximum schema location ("<<max_schema_loc<<") can't be larger than the window size ("<<win_size<<"). exitting..."<<endl;
		exit(1);
	}
	
	
	
	// we don't know how many system calls the application makes so start from a large number (like 400)
	//int normalDB[SYSCALL_COUNT][SYSCALL_COUNT][win_size];
	//for (int i=0;i<SYSCALL_COUNT; ++i)
	//	for (int j=0;j<SYSCALL_COUNT; ++j)
	//		for (int k=0;k<win_size; ++k)
	//			normalDB[i][j][k] = 0;
	// read in the existing normal db (if provided)
	int ***normalDB;
	normalDB = new int **[SYSCALL_COUNT];
	for (int i=0;i<SYSCALL_COUNT; ++i)
	{
		normalDB[i] = new int *[SYSCALL_COUNT];
		for (int j=0;j<SYSCALL_COUNT; ++j)
		{
			normalDB[i][j] = new int[win_size]; // no need to init over winsize (now we have schema len) but leave it like this for now
		}
	}
	
	if (in_db_file.find("N/A") == string::npos)
	{
		vector<int> col1;
		vector<int> col2;
		vector<int> col3;
		// read the normal db into a 2d array observing the max of last column (i.e. window_size)
		ifstream fin(in_db_file.c_str());
		assert(fin);
		char ch;
		line = "";
		string anomaly;
		while (fin.get(ch))
		{
			if( ch !='\n') 
				line+= ch;
			else // process the line
			{
				int idx1 = line.find(",");
				int idx2 = line.find(",", idx1 + 1);
				string part1 = line.substr(0, idx1);
				string part2 = line.substr(idx1 + 1, idx2-idx1-1);
				string part3 = line.substr(idx2 + 1);
				int p1 = atoi(part1.c_str());
				int p2 = atoi(part2.c_str());
				int p3 = atoi(part3.c_str());				
				col1.push_back(p1);
				col2.push_back(p2);
				col3.push_back(p3);
				//cout<<p1<<"---"<<p2<<"---"<<p3<<endl;			
				line = "";
			}
		
		}
		fin.close();
		if( (col1.size() != col2.size()) || (col1.size() != col3.size()) )
		{
			cout<<"Col vectors are not properly initialized..."<<endl;
			exit(1);
		}
		else
		{
			for (int i=0; i<col1.size();++i)
			{
				int x = col1.at(i);
				int y = col2.at(i);
				int z = col3.at(i);
				normalDB[x][y][z] = 1;
			}
		}
	

	}
	// Start working on the trace
	ifstream fin(trace.c_str());
	assert(fin);
	char ch;
	line = "";
	string anomaly;
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
			sliding_win.push_back(syscall);
			// now we have win_size syscalls in sliding window where the vector end is the most recent
			if (sliding_win.size() == win_size)
			{	
				// create the sliding window2
				sliding_win2.clear();
				for (int i=0; i<schema.size(); ++i)
				{
					sliding_win2.push_back(sliding_win.at(schema.at(i)));
				}
				
				
				
				
				/*
				
				
				for (int i=0; i<sliding_win.size(); ++i)
				{
				
					cout<<sliding_win.at(i)<<" ";
				
				}
				cout<<endl;
				for (int i=0; i<sliding_win2.size(); ++i)
				{
				
					cout<<sliding_win2.at(i)<<" ";
				
				}
				cout<<endl;
				*/
				
				
				
				
				for (int i=0; i<sliding_win2.size() -1; ++i)
				{
					int x = sliding_win2.at(sliding_win2.size() - 1);
					int y = sliding_win2.at(i);
					int z = sliding_win2.size() - i - 1;
					normalDB[x][y][z] = 1;
				}
				
			}
			if (sliding_win.size() == win_size) sliding_win.erase(sliding_win.begin());
			//cout<<sliding_win.size()<<endl;
			line = "";
		}
	}
	fin.close();
	// we have the normal db, write it to a file
	ofstream fout(out_db_file.c_str());
	for (int i=0;i<SYSCALL_COUNT; ++i)
		for (int j=0;j<SYSCALL_COUNT; ++j)
			for (int k=0;k<win_size; ++k)
				if (normalDB[i][j][k] != 0)
				{
					fout<<i<<","<<j<<","<<k<<endl;
				}
	fout.close();
	//deallocate array
	for (int i=0;i<SYSCALL_COUNT; ++i)
	{
		for (int j=0;j<SYSCALL_COUNT; ++j)
		{
			 delete[] normalDB[i][j];
		}
		delete[] normalDB[i];
	}
	delete[] normalDB;
	
}
