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
#include <iomanip>
using namespace std;
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int read_matrix (double *matrix_ptr, string filename, int rows, int cols)
{
	ifstream fin(filename.c_str());
	assert(fin);
	char ch;
	string line = "";
	int count = 0;
	while (fin.get(ch))
	{
		if( (ch !='\n') && (ch != ' ') ) 
			line+= ch;
		else // process the token
		{
			double token = -1;
			if (line != "")
			{
				token = atof(line.c_str());
				matrix_ptr[count] = token;
				++ count;
				//cout<<count-1<<":"<<line<<" "<<token<<endl;
			}
	
			line = "";
		}
	}
	fin.close();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int matrix_multiplication(double *result, double *M1, int M1_rows, int M1_cols, double *M2, int M2_rows, int M2_cols)
{
	//assumes result matrix is allocated with M1_rows x M2_cols
	if (M1_cols != M2_rows)
	{
		cout<<"Can't multiply a "<<M1_rows<<" x "<<M1_cols<<" matrix with a "<<M2_rows<<" x "<<M2_cols<<" matrix."<<endl;
		return -1;
	}
	for(int i=0; i<M1_rows; ++i)
	{
		for(int j=0; j<M2_cols; ++j)
		{
			double sum = 0;
			for(int k=0; k<M2_rows; ++k) // could have used M1_cols
			{
				sum += (M1[i * M1_cols + k] * M2[k * M2_cols + j]);
			}
			result[i * M2_cols + j] = sum;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int matrix_addition(double *result, double *M1, int M1_rows, int M1_cols, double *M2, int M2_rows, int M2_cols)
{
	//assumes result matrix is allocated with M1_cols x M1_rows = M2_cols x M2_rows 
	for(int i=0; i<M1_rows; ++i)
	{
		for(int j=0; j<M1_cols; ++j)
		{
			result[i * M1_cols + j] = M1[i * M1_cols + j] + M2[i * M1_cols + j];
		}
	
	}
	return 0;
}

double tansig(double x)
{
	return (2/(1+exp(-2*x))) - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int matrix_tansig(double *result, double *M1, int M1_rows, int M1_cols)
{
	//assumes result matrix is allocated with M1_cols x M1_rows
	for(int i=0; i<M1_rows; ++i)
	{
		for(int j=0; j<M1_cols; ++j)
		{
			result[i * M1_cols + j] = tansig(M1[i * M1_cols + j]);
		}
	}	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int write_matrix(string filename, double *M1, int M1_rows, int M1_cols)
{
	FILE *fp;
	fp = fopen(filename.c_str(), "w");
	if (fp == NULL) 
	{
		cout<<"Can't open input file!\n";
		return -1;
	}
	int count = 0;
	for(int i = 0; i< M1_rows * M1_cols; ++i)
	{
		fprintf(fp, "   %.70e", M1[i]);
		count++;
		if(count == M1_cols)
		{
			fprintf(fp, "\n");
			count = 0;
		}
	}
	fclose(fp);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void euclidean_distance(double *distance, double *max_dist, double *V1, double *V2, int length)
{
	*distance = 0;
	*max_dist = 0;
	double value = 0;
	for (int i=0; i<length; ++i)
	{
		value = (V1[i] - V2[i])*(V1[i] - V2[i]);
		*distance += value;
		if (*max_dist < value)
			*max_dist = value;
	}
	*distance = (double) sqrt ((double)*distance);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int main (int argc, char **argv)
{
	if (argc != 8)
	{		//                        1     2    3    4        5              6        7        
		cout<<"Usage: "<<argv[0]<<"  <W1> <W2> <B1> <B2> <#inputs/outputs> <#hidden> <trace>"<<endl;
		//if ( (argc == 2) && (string(argv[1]) == "-h") ) documentation();
		exit(1);
	}
	string W1_file = argv[1];
	string W2_file = argv[2];
	string B1_file = argv[3];
	string B2_file = argv[4];
	int output_count = atoi(argv[5]);
	int hidden_count = atoi(argv[6]);
	string trace_file = argv[7];
	
	double IW[hidden_count][output_count];   assert(IW);
	double LW[output_count][hidden_count];	 assert(LW);
	double B1[hidden_count];				 assert(B1);
	double B2[output_count];				 assert(B2);
	double P [output_count];				 assert(P);
	double W1P[hidden_count];				 assert(W1P);
	double W1PplusB1[hidden_count];			 assert(W1PplusB1);
	double tansigW1PplusB1[hidden_count];	 assert(tansigW1PplusB1);
	double W2P2[output_count];				 assert(W2P2);
	double W2P2plusB2[output_count];		 assert(W2P2plusB2);
	
	read_matrix(&IW[0][0], W1_file, hidden_count, output_count);
	read_matrix(&LW[0][0], W2_file, output_count, hidden_count);
	read_matrix(&B1[0], B1_file, hidden_count, 1);
	read_matrix(&B2[0], B2_file, output_count, 1);
	read_matrix(&P[0], trace_file, output_count, 1);
	
	matrix_multiplication(&W1P[0], &IW[0][0], hidden_count, output_count, &P[0], output_count, 1); // calculate Wp
	matrix_addition(&W1PplusB1[0], &W1P[0], hidden_count, 1, &B1[0], hidden_count, 1 ); // calculate Wp +B1
	matrix_tansig(&tansigW1PplusB1[0], &W1PplusB1[0], hidden_count, 1); // calculate f(Wp +B1)
	matrix_multiplication(&W2P2[0], &LW[0][0], output_count, hidden_count, &tansigW1PplusB1[0], hidden_count, 1); // calculate W2*(f(Wp +B1))
	matrix_addition(&W2P2plusB2[0], &W2P2[0], output_count, 1, &B2[0], output_count, 1);
	double dist = 0;
	double max_dist = 0;
	euclidean_distance(&dist, &max_dist, &P[0], &W2P2plusB2[0], output_count );
	// print the outout in cout
	cout<<"# distance, max_dist_component"<<endl;
	cout<<dist<<","<<max_dist<<endl;
	/* Save the cout flags. */
	ios_base::fmtflags originalFormat = cout.flags();
	/* Set the flags as needed. */ 
	cout << showpoint << fixed << setprecision(30);


	for (int i=0; i<output_count; ++i)
	{
		cout<<"   "<<W2P2plusB2[i];
	}
	cout<<endl;
	/* Reset the cout flags. */
	cout.flags(originalFormat); 
	//write_matrix("output.dat", &W2P2plusB2[0], output_count, 1);

}


