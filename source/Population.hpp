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
#include "Parameters.hpp"
#include "Decoder.hpp"
#include "Functions.hpp"
#include "ProbabilityDistro.hpp"
#include "asmRuntime/RuntimeFitness.hpp"

using namespace std;

class Population
{
	public:
		// Variables
		BYTE4		**population;
		double		*fitness;
		double		*delay;
		double		*anomaly;
		double		*raw_anomaly;
		double		*success;
		double		*rank;
		int			*length;
		int			*age;
		int			pagesize, numpages, popsize, range;
		int			overflow;						//The space to store the extra tournamentsize/2  children  
		int			max_rank;
		Decoder		*decoder;
		NumberGen	*number;
		vector<string>		syscalls;
		vector<double>		values;					// Will be used repeatedly for PDF initialization.
		vector<double>		counts;					// Will be used repeatedly for PDF initialization. Make sure this is uptodate before use!!
		ProbabilityDistro	global_distro;			// frequency of system calls from the instruction set
		ProbabilityDistro *distros;					// frequencies of system calls for each individual
		ProbabilityDistro selection;				// selection probability according to rank
		ProbabilityDistro rank_histogram;			// distribution of ranks
		ProbabilityDistro rank_histogram_prev;		// distribution of ranks
		int			fitness_fn;
		int			phmr_is_training; 
		int			CHECK_PARAMS;
		int			OPEN, WRITE, CLOSE;
		int			MAX_IND_LEN;
		string		runID;
		string		app_prefix;
		string		det_prefix;
		// Member Functions
					Population();
					~Population();
					// --- RETIRED --- Population(int indsize, int pgnum, int pgsize, Decoder *d, NumberGen *ng);
					// --- RETIRED --- Population(int indsize, int pgnum, int pgsize, string dparam, NumberGen *ng, int l_range);
					Population(int indsize, int pgnum, int pgsize, string dparam, NumberGen *ng, int fit_type, int l_range, string run_id, configuration config, string prefix, int tournament_size, string detector);
		string		print_individual(int i);
		void		pagebasedXO(int ind1, int pag1, int ind2, int pag2);
		void		nonpagebasedXO(int ind1, int ind2);
		void		cut_spliceXO(int ind1, int ind2);
		void		blockXO(int ind1, int ind2);
		void		uniformMutation(int ind);
		void		instructionwiseMutation(int ind, double p_mut);
		void		instructionwiseMutation_ND(int ind, double p_mut); // non destructive, maintains OWC
		void		uniformMutation_instFrq(int ind,  ProbabilityDistro *prob_dist);
		void		instructionwiseMutation_instFrq(int ind, double p_mut,  ProbabilityDistro *prob_dist);
		void		instructionwiseMutation_ND_instFrq(int ind, double p_mut,  ProbabilityDistro *prob_dist); // non destructive, maintains OWC
		void		swapMutation(int ind);
		void		swapMutation_ND(int ind);
		void		reproduce(int src, int dest);
		void		greedySearch();
		int			initializeTournament(int *tournament, int size);
		int			initializeTournament_pareto(int *tournament, int size);
		double		calculateFitness(int ind);
		double		calculateAnomaly_withoutPreamble(int ind);
		double		calculateAnomaly_withoutPreamble_pH(int ind);
		//double		--- RETIRED --- calculateAnomaly_withoutPreamble_pHmr(int ind);
		double		calculateAnomaly_withoutPreamble_pHmr2(int ind, int is_training_schema);
		double		calculateAnomaly_withoutPreamble_hmm(int ind);
		double		calculateAnomaly_withoutPreamble_NN(int ind);
		double		meanFitness();
		int			unique_count();
		void		find_OWC(int ind, int *O, int *W, int *C);
		void		updatePDF(int ind);
		//void		--- RETIRED --- writePopulation(string filename);  
		// Pareto Functions
		void		pareto_rank();
		void		pareto_rank2();
		int			A_dominates_B(int indA, int indB);
		void		update_selectionPDF();
		void		update_rankPDFs();
		void		replace();
		int			get_max_rank();
		int			rankOf(int ind);
		// functions transfere from simpGE
		void		process_strace(char *argv1, char *argv2);
		double		calc_fitness (int ind, double &anomrate, double &success);
		double		calc_fitness (string phen, string db, double &anomrate, double &success, int with_preamble);
		double		calc_fitness2(int ind, double &anomrate, double &success);
		double		calc_fitness3(int ind, double &anomrate, double &success);
		double		calc_fitness_pH(int ind, double &anomrate, double &success, double &delay);
		double		calc_fitness_pH(string phen, string db, double &anomrate, double &success, double &delay, int with_preamble);
		//double	--- RETIRED ---	calc_fitness_pHmr(int ind, double &anomrate, double &success, double &delay);
		double		calc_fitness_pHmr2(int ind, double &anomrate, double &success, double &delay, int is_training_schema);
		double		calc_fitness_pHmr2(string phen, string db, double &anomrate, double &success, double &delay, int is_training_schema, int with_preamble);
		double		calc_fitness_hmm(int ind, double &anomrate, double &success);
		double		calc_fitness_hmm(string phen, string db, double &anomrate, double &success, int with_preamble);
		double		calc_fitness_NN(int ind, double &distance, double &success, double &max_dist);
		double		calc_fitness_NN(string phen, string db, double &distance, double &success, double &max_dist, int with_preamble);
		void		prepare_NN_input(string filename);
		int			check_valid(int ind);
		void		write_results(string filename);
		void		check_results(int tour);
		void		recalculateAllFitness();
		
};


