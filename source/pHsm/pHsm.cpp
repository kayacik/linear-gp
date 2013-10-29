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
class Status
{
	public:
	int mismatch_count;
	int total_count;
	int process_tolerized;
	int process_sensitized;
	vector<int> lfcs; // keep track of lfcs
	vector<int> mismatches; // number of mismatches
	vector<double> delays; // projected delay. recorded throughout the trace
	vector<int> stop_execve; // 1 yes / 0 no. recorded throughout the trace
	vector<int> tolerized; // 1 yes / 0 no. recorded throughout the trace
	vector<int> sensitized; // 1 yes / 0 no. recorded throughout the trace
	int win_size;
	int delay_factor;
	int tol_limit;
	int anom_limit;
	int suspend_execve;
	int suspend_execve_time;
	int lfc;
	
	
	Status(int p_win_size, int p_delay_factor, int p_tol_limit, int p_anom_limit, int p_suspend_execve, int p_suspend_execve_time, int p_lfc)
	{
		mismatch_count = 0;
		total_count = 0;
		process_tolerized = 0;
		process_sensitized = 0;
		win_size = p_win_size;
		delay_factor = p_delay_factor;
		tol_limit = p_tol_limit;
		anom_limit = p_anom_limit;
		suspend_execve = p_suspend_execve;
		suspend_execve_time = p_suspend_execve_time;
		lfc = p_lfc;
	
	};
	void update(vector<int> sliding_win, int ***normalDB)
	{
		
		total_count++;
		int anomalous = 0;
		for (int i=0; i<sliding_win.size() -1; ++i)
		{
			//cout<<"begin"<<endl;
			int x = sliding_win.at(sliding_win.size() - 1);
			//cout<<"begin2"<<endl;
			int y = sliding_win.at(i);
			//cout<<"begin3"<<endl;
			int z = sliding_win.size() - i - 1;
			//cout<<"begin4"<<endl;
			if (normalDB[x][y][z] == 0)
				anomalous = 1;
			//cout<<"end"<<endl;
		}
		// update mismatches
		mismatches.push_back(anomalous);
		if (anomalous) mismatch_count++;
		// calculate  LFC
		int cur_lfc = 0;
		for (int j=mismatches.size()-1; (j>=0) && (j>=mismatches.size() - lfc); --j)
		{
			cur_lfc +=mismatches.at(j);
		}
		lfcs.push_back(cur_lfc);
		// calculate delay
		double cur_delay = delay_factor * 0.01 * pow( (double) 2, cur_lfc );
		delays.push_back(cur_delay);
		// do we have enough mismatches to suspend execve
		if (mismatch_count > suspend_execve)
			stop_execve.push_back(1);
		else
			stop_execve.push_back(0);
		// will the process be tolerized?
		if (cur_lfc > tol_limit)
		{
			process_sensitized = 1; // okay it can't be tolerized now
			process_tolerized = 0;
		}
		sensitized.push_back(process_sensitized);
		if ((mismatch_count > anom_limit) && !process_sensitized)
			process_tolerized = 1;
		tolerized.push_back(process_tolerized);	
	};
	void output()
	{
		if ((mismatches.size() != delays.size()) || (mismatches.size() != stop_execve.size()) || (mismatches.size() != tolerized.size()) || (mismatches.size() != sensitized.size()) )
		{
			cout<<"Error: Output vectors are not the same size, there must be a problem with record keeping"<<endl;
			exit(1);
		}
		cout<<"# mismatch_ct, total_ct, total_delay, max_lfc, execve_stopped@, sensitized@, tolerized@"<<endl;
		cout<<mismatch_count<<","<<total_count<<",";
		double total_delay = 0;
		for(int i=0; i<delays.size(); ++i)
		{
			total_delay += delays.at(i);
		}
		cout<<total_delay<<",";
		int max_lfc = 0;
		for(int i=0; i<lfcs.size(); ++i)
		{
			if (lfcs.at(i) > max_lfc) max_lfc = lfcs.at(i); 
		}
		cout<<max_lfc<<",";
		int stopped_at = -1;
		for(int i=0; i<stop_execve.size(); ++i)
		{
			if ((stop_execve.at(i) == 1) && (stopped_at == -1) ) stopped_at = i;
		}
		cout<<stopped_at<<",";
		int sensitized_at = -1;
		for(int i=0; i<sensitized.size(); ++i)
		{
			if ((sensitized.at(i) == 1) && (sensitized_at == -1) ) sensitized_at = i;
		}
		cout<<sensitized_at<<",";
		int tolerized_at = -1;
		for(int i=0; i<tolerized.size(); ++i)
		{
			if ((tolerized.at(i) == 1) && (tolerized_at == -1) ) tolerized_at = i;
		}
		cout<<tolerized_at<<endl;
		// in the future dump the vectors
		
	}
};

int main(int argc, char **argv)
{
	if (argc != 11)
	{		//                 		0 		      1  		          	2    	        	3			          4				5					6						7					8							9
		cout<<"Usage: "<<argv[0]<<"  <normal.db> <trace> <win_size (20)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc> <schema_file>"<<endl;
		//if ( (argc == 2) && (string(argv[1]) == "-h") ) documentation();
		exit(1);
	}
	string db_file = argv[1];
	string trace_file = argv[2];
	int win_size = atoi(argv[3]);
	int delay_factor = atoi(argv[4]);
	int tol_limit = atoi(argv[5]);
	int anom_limit = atoi(argv[6]);
	int suspend_execve = atoi(argv[7]);
	int suspend_execve_time = atoi(argv[8]);
	int lfc = atoi(argv[9]);
	string schema_file = argv[10];
	Status status(win_size, delay_factor, tol_limit, anom_limit, suspend_execve, suspend_execve_time, lfc);
	vector<int> sliding_win;
	vector<int> sliding_win2; // this is the resulting pattern
	vector<int> schema;
	// read in the schema file
	ifstream fin3(schema_file.c_str());
	assert(fin3);
	char ch2;
	string line = "";
	int numr;
	int max_schema_loc = 0;
	while (fin3.get(ch2))
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
	fin3.close();

	
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
			normalDB[i][j] = new int[win_size];
		}
	}
	
	
	
	
	///////////////////////////////////////////
	///////////////////////////////////////////
	// Step 1. read in the  normal db 
	///////////////////////////////////////////
	vector<int> col1;
	vector<int> col2;
	vector<int> col3;
	// read the normal db into a 2d array observing the max of last column (i.e. window_size)
	ifstream fin(db_file.c_str());
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

	///////////////////////////////////////////
	///////////////////////////////////////////
	// Step 2. Start working on the trace
	///////////////////////////////////////////
	ifstream fin2(trace_file.c_str());
	assert(fin2);
	while (fin2.get(ch))
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
				//
				// This is where you'll take the sliding window and apply detection and update status
				// such as LFC delay and so on...
				// create the sliding window2
				sliding_win2.clear();
				//cout<<"sc="<<schema.size()<<endl;
				for (int i=0; i<schema.size(); ++i)
				{
					sliding_win2.push_back(sliding_win.at(schema.at(i)));
				}
				//cout<<"here!!111!"<<endl;
				//for (int i=0;i<sliding_win2.size();++i)
				//{
				//	cout<<"here!!!"<<endl;
				//	cout<<"- "<<sliding_win2.at(i)<<endl;
				//}
				
				status.update(sliding_win2, normalDB);
			}
			if (sliding_win.size() == win_size) sliding_win.erase(sliding_win.begin());
			//cout<<sliding_win.size()<<endl;
			line = "";
		}
	}
	fin2.close();
	///////////////////////////////////////////
	///////////////////////////////////////////
	// Step 3. Produce output based on results
	///////////////////////////////////////////
	//
	// Produce an output that GP will read in, nominally, produce 
	// # mismatches, anomaly rate, max LFC, projected delay, execve stopped
	// attack training happened? ... so on
	//
	status.output();
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
	cout<<status.mismatch_count<<" / "<<status.total_count<<endl;
}
