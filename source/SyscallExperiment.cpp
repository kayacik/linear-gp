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
void documentation();
#define NUM_ITER 100 // Was 1000 changed to 100 to have more visibility.
#define TERMINATE_COUNT 10 // Terminate if the Rank PDF remains the same over 10 consecutive tournaments.
#define MIN_ITERS_BEFORE_TERMINATE 5000 // Terminate if the Rank PDF remains the same (over ...) after 5000 tournaments
///////////////////////////////////////////////////////////////////
/// CHANGES FROM v2.X to 3.0
/// - Mainly the variable length population.
///
///
int main(int argc, char **argv)
{
if (argc != 20)
{		//                        1            2            3             4             5            6           7         8       9     10			 11			  12            13        14			  15		      16				17				18			19
	cout<<"Usage: "<<argv[0]<<"<pop size> <page count> <page length> <iterations> <fitness type> <range> <output file> <p_mut> <p_xo> <p_swp> <greedy(1/0)> <runsstartfrom> <#runs> <enable OWC(1/0)> <XO_type(1..N)> <mut_type(1..N)> <swp_type(1..N)> <app_prefix> <detector_prefix>"<<endl;
	cout<<"       "<<argv[0]<<" -h for more information."<<endl; 
	if ( (argc == 2) && (string(argv[1]) == "-h") ) documentation();
	exit(1);
}
//*************************************
// Set Initial Timer                  *
//*************************************
double t0, t1;
time_t u1,u2;
time(&u1);
t0 = CPUTIME;
//******************************************************************
// Experiment Run Starts Here									   *
//******************************************************************
configuration config; // this was supposed to contain all Population() parameters but I am not gonna do that...
// Todo: Need to overhaul the parameter parsing. I don't need all these... 
config.enable_owc = atoi(argv[14]);
config.xo_type = atoi(argv[15]);
config.mut_type = atoi(argv[16]);
config.swp_type = atoi(argv[17]);
string prefix = argv[18];
string detector = argv[19];
int run_start = atoi(argv[12]);
int run_count = atoi(argv[13]);
int do_greedysearch = atoi(argv[11]);
int individualPDFneeded = 0;
if ( (config.mut_type >6) && (config.mut_type <10)  ) individualPDFneeded = 1; // we need to recalculate ind PDFs
if ( (do_greedysearch != 0) && (do_greedysearch != 1) ) { cout<<"[ERROR] Greedy param can be either 1 (enable) or 0 (disable)."<<endl; exit(1); }
for (int runs = run_start; runs < run_count; ++runs) // was 20
{
int fit_type = atoi(argv[5]);//was 1;
int pop_size = POP_SIZE; // was atoi(argv[1]);//was 500;
int page_count = atoi(argv[2]);//was 10,unused;
int page_size = atoi(argv[3]);//was 3 still being used in Population(). NOTE: popsize = pagesize * l_range;
long seed = runs*5+7;
int tournament_size = 4;
vector<double> mean_evade;
int l_range = atoi(argv[6]);
int iterations = atoi(argv[4]);//was 50000;
double P_xover = atof(argv[9]); //was 0.9
double P_mutation = atof(argv[8]);//was 0.5
double P_swap = atof(argv[10]);//was 0.5
string out_prog = argv[7];
int runidloc = out_prog.find_last_of("/");
int terminate = 0; // population converged?
int terminate_now = 0; // this takes terminate and other variables into consideration
string run_ID = out_prog.substr(runidloc+1); // run id is the part that comes after the last '/'
out_prog += ".";
out_prog += my_itoa(runs);
cout<<"[INFO] Results will be written to "<<out_prog<< " (RunID : "<<run_ID<<")"<<endl;
// Note: .inst file should have a blank line at the end, or it seg faults
string path = PRMDIR + prefix + ".inst";
NumberGen numgen (seed);
// train pHmr
string schema_file1 = "schema_training.dat";
string schema_file2 = "schema.dat";
string dump_file = out_prog + ".pHmr";
if (detector == "pHmr") 
{
	create_pHmr_schema(out_prog, schema_file1, runs, page_size); // run this before the initialization of Population in case of pHmr
	create_pHmr_schema(out_prog, schema_file2, runs, page_size); // run this before the initialization of Population in case of pHmr
	train_pHmr(schema_file1, string(dump_file + ".train").c_str(), prefix); // train pHmr with the new training schema
}
cout<<"=============================================================="<<endl;
cout<<"Below, you will see the artificial arms race take place."<<endl;
cout<<"Population starts with naive attacks and the mean raw anomaly"<<endl;
cout<<"(Mean_rawAnom) will be high. As training continues, GP interacts"<<endl;
cout<<"with the detector and discovers attachs with lower anomaly rates."<<endl;
cout<<"You can see this by observing the anomaly rates."<<endl;
cout<<"=============================================================="<<endl;
Population myPop (pop_size, page_count, page_size, path, &numgen, fit_type, l_range, run_ID, config, prefix, tournament_size, detector);
myPop.pareto_rank();
////// debug code
cout<<"[INFO] Printing the initial population properties:"<<endl;
cout<<"Id Length \tSuccess \tAnomaly \tDelay \tRank\n";
for(int i=0;i<pop_size; ++i)
{
	for(int j=0;j<pop_size; ++j)
	{
		if (myPop.rank[j] == i)
		{
			cout<<j<<" \t"<<myPop.length[j]<<" \t"<<myPop.success[j]<<" \t"<<myPop.anomaly[j]<<" \t"<<myPop.delay[j]<<" \t"<<myPop.rank[j]<<" \n";
		}
		
	}

}
//cout<<"Outcome1:"<<(myPop.global_distro == myPop.global_distro)<<endl;
//cout<<"Outcome2:"<<(myPop.global_distro == myPop.distros[0])<<endl;
/////



if ( (tournament_size % 2) != 0 ) cout << "[ERROR] This training code assumes that tournament size is even." << endl;
int *tournament = new int[tournament_size];

//******************************************************************
//Training Loop Starts Here									   *
//******************************************************************
for (int i = 0; ((i < iterations) && !(terminate_now)) ; ++i)
{
	//************************************************************
	// CPU INTENSIVE STUFF (every NUM_ITER tournaments)
	//	- Detailed reporting 
	//	- Greedy search to boost best ind's fitness
	//************************************************************
	
	if (i % NUM_ITER == 0 || i == iterations - 1)
	{
		if ( do_greedysearch ) myPop.greedySearch(); 
		int hits = 0;
		double b = 0;
		double d = 0;
		double t = 0;
		double t2 = 0;
		double f = 0;
		
		double total_len = 0;
		for(int j = 0; j < myPop.popsize; ++j)
		{ 
			double tmp = myPop.fitness[j];
			//cout<<"fit="<<tmp<<endl;
			total_len += myPop.length[j]; 
			d += myPop.delay[j];
			f += tmp;
			if (tmp >= 5) 
			{
				++hits;
				t += myPop.anomaly[j];
				t2 += myPop.raw_anomaly[j];
			}
			 
		}
		cout<<runs<<"\tTour = "<<i<<" MeanLen = "<<total_len/(double)myPop.popsize<<" MeanFit = "<<f/(double)myPop.popsize<<" Hits = "<<hits<<" Mean_Anom = "<<t/(double)hits<<" Mean_rawAnom = "<<t2/(double)hits<<" Mean_Delay = "<<d/(double)myPop.popsize<<" Unique = "<<myPop.unique_count()<<" Px = "<<P_xover<<" Pm = "<<P_mutation*( (double)( (double)(iterations - i) / (double) iterations ) )<<" Term.Ct: "<<terminate<<" Max rank: "<<myPop.max_rank<<endl;
	}
	myPop.initializeTournament_pareto(tournament, tournament_size); // Tournament must be sorted according to fitness
	//***********************************************
	// Copy upper half to lower half                *
	//************************************************
	//cout<<"Reproduce"<<endl;
	for (int j = 0; j< tournament_size/2; ++j)
		myPop.reproduce( tournament[j], tournament[tournament_size/2 + j] ); // In tournament of 4: 0 -> 2 and 1 -> 3
	//***********************************************
	//Apply Crossover					            *
	//***********************************************
	//cout<<"XO"<<endl;
	if ( numgen.ran3() < P_xover )
	{	
		for (int j = tournament_size/2; j< tournament_size; ++j)
		{
			if (j % 2 == 0)
			{
				// Apply the selected XO
				int point1, point2;
			
				switch (config.xo_type)
				{
					case 1: // page based XO
						point1 = numgen.uniform_intgen(0, (int)(myPop.length[tournament[j]]/page_size) -1);
						point2 = numgen.uniform_intgen(0, (int)(myPop.length[tournament[j+1]]/page_size) -1);
						myPop.pagebasedXO(tournament[j], point1, tournament[j + 1], point2);
						break;
					case 2: // non-page based XO
						myPop.nonpagebasedXO(tournament[j], tournament[j + 1]);
						break;
					case 3: // cut and splice XO
						myPop.cut_spliceXO(tournament[j], tournament[j + 1]);
						break;
					case 4: // block XO
						myPop.blockXO(tournament[j], tournament[j + 1]);
						break;
					default:
						cout<<"[ERROR] Illegal crossover selection, no crossover will be applied."<<endl;
						break;
				}					
			if(individualPDFneeded) 
			{
				myPop.updatePDF(tournament[j]);
				myPop.updatePDF(tournament[j + 1]);
			}
			} 
		}
	}
	//cout<<"Mut"<<endl;
	//***********************************************
	//Apply Mutation								*
	//***********************************************
	for (int j = tournament_size/2; (j< tournament_size) && (P_mutation > 0); ++j)
		{
			switch (config.mut_type) // Apply the selected mutation
				{
				case 1:
					myPop.uniformMutation(tournament[j]);
					break;
				case 2:
					myPop.instructionwiseMutation(tournament[j], P_mutation* ( (double)( (double)(iterations - i) / (double) iterations ) ) );
					break;
				case 3:
					myPop.instructionwiseMutation_ND(tournament[j], P_mutation* ( (double)( (double)(iterations - i) / (double) iterations ) ) );
					break;
				case 4:
					myPop.uniformMutation_instFrq(tournament[j], &(myPop.global_distro));
					break;
				case 5:
					myPop.instructionwiseMutation_instFrq(tournament[j], P_mutation* ( (double)( (double)(iterations - i) / (double) iterations ) ), &(myPop.global_distro) );
					break;
				case 6:
					myPop.instructionwiseMutation_ND_instFrq(tournament[j], P_mutation* ( (double)( (double)(iterations - i) / (double) iterations ) ), &(myPop.global_distro) );
					break;
				case 7:
					myPop.uniformMutation_instFrq(tournament[j], &(myPop.distros[tournament[j]]));
					break;
				case 8:
					myPop.instructionwiseMutation_instFrq(tournament[j], P_mutation* ( (double)( (double)(iterations - i) / (double) iterations ) ), &(myPop.distros[tournament[j]]) );
					break;
				case 9:
					myPop.instructionwiseMutation_ND_instFrq(tournament[j], P_mutation* ( (double)( (double)(iterations - i) / (double) iterations ) ), &(myPop.distros[tournament[j]]) );
					break;
				default:
					cout<<"[ERROR] Illegal mutation selection, no mutation will be applied."<<endl;
					break;
				}
			if(individualPDFneeded) myPop.updatePDF(tournament[j]);
					
		}
	//cout<<"Swap"<<endl;
	//***********************************************
	// Apply Swap									*
	//***********************************************
	for (int j = tournament_size/2; (j< tournament_size) && (P_swap > 0); ++j)
		{
			if ( numgen.ran3() < P_swap )
			{
				switch (config.swp_type) // Apply the selected swap
					{
					case 1:
						myPop.swapMutation(tournament[j]);
						break;
					case 2:
						myPop.swapMutation_ND(tournament[j]);
						break;
					default:
					cout<<"[ERROR] Illegal swap selection, no swap will be applied."<<endl;
					break;
					}
			}
		}
	//	cout<<"Rank"<<endl;
	// 1. rank all N individuals
	myPop.pareto_rank();
	// 2. replace the worst two individuals with children	
	//cout<<"Replace"<<endl;
	myPop.replace();
	// ** NOT OPTIMAL ** but rank the new population (N inds)
	//cout<<"Rank2"<<endl;
	myPop.pareto_rank();
	
	//Update selection PDF
	//cout<<"UPdate selection PDF"<<endl;
	myPop.update_selectionPDF();
	// Update rank histograms
	//cout<<"UPdate rank PDF"<<endl;
	myPop.update_rankPDFs();
	// update the termination criteria
	if (myPop.rank_histogram_prev == myPop.rank_histogram)
		terminate = terminate + 1;
	else
		terminate = 0;
	//if (i % 100 == 0 )cout<<"Tour: "<<i<<"   Terminate: "<<terminate<<"   Max rank: "<<myPop.max_rank<<endl;
	terminate_now = (terminate >= TERMINATE_COUNT) && (i >=MIN_ITERS_BEFORE_TERMINATE); //terminate if the rank PDF does not change over 10 tournaments AFTER 5000 iterations
																  // in other words, give population 5000 tournaments to find a solution... 
} // End of training loop



delete[] tournament;
myPop.write_results(out_prog);
// after this point, training ends, and analysis begins. No further tournament selection should be done after this.
if (detector == "pHmr") 
{
	train_pHmr(schema_file2, string(dump_file + ".test").c_str(), prefix);
	// set the flag so that the fitness func knows it is testing.
	myPop.phmr_is_training = 0;
	myPop.recalculateAllFitness();
	myPop.write_results(out_prog+".test");
}


} // End of run
//*************************************
// Set Timer Again and Report         *
//*************************************
t1 = CPUTIME;
time(&u2);
cout<<"[INFO] CPU time = "<< t1-t0 << " secs.\nUser Time = "<<(int)(u2-u1)<<" secs.\n";
return 0;
}

void documentation()
{
char str[] = "[INFO] A linear GP implementation. The required parameters are as follows\n\n"
			 "   - POP SIZE : Population size. (default: 500) -- Note: This is hardcoded to 500 at the moment.\n"
			 "   - PAGE COUNT : Number of pages in an individual (not in use, default: -1). \n"
			 "   - PAGE LENGTH: Number of instructions in a page (not in use, default: -1). \n"
			 "   - NO. OF ITERATIONS : Number of tournaments. (default: [10000,50000], less for quicker runs) \n"
			 "   - FITNESS TYPE : Fitness function to be used. 1 for incremental, 2 for concurrent and 3 for bypassing the validity check. (default: 1) \n"
			 "   - RANGE : Maximum number of instructions allowed in an individual during initialization. (default: [100,500]) \n"
			 "   - OUTPUT FILE : Output file prefix \n"
			 "   - PROB OF MUTATION : Probability of mutation, some mutations exchange more material so set it carefully. (default: 0.5)\n"
			 "   - PROB OF CROSSOVER : Probability of crossover. (default: 0.9) \n"
			 "   - PROB OF SWAP : Probability of swap. (default: 0.5)\n"
			 "   - GREEDY : Apply greedy search operator? 1 yes, 0 no. (default: 0)\n"
			 "   - RUN STARTS FROM : Determine the seed for random number generator. Use if your run got interrupted and don't want to start over. (default: 0)\n"
			 "   - NO. OF RUNS : Number of runs. (default: 1, >1 if you are computing stats over many runs.) \n"
			 "   - ENABLE OWC : Seed all individuals with OWC during initialization? 1 yes, 0 no. (default: 0, 1 for quicker convergence) \n"
			 "   - CROSSOVER TYPE : Crossover type (default: 1)\n"
			 "   - MUTATION TYPE : Mutation type (default: 1)\n"
			 "   - SWAP TYPE: Swap type (default: 1)\n"
			 "   - APPLICATION: Which application to run GP on. Choices are traceroute, ftp, restore or samba. \n "
			 "   - DETECTOR: Which detector to run GP against. Choices are stide, pH, pHmr, hmm or svm (case sensitive).  \n"
			 "   E.g. ./syscall-experiment 500 -1 5 10000 1 100 /tmp/gpfile 0.5 0.9 0.5 0 0 1 0 3 2 1 traceroute pH\n"
			 "   E.g. ./syscall-experiment 500 -1 5 10000 1 100 /tmp/gpfile 0.5 0.9 0.5 0 0 1 1 3 2 1 traceroute pH (turns on OWC)";
cout<<str<<endl;



}
