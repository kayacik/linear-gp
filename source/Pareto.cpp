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

#include "Pareto.hpp"
/***********************************************************
 *
 */
 Pareto::Pareto(double *obj1, double *obj2, double *obj3, int obj_size)
 {
	objective1 = obj1;
	objective2 = obj2;
	objective3 = obj3;
	size = obj_size
 };

/***********************************************************
 *
 */ 
  Pareto::Pareto()
 {
	size = 0;
 };


/***********************************************************
 *
 */ 
  Pareto::~Pareto()
 {
	objective1 = NULL;
	objective2 = NULL;
	objective3 = NULL;
	size = 0;
 };
 
 /***********************************************************
 *
 */
 void Pareto::rank()
 {
	// implemented from Deb's NSGA2 paper
	vector<int> S[pop->popsize]; //Set of solutions dominated by i
	vector<int> Front; //Current front
	vector<int> Q; // Next front
	int N[pop->popsize]; //domination counter
		
	for(int i=0; i<pop->popsize; ++i)
	{
		N[i] = 0;
		for(int j=0; j<pop->popsize; ++j)
		{
			if(A_dominates_B(i, j) == 1)
				S[i].push_back(j);
			else if (A_dominates_B(j, i) == 1)
				N[i]++;
		}
		if (N[i] == 0)
		{
			pop->rank[i] = 1;
			Front.push_back(i);
		}	
	}
	int index = 1; // i = 1 in paper
	while (!Front.empty())
	{
		Q.clear();
		for(int i=0; i<Front.size(); ++i)
		{
			for(int j=0; j<S[Front.at(i)].size(); ++j) // p = Front.at(i)
			{
				N[S[Front.at(i)].at(j)]--; //q = S[Front.at(i)].at(j)
				if (N[S[Front.at(i)].at(j)] == 0)
				{
					pop->rank[S[Front.at(i)].at(j)] = index + 1;
					Q.push_back(S[Front.at(i)].at(j));
				}
			}
		}
		index ++;
		Front = Q;
	}
 };
 
/***********************************************************
 *
 */
 int Pareto::A_dominates_B(int indA, int indB)
 {
	// You have to know where the sub-ojectives are
	// in asmGP they are size of ind, success and anomrate
	// kind of hard coded for now I might generalize this later
	// 1: A dominates B, -1: indifferent, 0: otherwise
	// Algorithm implemented from Andy's thesis
	int flag = 0;
	if ( objective1[indA] <= objective1[indB] ) // length: smaller better
	{
		flag = flag + (objective1[indA] < objective1[indB]);
		if ( pop->success[indA] >= pop->success[indB] ) // success: larger better
		{
			flag = flag + ( pop->success[indA] > pop->success[indB] );
			if ( pop->anomaly[indA] <= pop->anomaly[indB] ) // anomaly: smaller better
			{
				flag = flag + ( pop->anomaly[indA] < pop->anomaly[indB] );
			}
			else { return 0; }
		}
		else { return 0; }		
	}
	else { return 0; } 
	if (flag > 0) // A dominates B
		return 1;
	else // A and be are equal in all objectives
		return -1;
 };