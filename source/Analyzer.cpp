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

void save_attacks(string orig_prefix, string new_prefix)
{
	string command1 = "cp "+orig_prefix+".processed  "+new_prefix+".exploit";
	string command2 = "cp "+orig_prefix+".processed2  "+new_prefix+".attack";
	cout<<command1<<endl;
	system(command1.c_str());
	cout<<command2<<endl;
	system(command2.c_str());
}

;

int main(int argc, char **argv)
{
	if (argc != 5)
	{		//                        1            2            3           4                         
		cout<<"Usage: "<<argv[0]<<"<pop file> <attack number> <detector> <app_prefix>"<<endl;
		exit(1);
	}
	
	// Basic Population initializations, do the BARE MINIMUM
	Population myPop;
	myPop.app_prefix = argv[4];
	// read in the vector
	ifstream fin2 (string("Stide/"+myPop.app_prefix+".enum").c_str());
	string line2="";
	char ch2;
	while (fin2.get(ch2))
	{
		if(( ch2 !='\n') && ( ch2 !='\r'))
			line2+= ch2;
		else // process the line
		{
			myPop.syscalls.push_back(line2);
			line2 = "";
		}
	}
	fin2.close();
	
	// Other Parameters
	string det_prefix = argv[3];
	string second_arg = argv[2];
	int attack_number = atoi(argv[2]);
	
	// DB Locs
	string stide_case1 = "/Volumes/Disk2/DetectorDBs/Stide/DBs/traceroute/case1.db";
	string stide_case2 = "/Volumes/Disk2/DetectorDBs/Stide/DBs/traceroute/case2.db";
	string stide_case3 = "/Volumes/Disk2/DetectorDBs/Stide/DBs/traceroute/case3.db";
	string stide_case4 = "/Volumes/Disk2/DetectorDBs/Stide/DBs/traceroute/case4.db";
	string stide_case5 = "/Volumes/Disk2/DetectorDBs/Stide/DBs/traceroute/case5.db";
	string stide_caseX = "/Volumes/Disk2/DetectorDBs/Stide/DBs/traceroute/traceroute.db";
	
	// Read in the file
	vector<double> v_id;
	vector<double> v_fit;
	vector<double> v_anom;
	vector<double> v_ranom;
	vector<double> v_rank;
	vector<double> v_succ;
	vector<double> v_len;
	vector<double> v_delay;
	fstream fin (argv[1]);
	double min_anom = 100;
	int min_anom_loc = -1;
	assert(fin);
	string line="";
	char ch;
	int flag = 0;
	while (fin.get(ch))
	{
		if(( ch !='\n') && ( ch !='\r'))
				line+= ch;
		else // process the line
		{
			if ((flag == 1) && (line.find("***") != string::npos) )
			{
				// that's it we read in what we needed to
				break;
			}    
			if (flag == 1)
			{
				// process the line
				int idx1 = 0;
				int idx2 = line.find("\t");
				string indx = string_trim(line.substr(idx1, idx2 - idx1));
				double part2 = atof(indx.c_str());
				double part1 = 0;
				v_id.push_back((part2/(double)1000) + part1); // this index is not correct in this context
				int idx3 = line.find("\t", idx2 + 1);
				string fit = string_trim(line.substr(idx2 + 1, idx3 - idx2 - 1));
				v_fit.push_back(atof(fit.c_str()));
				int idx4 = line.find("\t", idx3 + 1);
				string anom = string_trim(line.substr(idx3 + 1, idx4 - idx3 - 1));
				v_anom.push_back(atof(anom.c_str()));
				if (atof(anom.c_str()) < min_anom)
				{
					min_anom = atof(anom.c_str());
					min_anom_loc = v_anom.size() - 1; 
				}
				int idx5 = line.find("\t", idx4 + 1);
				string ranom = string_trim(line.substr(idx4 + 1, idx5 - idx4 - 1));
				v_ranom.push_back(atof(ranom.c_str()));
				int idx6 = line.find("\t", idx5 + 1);
				string rank = string_trim(line.substr(idx5 + 1, idx6 - idx5 - 1));
				v_rank.push_back(atof(rank.c_str()));
				int idx7 = line.find("\t", idx6 + 1);
				string succ = string_trim(line.substr(idx6 + 1, idx7 - idx6 - 1));
				v_succ.push_back(atof(succ.c_str()));
				int idx8 = line.find("\t", idx7 + 1);
				string len = string_trim(line.substr(idx7 + 1, idx8 - idx7 - 1));
				v_len.push_back(atof(len.c_str()));
				int idx9 = line.find("\t", idx8 + 1);
				string delay = string_trim(line.substr(idx8 + 1, idx9 - idx8 - 1));
				v_delay.push_back(atof(delay.c_str()));
			}
			if ( (line.find("fitness") != string::npos) && (line.find("anomaly") != string::npos) && (line.find("raw_anomaly") != string::npos) && (line.find("rank") != string::npos) )
			{
				flag++;
			}
			line = "";
			//cout<<flag<<endl;
		}
	}
	// this is where we start to read in the attacks
	flag = 0;
	string attack="";
	vector<string> attacks;
	int attack_count = -1;
	/////////// variables that we want to see
	double anomrate;
	double delay;
	/////////////////////////////////////////
	if (string(argv[2]) == "best")
	{
		attack_number = min_anom_loc;
	}
	
	/////////////////////////////////////////
	int process_line = 0;
	while (fin.get(ch))
	{
		if(( ch !='\n') && ( ch !='\r'))
				line+= ch;
		else // process the line
		{
			if(line.find("***** [") != string::npos)
			{
				attack_count++;
				process_line = 1;
				line = "";
			}
			else if(line == "")
			{
				if( ( attack != "") && (attack_number == attack_count) )
				{	
					cout<<"Displaying attack number "<<attack_number<<"."<<endl;
					/////////////////////////////////////////
					/////// Process the attack here /////////
					/////////////////////////////////////////
					double anomrate1;
					double anomrate0;
					double delay;
					double success;
					string line1 = "";
					string line2 = "";
					if (det_prefix == "Stide")
					{
						cout<<"------- Attack Arates -------"<<endl;
						myPop.calc_fitness(attack, stide_case1, anomrate1, success,  1); // anom rate with preamble
						cout<<anomrate1<<endl;
						myPop.calc_fitness(attack, stide_case2, anomrate1, success,  1); // anom rate with preamble
						cout<<anomrate1<<endl;
						myPop.calc_fitness(attack, stide_case3, anomrate1, success,  1); // anom rate with preamble
						cout<<anomrate1<<endl;
						myPop.calc_fitness(attack, stide_case4, anomrate1, success,  1); // anom rate with preamble
						cout<<anomrate1<<endl;
						myPop.calc_fitness(attack, stide_case5, anomrate1, success,  1); // anom rate with preamble
						cout<<anomrate1<<endl;
						myPop.calc_fitness(attack, stide_caseX, anomrate1, success,  1); // anom rate with preamble
						cout<<anomrate1<<endl;
						cout<<"------- Exploit Arates -------"<<endl;
						myPop.calc_fitness(attack, stide_case1, anomrate0, success,  0); // anom rate without preamble
						cout<<anomrate0<<endl;
						myPop.calc_fitness(attack, stide_case2, anomrate0, success,  0); // anom rate without preamble
						cout<<anomrate0<<endl;	
						myPop.calc_fitness(attack, stide_case3, anomrate0, success,  0); // anom rate without preamble
						cout<<anomrate0<<endl;					
						myPop.calc_fitness(attack, stide_case4, anomrate0, success,  0); // anom rate without preamble
						cout<<anomrate0<<endl;
						myPop.calc_fitness(attack, stide_case5, anomrate0, success,  0); // anom rate without preamble
						cout<<anomrate0<<endl;					
						myPop.calc_fitness(attack, stide_caseX, anomrate0, success,  0); // anom rate without preamble
						cout<<anomrate0<<endl;
						string orig_loc = "Stide/analyzer-run";
						string new_loc = argv[1];
						save_attacks(orig_loc, new_loc);
					}
	
				}
				attack ="";
				process_line = 0;
			}
			
			if (process_line)
			{
				attack +=line;
				attack +="\n";
			}
			line ="";
		}
	}
	fin.close();
	cout<<"----- Deallocation of unallocated errors below are normal -----"<<endl;
	

	return 0;
}
