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

#include "Population.hpp"
/***********************************************************
 *
 */
Population::Population() { };

/***********************************************************
 * Main constructor with fitness function select option
 * This is the contructor that needs to be used
 */
Population::Population(int indsize, int pgnum, int pgsize, string decoparam, NumberGen *ng, int fit_type, int l_range, string run_ID, configuration config, string prefix, int tournament_size, string detector)
{
	/*
	population = new BYTE4[indsize*pgsize*pgnum];
	fitness = new double[indsize];
	anomaly = new double[indsize];
	assert(population); assert(fitness); assert(anomaly);
	pagesize = pgsize;
	numpages = pgnum;
	popsize = indsize;
	number = ng;
	*/
	phmr_is_training = 1;
	CHECK_PARAMS = 1; // while checking for OWC is it enough to have any Open <0> or should it be Open /etc/passwd <1> (GA vs. GP)
	MAX_IND_LEN = 1000;
	pagesize = pgsize;
	numpages = pgnum; // "retired"
	popsize = POP_SIZE;
	number = ng;
	range = l_range;
	runID = run_ID;
	app_prefix = prefix;
	det_prefix = detector;
	max_rank = -1;
	overflow = tournament_size/2;
	fitness = new double[indsize + overflow];
	delay = new double[indsize + overflow];
	anomaly = new double[indsize + overflow];
	raw_anomaly = new double[indsize + overflow];
	success = new double[indsize + overflow];
	rank = new double[indsize + overflow];
	length = new int[indsize + overflow];
	population = new BYTE4 * [popsize + overflow];
	distros = new ProbabilityDistro[popsize + overflow];
	for (int i=0; i < popsize; ++i)
	{
		values.push_back(i); // we build the values vector here
		counts.push_back(0); // we push zeroes to get the size right
	}
	
	for (int i=0; i < (popsize + overflow); ++i)
	{
		int i_size = number->uniform_intgen(1, range);
		population[i] = new BYTE4[i_size*pagesize];
		assert(population[i]);
		length[i] = i_size*pagesize;		
	}

	assert(population); assert(fitness); assert(anomaly); assert(length); assert(rank); assert(delay); assert(raw_anomaly);
	
	fitness_fn = fit_type; //default if you want 2 change it from main() by adding myPop.fitness_fn = 2;
	// read in the vector
	ifstream fin (string("../resources/pH/"+app_prefix+".enum").c_str());
	assert(fin);
	string line="";
	char ch;
	while (fin.get(ch))
	{
		if(( ch !='\n') && ( ch !='\r'))
			line+= ch;
		else // process the line
		{
			syscalls.push_back(line);
			line = "";
		}
	}
	fin.close();


	decoder = new Decoder(decoparam);
	decoder->NUMG = number;
	// if owc init enabled, locate them at the instruction set
	OPEN = -1; CLOSE = -1; WRITE = -1;
	if (config.enable_owc == 1)
	{
		for(int i=0; i< decoder->INS.size; ++i)
		{
			if (decoder->INS.values[i] == "open")
				OPEN = i;
			if (decoder->INS.values[i] == "write")
				WRITE = i;
			if (decoder->INS.values[i] == "close")
				CLOSE = i;
		}
	}
	if ((OPEN == -1) || (CLOSE == -1) || (WRITE == -1)) 
		cout<<"[INFO] OWC not found. This is normal if OWC initialization is not selected."<<endl;
	
	for(int i=0; i<(popsize + overflow); ++i)
	{
		
		for(int j=0; j<length[i]; ++j)
		{
			*(population[i] + j) =  decoder->random_instruction();
		}
		// It is initialized but, pick three locations to write open-write-close
		if (config.enable_owc == 1)
		{
			//cout<<"OWC initialization enabled."<<endl;
			int open_loc = number->uniform_intgen(0, length[i]-3);
			int write_loc = number->uniform_intgen(open_loc + 1, length[i]-2);
			int close_loc = number->uniform_intgen(write_loc + 1, length[i]-1);
	
			*(population[i] + open_loc) =  decoder->specific_instruction(OPEN, 0, -1); // from params/syscall.inst, open line# -1
			//cout<<i<<"o:"<<open_loc<<" "<<decoder->decode_instruction(*(population[i] + open_loc))<<endl;
			*(population[i] + write_loc) =  decoder->specific_instruction(WRITE, 0, 1); // from params/syscall.inst, write line# -1
			//cout<<i<<"w:"<<write_loc<<" "<<decoder->decode_instruction(*(population[i] + write_loc))<<endl;
			*(population[i] + close_loc) =  decoder->specific_instruction(CLOSE, 0, -1); // from params/syscall.inst, close line# -1
			//cout<<i<<"c:"<<close_loc<<" "<<decoder->decode_instruction(*(population[i] + close_loc))<<endl;
			//cout<<"-----------------------"<<endl;
		}
	}
	// Prepare the global probability distrubition from instruction set
	vector<double> cnt;
	vector<double> val;
	for (int i=0; i<decoder->INS.size; ++i)
	{
		val.push_back(i);
		cnt.push_back(decoder->INS.probs[i]);
	}
	global_distro.initialize(val, cnt);
	global_distro.printPDF_CDF();
	
	// Prepare the individual PDFs using decode_instr, not the most efficient
	// way but since this is done once, we can afford it
	for(int i=0; i<(popsize + overflow); ++i)
		updatePDF(i);
		
	// Calculate the fitness of individuals
	for(int i=0; i<(popsize + overflow); ++i)
		calculateFitness(i);
	// Now pareto rank it
	pareto_rank();
	// create the initial selection PDF
	update_selectionPDF();
	cout<<"Finished initializing Population object."<<endl;
};


/***********************************************************
 *
 */
Population::~Population() 
{
	//delete[] population;
	for (int i = 0; i < popsize; ++i)
	{
      delete [] population[i];
	}
	delete [] population;
	delete[] fitness;
	delete[] anomaly;
	delete[] length;
	//delete decoder;
};

/***********************************************************
 *
 */
string Population::print_individual(int index)
{

	string ret;
	for(int i=0; i<length[index]; ++i)
	{
		ret += decoder->decode_instruction(*(population[index] + i));
		ret += "\n";
	}
	//
	//for(int i=index*numpages*pagesize; i<(index+1)*numpages*pagesize; ++i)
	//{
	//	ret+=decoder->decode_instruction(population[i]);
	//	ret+="\n";
	//}	
	return ret;

};

/***********************************************************
 * XO TYPE: 1
 * Page based crossover between ind1, pag1 X ind2, pag2
 * ind1 and pag1 should start from 0
 */
void Population::pagebasedXO(int ind1, int pag1, int ind2, int pag2)
{
	BYTE4 *tmp = new BYTE4[pagesize];
	assert(tmp);
	BYTE4 *p1 = (population[ind1] + pag1*pagesize); //ptr to first instruction in pag1 
	BYTE4 *p2 = (population[ind2] + pag2*pagesize); //ptr to first instruction in pag2 
	memcpy( tmp, p1,  pagesize * sizeof(BYTE4) );
	memcpy( p1,  p2,  pagesize * sizeof(BYTE4) );
	memcpy( p2,  tmp, pagesize * sizeof(BYTE4) );
	calculateFitness(ind1);
	calculateFitness(ind2);
	delete[] tmp;
};

/***********************************************************
 * XO TYPE: 2
 * Flexible XO: choses the amount of exchange and
 * XO points.
 * ind1 should start from 0
 */
void Population::nonpagebasedXO(int ind1, int ind2)
{
	// determine the max possible amount of exchange (i.e length of the shorter ind)
	int smaller_len = 0;
	if (length[ind1] > length[ind2])
		smaller_len = length[ind2];
	else
		smaller_len = length[ind1];
	
	int amount = number->uniform_intgen(1, smaller_len);
	BYTE4 *tmp = new BYTE4[amount];
	assert(tmp);
	// determine the XO points on both parents
	int loc1 = number->uniform_intgen(0, length[ind1] - amount); //start and end locations are inclusive
	int loc2 = number->uniform_intgen(0, length[ind2] - amount); //start and end locations are inclusive
	BYTE4 *p1 = (population[ind1] + loc1); //ptr to first instruction in parent1 
	BYTE4 *p2 = (population[ind2] + loc2); //ptr to first instruction in parent2 
	memcpy( tmp, p1,  amount * sizeof(BYTE4) );
	memcpy( p1,  p2,  amount * sizeof(BYTE4) );
	memcpy( p2,  tmp, amount * sizeof(BYTE4) );
	calculateFitness(ind1);
	calculateFitness(ind2);
	delete[] tmp;
};


/***********************************************************
 * XO TYPE: 3
 * Cut and Splice XO: picks a different xo point for each parent
 * it should change the max. pop. len as opposed to others (that onlu change the avg.).
 * ind1 should start from 0
 */
void Population::cut_spliceXO(int ind1, int ind2)
{
	/*
	BYTE4 *i1 = new BYTE4[length[ind1] + 50];
	BYTE4 *i2 = new BYTE4[length[ind2] + 50];
	memcpy(i1, population[ind1], length[ind1]*sizeof(BYTE4));
	memcpy(i2, population[ind2], length[ind2]*sizeof(BYTE4));
	//i1[length[ind1]] = i1[0];
	//i2[length[ind2]] = i2[0]; 
	delete [] population[ind1];
	delete [] population[ind2];
	population[ind1] = i1;
	population[ind2] = i2;
	*/
	
	
	// determine the XO points on both parents
	int loc1 = number->uniform_intgen(0, length[ind1] - 1); //start and end locations are inclusive
	int loc2 = number->uniform_intgen(0, length[ind2] - 1); //start and end locations are inclusive
	int xchange_amt1 = length[ind1] - loc1; //no -1 because locN will be included in exchange
	int xchange_amt2 = length[ind2] - loc2; //no -1 because locN will be included in exchange
	int new_len1 = loc1 + xchange_amt2;
	int new_len2 = loc2 + xchange_amt1;
	//cout<<"len1="<<length[ind1]<<" loc1="<<loc1<<" xe1="<<xchange_amt1<<endl;
	//cout<<"len2="<<length[ind2]<<" loc2="<<loc2<<" xe2="<<xchange_amt2<<endl;
	// xo is not permissible if one of the children exceed the max ind len limit
	if ((new_len1 > MAX_IND_LEN) || (new_len2 > MAX_IND_LEN))
	{
		return;  
	}
	// copy the material to be exchnaged to temp locations before ind resizing
	BYTE4 *new_ind1 = new BYTE4[new_len1];
	//for(int i=0;i<new_len1; ++i) {new_ind1[i] = decoder->random_instruction();}
	BYTE4 *new_ind2 = new BYTE4[new_len2];
	//for(int i=0;i<new_len2; ++i) {new_ind2[i] = decoder->random_instruction();}
	// copy now	
	memcpy(new_ind1, (population[ind1]), loc1 * sizeof(BYTE4));
	memcpy((new_ind1 + loc1), (population[ind2] + loc2), xchange_amt2 * sizeof(BYTE4));
	memcpy(new_ind2, (population[ind2]), loc2 * sizeof(BYTE4));
	memcpy((new_ind2 + loc2), (population[ind1] + loc1), xchange_amt1 * sizeof(BYTE4));	
	// modify population ptrs
	delete [] population[ind1];
	delete [] population[ind2];
	population[ind1] = new_ind1;
	population[ind2] = new_ind2;
	assert(population[ind1]);
	assert(population[ind2]); 
	// updatethe relevant information (len, fitness)
	length[ind1] = new_len1;
	length[ind2] = new_len2;
	calculateFitness(ind1);
	calculateFitness(ind2);
	
};


/***********************************************************
 * XO TYPE: 4
 * Block XO: determines the locations of OWC in parents, OWCs divide the
 * parents into blocks. This XO exchanges 1-2 blocks between parents. 
 * Note that exchanging 3 blocks is actually exchanging 1 and
 * exchanging 4 blocks is actually exchanging none.
 */
void Population::blockXO(int ind1, int ind2)
{
	//cout<<"in XO"<<endl;
	// Determine the OWC in parents
	// I.e. first O, last C and any W in between for both parents
	int O1 = -1, O2 = -1, W1 = -1, W2 = -1, C1 = -1, C2 = -1;
	find_OWC(ind1, &O1, &W1, &C1);
	find_OWC(ind2, &O2, &W2, &C2);
	// we have the OWC locations, all should be non -1.
	if ((O1 == -1) || (W1 == -1) || (C1 == -1) || (O2 == -1) || (W2 == -1) || (C2 == -1))
	{
		cout<<"Warning: Cannot apply block XO operator if OWC are not present."<<endl;
		cout<<"   O1:"<<O1<<" W1:"<<W1<< " C1:"<<C1<<endl;
		cout<<"   O2:"<<O2<<" W2:"<<W2<< " C2:"<<C2<<endl;
		return;
	}
	if ( !((O1 < W1) && (W1 < C1)) || !((O2 < W2) && (W2 < C2)) )
	{
		cout<<"Warning: Cannot apply block XO operator if OWC is not in that order."<<endl;
		cout<<"   O1:"<<O1<<" W1:"<<W1<< " C1:"<<C1<<endl;
		cout<<"   O2:"<<O2<<" W2:"<<W2<< " C2:"<<C2<<endl;
		return;
	}
	// Block1   O   Block2   W   Block3   C   Block4
	// Pick one block from each parent
	int block1 =  number->uniform_intgen(1, 4);
	int block2 =  number->uniform_intgen(1, 4);
	
	int p1_block1_len = O1 - 0;
	int p1_block2_len = W1 - O1 - 1;
	int p1_block3_len = C1 - W1 - 1;
	int p1_block4_len = length[ind1] - C1 - 1; 
	int p1_block1_starts = 0;
	int p1_block2_starts = O1 + 1;
	int p1_block3_starts = W1 + 1;
	int p1_block4_starts = C1 + 1;
	
	int p2_block1_len = O2 - 0;
	int p2_block2_len = W2 - O2 - 1;
	int p2_block3_len = C2 - W2 - 1;
	int p2_block4_len = length[ind2] - C2 - 1;
	int p2_block1_starts = 0;
	int p2_block2_starts = O2 + 1;
	int p2_block3_starts = W2 + 1;
	int p2_block4_starts = C2 + 1;
	vector<BYTE4> p1_block1, p1_block2, p1_block3, p1_block4; 
	vector<BYTE4> p2_block1, p2_block2, p2_block3, p2_block4; 
	for (int i = 0; i < length[ind1]; ++i)
	{	
		if (i < O1) p1_block1.push_back(*(population[ind1] + i));
		if ((i > O1) && (i < W1)) p1_block2.push_back(*(population[ind1] + i));
		if ((i > W1) && (i < C1)) p1_block3.push_back(*(population[ind1] + i));
		if (i > C1) p1_block4.push_back(*(population[ind1] + i));
	}
	for (int i = 0; i < length[ind2]; ++i)
	{	
		if (i < O2) p2_block1.push_back(*(population[ind2] + i));
		if ((i > O2) && (i < W2)) p2_block2.push_back(*(population[ind2] + i));
		if ((i > W2) && (i < C2)) p2_block3.push_back(*(population[ind2] + i));
		if (i > C2) p2_block4.push_back(*(population[ind2] + i));
	}				
	// build the second child
	vector<BYTE4> child2;
	switch(block1)
	{
		case 1:
			for (int i=0; i<p1_block1.size(); ++i) child2.push_back(p1_block1.at(i));
			child2.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p2_block2.size(); ++i) child2.push_back(p2_block2.at(i));
			child2.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p2_block3.size(); ++i) child2.push_back(p2_block3.at(i));
			child2.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p2_block4.size(); ++i) child2.push_back(p2_block4.at(i));					
			break;
		case 2:
			for (int i=0; i<p2_block1.size(); ++i) child2.push_back(p2_block1.at(i));
			child2.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p1_block2.size(); ++i) child2.push_back(p1_block2.at(i));
			child2.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p2_block3.size(); ++i) child2.push_back(p2_block3.at(i));
			child2.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p2_block4.size(); ++i) child2.push_back(p2_block4.at(i));					
			break;
		case 3:
			for (int i=0; i<p2_block1.size(); ++i) child2.push_back(p2_block1.at(i));
			child2.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p2_block2.size(); ++i) child2.push_back(p2_block2.at(i));
			child2.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p1_block3.size(); ++i) child2.push_back(p1_block3.at(i));
			child2.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p2_block4.size(); ++i) child2.push_back(p2_block4.at(i));					
			break;
		case 4:
			for (int i=0; i<p2_block1.size(); ++i) child2.push_back(p2_block1.at(i));
			child2.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p2_block2.size(); ++i) child2.push_back(p2_block2.at(i));
			child2.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p2_block3.size(); ++i) child2.push_back(p2_block3.at(i));
			child2.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p1_block4.size(); ++i) child2.push_back(p1_block4.at(i));					
			break;
	}
	// build the first child
	vector<BYTE4> child1;
	switch(block2)
	{
		case 1:
			for (int i=0; i<p2_block1.size(); ++i) child1.push_back(p2_block1.at(i));
			child1.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p1_block2.size(); ++i) child1.push_back(p1_block2.at(i));
			child1.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p1_block3.size(); ++i) child1.push_back(p1_block3.at(i));
			child1.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p1_block4.size(); ++i) child1.push_back(p1_block4.at(i));					
			break;
		case 2:
			for (int i=0; i<p1_block1.size(); ++i) child1.push_back(p1_block1.at(i));
			child1.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p2_block2.size(); ++i) child1.push_back(p2_block2.at(i));
			child1.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p1_block3.size(); ++i) child1.push_back(p1_block3.at(i));
			child1.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p1_block4.size(); ++i) child1.push_back(p1_block4.at(i));					
			break;
		case 3:
			for (int i=0; i<p1_block1.size(); ++i) child1.push_back(p1_block1.at(i));
			child1.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p1_block2.size(); ++i) child1.push_back(p1_block2.at(i));
			child1.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p2_block3.size(); ++i) child1.push_back(p2_block3.at(i));
			child1.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p1_block4.size(); ++i) child1.push_back(p1_block4.at(i));					
			break;
		case 4:
			for (int i=0; i<p1_block1.size(); ++i) child1.push_back(p1_block1.at(i));
			child1.push_back( decoder->specific_instruction(OPEN, 0, -1) );
			for (int i=0; i<p1_block2.size(); ++i) child1.push_back(p1_block2.at(i));
			child1.push_back( decoder->specific_instruction(WRITE, 0, 1) );
			for (int i=0; i<p1_block3.size(); ++i) child1.push_back(p1_block3.at(i));
			child1.push_back( decoder->specific_instruction(CLOSE, 0, -1) );
			for (int i=0; i<p2_block4.size(); ++i) child1.push_back(p2_block4.at(i));					
			break;
	}
	BYTE4 *new_ind1 = new BYTE4[child1.size()];
	BYTE4 *new_ind2 = new BYTE4[child2.size()];
	assert(new_ind1);
	assert(new_ind2);
	for(int i = 0; i < child1.size(); ++i)
		new_ind1[i] = child1.at(i);
	for(int i = 0; i < child2.size(); ++i)
		new_ind2[i] = child2.at(i);
	// modify population ptrs
	delete [] population[ind1];
	delete [] population[ind2];
	population[ind1] = new_ind1;
	population[ind2] = new_ind2;
	assert(population[ind1]);
	assert(population[ind2]); 
	// updatethe relevant information (len, fitness)
	length[ind1] = child1.size();
	length[ind2] = child2.size();
	calculateFitness(ind1);
	calculateFitness(ind2);								
	/*
	BYTE4 *new_ind1;
	BYTE4 *new_ind2;
	int new_len1 = 0, new_len2 = 0;
	int error = 0;
	// work on parent2 based on the block from parent 1
	switch(block1)
	{
		case 1:
		cout<<"p2b1"<<endl;
			new_len2 = p1_block1_len + 1 + p2_block2_len + 1 + p2_block3_len + 1 + p2_block4_len; 
			new_ind2 = new BYTE4[new_len2];
			assert(new_ind2);
			memcpy(new_ind2, population[ind1], (p1_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p1_block1_len + 2), (population[ind2] + p2_block2_starts), (p2_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p1_block1_len + p2_block2_len + 3), (population[ind2] + p2_block3_starts), (p2_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p1_block1_len + p2_block2_len + p2_block3_len + 4), (population[ind2] + p2_block4_starts), (p2_block4_len) * sizeof(BYTE4) );
			break;
		case 2:
		cout<<"p2b2"<<endl;
			new_len2 = p2_block1_len + 1 + p1_block2_len + 1 + p2_block3_len + 1 + p2_block4_len; 
			new_ind2 = new BYTE4[new_len2];
			assert(new_ind2);
			memcpy(new_ind2, population[ind2], (p2_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + 2), (population[ind1] + p1_block2_starts), (p1_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + p1_block2_len + 3), (population[ind2] + p2_block3_starts), (p2_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + p1_block2_len + p2_block3_len + 4), (population[ind2] + p2_block4_starts), (p2_block4_len) * sizeof(BYTE4) );
			break;
		case 3:
		cout<<"p2b3"<<endl;
			new_len2 = p2_block1_len + 1 + p2_block2_len + 1 + p1_block3_len + 1 + p2_block4_len; 
			new_ind2 = new BYTE4[new_len2];
			assert(new_ind2);
			memcpy(new_ind2, population[ind2], (p2_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + 2), (population[ind2] + p2_block2_starts), (p2_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + p2_block2_len + 3), (population[ind1] + p1_block3_starts), (p1_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + p2_block2_len + p1_block3_len + 4), (population[ind2] + p2_block4_starts), (p2_block4_len) * sizeof(BYTE4) );
			break;
		case 4:
		cout<<"p2b4"<<endl;
			new_len2 = p2_block1_len + 1 + p2_block2_len + 1 + p2_block3_len + 1 + p1_block4_len; 
			new_ind2 = new BYTE4[new_len2];
			assert(new_ind2);
			memcpy(new_ind2, population[ind2], (p2_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + 2), (population[ind2] + p2_block2_starts), (p2_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + p2_block2_len + 3), (population[ind2] + p2_block3_starts), (p2_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind2 + p2_block1_len + p2_block2_len + p2_block3_len + 4), (population[ind1] + p1_block4_starts), (p1_block4_len) * sizeof(BYTE4) );
			break;
		default:
			error = 1;
			break;
	}
	
	// work on parent1 based on the block from parent 2
	switch(block2)
	{
		case 1:
		cout<<"p1b1"<<endl;
			new_len1 = p2_block1_len + 1 + p1_block2_len + 1 + p1_block3_len + 1 + p1_block4_len; 
			new_ind1 = new BYTE4[new_len1];
			assert(new_ind1);
			memcpy(new_ind1, population[ind2], (p2_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p2_block1_len + 2), (population[ind1] + p1_block2_starts), (p1_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p2_block1_len + p1_block2_len + 3), (population[ind1] + p1_block3_starts), (p1_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p2_block1_len + p1_block2_len + p1_block3_len + 4), (population[ind1] + p1_block4_starts), (p1_block4_len) * sizeof(BYTE4) );
			break;
		case 2:
		cout<<"p1b2"<<endl;
			new_len1 = p1_block1_len + 1 + p2_block2_len + 1 + p1_block3_len + 1 + p1_block4_len; 
			new_ind1 = new BYTE4[new_len1];
			assert(new_ind1);
			memcpy(new_ind1, population[ind1], (p1_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + 2), (population[ind2] + p2_block2_starts), (p2_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + p2_block2_len + 3), (population[ind1] + p1_block3_starts), (p1_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + p2_block2_len + p1_block3_len + 4), (population[ind1] + p1_block4_starts), (p1_block4_len) * sizeof(BYTE4) );
			break;
		case 3:
		cout<<"p1b3"<<endl;
			new_len1 = p1_block1_len + 1 + p1_block2_len + 1 + p2_block3_len + 1 + p1_block4_len; 
			new_ind1 = new BYTE4[new_len1];
			assert(new_ind1);
			memcpy(new_ind1, population[ind1], (p1_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + 2), (population[ind1] + p1_block2_starts), (p1_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + p1_block2_len + 3), (population[ind2] + p2_block3_starts), (p2_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + p1_block2_len + p2_block3_len + 4), (population[ind1] + p1_block4_starts), (p1_block4_len) * sizeof(BYTE4) );
			break;
		case 4:
		cout<<"p1b4"<<endl;
			new_len1 = p1_block1_len + 1 + p1_block2_len + 1 + p1_block3_len + 1 + p2_block4_len; 
			new_ind1 = new BYTE4[new_len1];
			assert(new_ind1);
			memcpy(new_ind1, population[ind1], (p1_block1_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + 2), (population[ind1] + p1_block2_starts), (p1_block2_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + p1_block2_len + 3), (population[ind1] + p1_block3_starts), (p1_block3_len + 1) * sizeof(BYTE4) );
			memcpy((new_ind1 + p1_block1_len + p1_block2_len + p1_block3_len + 4), (population[ind2] + p2_block4_starts), (p2_block4_len) * sizeof(BYTE4) );
			break;
		default:
			error = 1;
			break;
	}
	
	delete [] new_ind1;
	delete [] new_ind2;
	*/
	//cout<<"out XO"<<endl;
};





/***********************************************************
 * Uniform mutation over the interval determined by the field size
 * 
 */
void Population::uniformMutation(int ind)
{
	int loc1 = number->uniform_intgen(0, length[ind] - 1);
	*(population[ind] + loc1) = decoder->random_instruction();
	calculateFitness(ind);
	// Note: It does not change the individual length 
};


/***********************************************************
 * Mutate each instruction with P_mut then at the end
 * calculate fitness
 */
void Population::instructionwiseMutation(int ind, double p_mut)
{
	for (int i = 0; i < length[ind]; ++i)
	{
		if ( number->ran3() < p_mut )
		{
			*(population[ind] + i) = decoder->random_instruction();
		}
	}  
	calculateFitness(ind);
	// Note: It does not change the individual length 
};

/***********************************************************
 * Mutate each instruction with P_mut then at the end
 * calculate fitness
 * This op is non destructive in a sense that it keeps
 * the OWC sequence
 */
void Population::instructionwiseMutation_ND(int ind, double p_mut)
{
	//cout<<"in MU"<<endl;
	int O = -1, W = -1, C = -1;
	find_OWC(ind, &O, &W, &C);
	for (int i = 0; i < length[ind]; ++i)
	{
		if ((i != O) && (i != W) && (i != C))
		{
			if ( number->ran3() < p_mut )
			{
				*(population[ind] + i) = decoder->random_instruction();
			}
		}
	}  
	calculateFitness(ind);
	//cout<<"out MU"<<endl;
	// Note: It does not change the individual length 
};















/***********************************************************
 * Uniform mutation over the interval determined by the field size
 * Instruction is selected proportional to syscall frequence in 
 * the instruction set.
 */
void Population::uniformMutation_instFrq(int ind, ProbabilityDistro *prob_dist)
{
	int loc1 = number->uniform_intgen(0, length[ind] - 1);
	*(population[ind] + loc1) = decoder->random_instruction(prob_dist);
	calculateFitness(ind);
	// Note: It does not change the individual length 
};


/***********************************************************
 * Mutate each instruction with P_mut then at the end
 * calculate fitness. Instruction is selected proportional to syscall frequence in 
 * the instruction set.
 */
void Population::instructionwiseMutation_instFrq(int ind, double p_mut,  ProbabilityDistro *prob_dist)
{
	for (int i = 0; i < length[ind]; ++i)
	{
		if ( number->ran3() < p_mut )
		{
			*(population[ind] + i) = decoder->random_instruction(prob_dist);
		}
	}  
	calculateFitness(ind);
	// Note: It does not change the individual length 
};

/***********************************************************
 * Mutate each instruction with P_mut then at the end
 * calculate fitness
 * This op is non destructive in a sense that it keeps
 * the OWC sequence
 * Instruction is selected proportional to syscall frequence in 
 * the instruction set.
 */
void Population::instructionwiseMutation_ND_instFrq(int ind, double p_mut,  ProbabilityDistro *prob_dist)
{
	//cout<<"in MU"<<endl;
	int O = -1, W = -1, C = -1;
	find_OWC(ind, &O, &W, &C);
	for (int i = 0; i < length[ind]; ++i)
	{
		if ((i != O) && (i != W) && (i != C))
		{
			if ( number->ran3() < p_mut )
			{
				*(population[ind] + i) = decoder->random_instruction(prob_dist);
			}
		}
	}  
	calculateFitness(ind);
	//cout<<"out MU"<<endl;
	// Note: It does not change the individual length 
};














/***********************************************************
 * Swap two instructions in an individual
 * 
 */
void Population::swapMutation(int ind)
{
	if(length[ind] == 1) return; // you can't apply swap on a single instr. individual
	int loc1 = 0, loc3 = 0;
	
	// pick two random instructions, don't pick the same
	while (loc1 == loc3)
	{
		loc1 = number->uniform_intgen(0, length[ind] - 1);
		loc3 = number->uniform_intgen(0, length[ind] - 1);
	}
	BYTE4 tmp = *(population[ind] + loc1);
	*(population[ind] + loc1) = *(population[ind] + loc3);
	*(population[ind] + loc3) = tmp;
	calculateFitness(ind);
	// Note: It does not change the individual length
};

/***********************************************************
 * Swap two instructions in an individual
 * maintain OWC
 */
void Population::swapMutation_ND(int ind)
{
	
	if(length[ind] < 5) return; //  O X W X C you need 5 instructions to make this work
	int loc1 = 0, loc3 = 0;
	int o = -1, w = -1, c = -1;
	find_OWC(ind, &o, &w, &c);
	//cout<<"in SW "<<o<<"-"<<w<<"-"<<c<<endl;
	// pick two random instructions, don't pick the same
	while ( (loc1 == loc3) || (loc1 == o) || (loc1 == w) || (loc1 == c) || (loc3 == o) || (loc3 == w) || (loc3 == c) )
	{
		loc1 = number->uniform_intgen(0, length[ind] - 1);
		loc3 = number->uniform_intgen(0, length[ind] - 1);
		//cout<<"l1="<<loc1<<" l3="<<loc3<<endl;
	}
	BYTE4 tmp = *(population[ind] + loc1);
	*(population[ind] + loc1) = *(population[ind] + loc3);
	*(population[ind] + loc3) = tmp;
	calculateFitness(ind);
	//cout<<"out SW"<<endl;
	// Note: It does not change the individual length
};

/***********************************************************
 * Copy source individual to destination
 * 
 */
void Population::reproduce(int src, int dst)
{
	delete[] population[dst];
	population[dst] = new BYTE4[length[src]];
	 
	int sloc = src*pagesize*numpages;
	int dloc = dst*pagesize*numpages;
	memcpy( population[dst],  population[src],  length[src] * sizeof(BYTE4) );
	
	
	/// Important: You should update all the individual characteristics that you use here
	fitness[dst] = fitness[src];
	length[dst] = length[src]; 
	anomaly[dst] = anomaly[src];
	rank[dst] = rank[src];
	success[dst] = success[src];
	delay[dst] = delay[src];
	raw_anomaly[dst] = raw_anomaly[src];
};


/***********************************************************
 * 
 * 
 */
void Population::greedySearch()
{
	// 1.  Find the best individual 
	double max_fit = 0;
	int max_fit_loc = -1;
	vector<int> bests;
	for(int i=0; i<popsize; ++i)
	{
		if (fitness[i] > max_fit)
		{
			bests.clear();
			max_fit = fitness[i];
			max_fit_loc = i;
			bests.push_back(max_fit_loc);
		}
		else if (fitness[i] == max_fit)
		{
			max_fit_loc = i;
			bests.push_back(max_fit_loc);
		}
	}
	// 1.5 check if max fit is found
	if (bests.size() == 0)
	{
		cout<<"greedySearch(): Failed to determine max fitness... returning."<<endl;
		return;
	}
	// 1.6 Pick a random best and set max_fit_loc
	if (bests.size() == 1)
	{
		max_fit_loc = bests.at(0);
	}
	else
	{
		int bstloc = number->uniform_intgen(0, bests.size() - 1);
		max_fit_loc = bests.at(bstloc);
	}
	// 2. for each inst loc
	double max_fit2 = fitness[max_fit_loc];
	int ival = -1, jval = -1;
	
	for (int i=0; i<length[max_fit_loc]; ++i)
	{
		//int ins_ptr = max_fit_loc*pagesize*numpages + i;
		BYTE4 old_instr = 0x00000000;
		double old_fit = fitness[max_fit_loc];
		for(int j=0; j< decoder->INS_len; ++j)
		{
			old_instr = *(population[max_fit_loc] + i);
			old_fit = fitness[max_fit_loc];
			*(population[max_fit_loc] + i) = decoder->specific_instruction(j);
			calculateFitness(max_fit_loc);
			if (fitness[max_fit_loc] > max_fit2)
			{
				max_fit2 = fitness[max_fit_loc];
				ival = i;
				jval = j; 
			}
			*(population[max_fit_loc] + i) = old_instr;
			fitness[max_fit_loc] = old_fit;
		}
	}
	// 3. check ival jval and make the one instruction change that gives the highest boost
	if ( (ival != -1) && (jval != -1) )
	{
		//int ins_ptr = max_fit_loc*pagesize*numpages + ival;
		*(population[max_fit_loc] + ival) = decoder->specific_instruction(jval);
		calculateFitness(max_fit_loc);
	}
};


/***********************************************************
 * 
 * 
 */
int Population::initializeTournament(int *tournament, int size)
{
	int *tmp =  new int[size]; assert(tmp); 
	double MIN_FITNESS = -DBL_MAX; // from float.h
	// Choose 4 different individuals 
	for(int i=0;i<size;++i)
	{
		tmp[i] = number->uniform_intgen(0, popsize - 1);
		for(int j=0;j<i;++j)
		{
			if (tmp[j] == tmp[i]) i--;
		}
	}
	// for(int k=0;k<size; ++k)
	//		cout<<" t="<<tmp[k]<<" f="<<fitness[tmp[k]]<<endl;
	// Sort them according to fitness
	for(int i=0;i<size;++i)
	{
		int max = -1;
		double max_fit = MIN_FITNESS;
		for(int j=0;j<size;++j)
		{
			if ( (tmp[j] != -1) && (fitness[tmp[j]] > max_fit) ) 
			{	
				max = j;
				max_fit = fitness[tmp[j]];
			}
		}
		tournament[i] = tmp[max];
		//cout<<"T="<<tournament[i]<<" F="<<fitness[tournament[i]]<<endl;
		tmp[max] = -1;
	}
	delete[] tmp;
	return 0;
};

/***********************************************************
 * 
 * 
 */
int Population::initializeTournament_pareto(int *tournament, int size)
{
	// Assumption: tournament is allocated 
	// Choose size/2  different individuals but not from the overflow section
	for(int i=0;i<size/2;++i)
	{
		tournament[i] = (int) selection.getValue(number->ran3());
		for(int j=0;j<i;++j)
		{
			if (tournament[j] == tournament[i]) i--;
		}
	}
	// Lower half is set to point to the overflow individuals
	for(int i=0;i<size/2;++i)
	{
		tournament[i + size/2] = popsize + i;
	}
	return 0;
};


/***********************************************************
 * This is the wrapper for fitness calculation, no matter 
 * how you calculate it, it should be returned from this
 */
double Population::calculateFitness(int ind)
{
	// This function needs to cal the proper fitness function based on the detector and 
	// the fitness function type provided
	
	double arate = 0;
	double arate2 = 0;
	double fit = 0;
	double ind_success = -1;
	double p_delay = 0;
	if (det_prefix == "stide")
	{	
		if (fitness_fn == 1)
		{
			//cout<<"fitness 1"<<endl;
			fit = calc_fitness(ind, arate, ind_success);
		}
		else if (fitness_fn == 2)// second fn
		{
			//cout<<"fitness 2"<<endl;
			fit = calc_fitness2(ind, arate, ind_success);
		}
		else // third function
		{
			fit = calc_fitness3(ind, arate, ind_success);
		}
		arate2 = calculateAnomaly_withoutPreamble(ind);
	}else if (det_prefix == "pH")
	{
		// as of v3.8 we only use the type 1 fitness function since it seems to work fine,
		// we won't 3 types as we did with stide
		fit = calc_fitness_pH(ind, arate, ind_success, p_delay);
		arate2 = calculateAnomaly_withoutPreamble_pH(ind);
	}
	else if (det_prefix == "pHmr")
	{
		// mimicry ressistant pH
		
			// uses the training schema
			fit = calc_fitness_pHmr2(ind, arate, ind_success, p_delay, phmr_is_training);
			arate2 = calculateAnomaly_withoutPreamble_pHmr2(ind, phmr_is_training);
		
		
	}
	else if (det_prefix == "hmm")
	{
		fit = calc_fitness_hmm(ind, arate, ind_success);
		arate2 = calculateAnomaly_withoutPreamble_hmm(ind);
	}
	else if (det_prefix == "NN")
	{
		fit = calc_fitness_NN(ind, arate, ind_success, p_delay); // use arate for distance and p_delay for maximum distance
		arate2 = calculateAnomaly_withoutPreamble_NN(ind);
	}
	else
	{
		cout<<"ERROR: Detector '"<<det_prefix<<"' not recognized by the fitness function. Exitting."<<endl;
		exit(1);
	}
	fitness[ind] = fit;
	anomaly[ind] = arate;
	raw_anomaly[ind] = arate2;
	success[ind] = ind_success;
	delay[ind] = p_delay;
	return fit;
};



/***********************************************************
 * This is for the final reporting, report the anomaly rates without the preamble
 * 3.8: It is no more for final reporting, I will include raw anomaly in pareto ranking 
 */
double Population::calculateAnomaly_withoutPreamble(int ind)
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "Stide/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	double ret = 0;
	double arate = 100; // worst case scenario
	//cout<<valid<<endl;

	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("Stide/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string command2 = "./Stide/stide -c Stide/stide.config -d Stide/"+app_prefix+".db < "+run_id+".processed > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id + ".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	string history;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			
			if (line.find("Percentage anomalous") != string::npos)
			//if (line.find("Number of anomalies") != string::npos)
			{
				
				int idx = line.find("=");
				anomaly = line.substr(idx + 2, line.size() - idx - 2);
				//cout<<"anom="<<anomaly.c_str()<<endl;
				done = 1;
				//cout<<history<<endl;
				//cout<<"anom = "<<anomaly<<endl;;
				
				//cout<<"anomaly ="<<anomaly<<"<"<<endl;
			}
			history += line; history+="\n";
			line = "";
		}
	}
	if (anomaly.find("nan") != string::npos)
	{
		cout<<"NaN anomaly"<<endl;
		arate = 100;
	}
	else
	{
		arate = atof(anomaly.c_str());
	}
	if (arate == 0)
		cout<<"----0 Rate -----"<<history<<endl;
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	history = "";
	//system(command3.c_str());
	fin.close();
	//if (arate < 10) cout<<"fit5 anom: "<<arate<<endl;
	return (arate);
};

/***********************************************************
 * This is for the final reporting, report the anomaly rates without the preamble
 * 3.8: It is no more for final reporting, I will include raw anomaly in pareto ranking 
 * for pH detector
 */
double Population::calculateAnomaly_withoutPreamble_pH(int ind)
{
	// some additional variable declaratins to make this function work
	// these params were passed by ref in original fitness calc fn.
	double anomrate = 0;
	int success = 0;
	double delay = 0; // in the future you might need to return this
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "../resources/pH/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("../resources/pH/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	// note that below,  <win_size (9)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pH  ../resources/pH/"+app_prefix+".db  "+run_id+".processed  9 1 12 30 10 172800 128 > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	// also it returns the anomaly rate as opposed to fitness in fitness cals
	return (anomrate);
};

/***********************************************************
 * This is for the final reporting, report the anomaly rates without the preamble
 * 3.8: It is no more for final reporting, I will include raw anomaly in pareto ranking 
 * for pH detector
 */
double Population::calculateAnomaly_withoutPreamble_hmm(int ind)
{
	// some additional variable declaratins to make this function work
	// these params were passed by ref in original fitness calc fn.
	double anomrate = 0;
	int success = 0;
	double delay = 0; // in the future you might need to return this
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "hmm/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("hmm/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string command2 = "./hmm/HMM  hmm/"+app_prefix+".db  "+run_id+".processed  > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				string flags = line.substr(0, idx1);
				string total = line.substr(idx1+1, idx2 - idx1 -1);
				arate = (double)((double) atoi(flags.c_str()) / (double) atoi(total.c_str())); 
				if (isnan(arate)) arate = 1;
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	// also it returns the anomaly rate as opposed to fitness in fitness cals
	return (anomrate);
};

/***********************************************************
 *
 *
 *
 */
double Population::calculateAnomaly_withoutPreamble_NN(int ind)
{
	double distance = 0;
	double max_dist = 0;
	int success = 0;
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "NN/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("NN/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	prepare_NN_input(run_id+".processed");
	string command2 = "./NN/neural_net_sim NN/DBs/"+app_prefix+".iw NN/DBs/"+app_prefix+".lw NN/DBs/"+app_prefix+".b1 NN/DBs/"+app_prefix+".b2 300 15 "+run_id+".processed_nninput   > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				string str_distance = line.substr(0, idx1);
				string str_max_dist = line.substr(idx1+1, idx2 - idx1 -1);
				distance = atof(str_distance.c_str());
				max_dist = atof(str_max_dist.c_str());
				
				
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	// NN returns a value between 0 and sqrt(1200 (300 dims variying between -1 and 1)), converting it to percentage
	distance = (distance / (double) sqrt((double)1200))*100; // normalize the distance to vary between 0 and 1
	ret += (double) (100 - distance) / (double) 20; // normalize it to 5
	
	return (distance);
}



/***********************************************************
 * This is for the final reporting, report the anomaly rates without the preamble
 * 3.8: It is no more for final reporting, I will include raw anomaly in pareto ranking 
 * for pH detector
 *
 
 ///////////////////////
 /////// RETIRED!!! This function has a methodology mistake,  won't produce meaningful results
double Population::calculateAnomaly_withoutPreamble_pHmr(int ind)
{
	cout<<"You are using a retired function, results are not meaningful!!!"<<endl;
	// some additional variable declaratins to make this function work
	// these params were passed by ref in original fitness calc fn.
	double anomrate = 0;
	int success = 0;
	double delay = 0; // in the future you might need to return this
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "pHmr/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("pHmr/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	// note that below,  <win_size (9)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pHmr/pHmr  pHmr/"+app_prefix+".db  "+run_id+".processed  20 1 12 30 10 172800 128 pHmr/schema.dat> "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	// also it returns the anomaly rate as opposed to fitness in fitness cals
	return (anomrate);
};
*/

/***********************************************************
 * This is for the final reporting, report the anomaly rates without the preamble
 * 3.8: It is no more for final reporting, I will include raw anomaly in pareto ranking 
 * for pH detector
 * Second function: uses the training schema
 */
double Population::calculateAnomaly_withoutPreamble_pHmr2(int ind, int is_training_schema)
{
	// some additional variable declaratins to make this function work
	// these params were passed by ref in original fitness calc fn.
	double anomrate = 0;
	int success = 0;
	double delay = 0; // in the future you might need to return this
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "pHmr/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("pHmr/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string blurb = "";
	if (is_training_schema)
		blurb = "_training";
	// note that below,  <win_size (9)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pHmr/pHmr  pHmr/"+app_prefix+".db  "+run_id+".processed  20 1 12 30 10 172800 128 pHmr/schema"+blurb+".dat > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	// also it returns the anomaly rate as opposed to fitness in fitness cals
	return (anomrate);
};




/***********************************************************
 * Reporting function: average fitness
 * 
 */

double Population::meanFitness()
{
	double average = 0;
	for(int i=0; i<popsize; ++i)
		average += fitness[i];
	return average/ (double) popsize;
};


/***********************************************************
 * 
 * 
 */
int Population::unique_count()
{
	int same[popsize];
	for (int ii=0; ii<popsize; ++ii) same[ii] = 0;
	int samecount = 0;
	string s1, s2;
	for (int ii=0; ii<popsize; ++ii)
	{
		s1 = print_individual(ii);
		for (int jj=0; jj<popsize; ++jj)
		{
			if (ii != jj)
			{
				s2 = print_individual(jj);
				if ( s1 == s2 ) { same[ii]++; break; }//since we dont care how many of the same
			}
		}
	}
	for (int ii=0; ii<popsize; ++ii)
	{
		if (same[ii] != 0) { samecount++; }
	}
	return popsize-samecount;
};

/***********************************************************
 * 
 * 
 */
void Population::update_selectionPDF()
{
	for (int j=0; j<popsize; ++j)
	{
		counts.at(j) = max_rank - rank[j] + 1; // count = maximum rank encountered minus the current rank (smaller rank gets the higher count)
	}
	selection.initialize(values, counts);
	//selection.printPDF_CDF();
};

/***********************************************************
 * 
 * 
 */
void Population::update_rankPDFs()
{
	vector<double> rank_vals;
	vector<double> rank_cnts;
	//cout<<max_rank<<endl;
	for (int j=0; j<max_rank; ++j)
	{
		rank_vals.push_back(j+1); // loc 0 = rank 1
		rank_cnts.push_back(0);
	}
	for (int j=0; j<popsize; ++j)
	{
		//cout<<"accesing "<<rank[j]-1<<" at "<<j<<endl;
		rank_cnts.at((int)rank[j] - 1) = rank_cnts.at((int)rank[j] - 1) + 1; // loc 0 = rank 1
	}
	rank_histogram_prev = rank_histogram;
	rank_histogram.initialize(rank_vals, rank_cnts); 
};

/***********************************************************
 * 
 * 
 */
void Population::replace()
{
	//cout<<"here1"<<endl;
	// 1. Find the worst 2 (overflow) individuals, store it in worst
	vector<int> I;
	vector<int> R;
	int cur_rank = max_rank;
	int selection;
	while(I.size() < overflow)
	{
		vector<int> rnk;
		for (int i=0; i<popsize; ++i)
		{
			if (rank[i] == cur_rank) rnk.push_back(i);		
		}
		while((!rnk.empty()) && (I.size() < overflow))
		{
			selection = number->uniform_intgen(0, rnk.size()-1);
			I.push_back(rnk.at(selection));
			R.push_back(cur_rank);
			rnk.erase(rnk.begin() + selection);
		}
		cur_rank--;
	}
	//cout<<"here2"<<endl;
	//2. Find the ranks of overflow section (500 - 501)
	for(int i=popsize; i<popsize + overflow; ++i)
	{
		I.push_back(i);
		R.push_back(rankOf(i));
	}
	//cout<<"here3"<<endl;
	int moved = 0;
	//for (int i=0;i<I.size();++i)
	//{
	//	cout<<I.at(i)<<"("<<R.at(i)<<") ---\n";
	//}
	
	
	vector<int> I2;
	vector<int> R2;
	while (moved < overflow)
	{
		int mx_rank = 0;
		int mx_indx = -1;
		int v_index = -1;
		for (int i=0; i<I.size(); ++i)
		{
			if (R.at(i) > mx_rank)
			{
				mx_rank = R.at(i);
				mx_indx = I.at(i);
				v_index = i;
				//cout<<mx_rank<<" -- "<<mx_indx<<" -- "<<v_index<<"\n";
			}
		}
		I2.push_back(mx_indx);
		R2.push_back(mx_rank);
		I.erase(I.begin() + v_index);
		R.erase(R.begin() + v_index);
		moved++;
	}
	//cout<<"here4"<<endl;
	// I2 has the higher ranked inds and I has the lower ranked
	//3. reporoduce and update ranks
	for (int i =0; i<I2.size(); ++i)
	{
		//cout<<I.at(i)<<"("<<R.at(i)<<") -> "<<I2.at(i)<<"("<<R2.at(i)<<")\n";
		reproduce(I.at(i), I2.at(i));
	}
		//cout<<"here5"<<endl;
	
};

/***********************************************************
 * THIS FUNCTION IS RETIRED -- PROBLEMS
 * 1. Binary representation between Intel and Mac makes this function useless
 * 2. It still uses the old population array
void Population::writePopulation(string filename)
{
	FILE *fp;
	fp = fopen(filename.c_str(), "wb");
	fwrite(population, sizeof(BYTE4), popsize*numpages*pagesize, fp);
	fclose(fp);
};
*/

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation 1: This function calculates validity and anom rate. Only successful attacks 
// can get brownie points for anom rate

double Population::calc_fitness(int ind, double &anomrate, double &success)
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "Stide/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	//cout<<valid<<endl;
	if (valid >= 5)//valid != 0)
	{
		char *t_arg2 = new char[run_id.size() + 1];
		strcpy(t_arg2,run_id.c_str());
		process_strace((char*)string("Stide/"+app_prefix+".enum").c_str(), t_arg2);
		delete[] t_arg2;
		string command2 = "./Stide/stide -c Stide/stide.config -d Stide/"+app_prefix+".db < "+run_id+".processed2 > "+run_id+".outcome";
		system(command2.c_str());
		// now process .outcome file
		string fin_file = run_id + ".outcome"; 
		ifstream fin(fin_file.c_str());
		assert(fin);
		char ch;
		string line = "";
		string anomaly;
		int done = 0;
		string history;
		while (fin.get(ch) && !done)
		{
			if( ch !='\n') 
				line+= ch;
			else // process the line
			{
				
				if (line.find("Percentage anomalous") != string::npos)
				//if (line.find("Number of anomalies") != string::npos)
				{
					
					int idx = line.find("=");
					anomaly = line.substr(idx + 2, line.size() - idx - 2);
					//cout<<"anom="<<anomaly.c_str()<<endl;
					done = 1;
					//cout<<history<<endl;
					//cout<<"anom = "<<anomaly<<endl;;
					
					//cout<<"anomaly ="<<anomaly<<"<"<<endl;
				}
				history += line; history+="\n";
				line = "";
			}
		}
		if (anomaly.find("nan") != string::npos)
		{
			cout<<"NaN anomaly"<<endl;
			arate = 100;
		}
		else
		{
			arate = atof(anomaly.c_str());
		}
		if (arate == 0)
			cout<<"----0 Rate -----"<<history<<endl;
		// Clean-up
		string command3 = "rm -f "+run_id+"*";
		history = "";
		//system(command3.c_str());
		fin.close();
		//if (arate < 10) cout<<"fit5 anom: "<<arate<<endl;
	}
	ret = (double) valid;
	ret += (double) (100 - arate) / (double) 20; // normalize it to 5
	anomrate = arate;
	return (ret);
};

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation 1: This function calculates validity and anom rate. Only successful attacks 
// can get brownie points for anom rate

double Population::calc_fitness(string phen, string db, double &anomrate, double &success, int with_preamble)
{
	//cout<<"calculating..."<<endl;
	//string phen = print_individual(ind);
	string run_id = "Stide/analyzer-run";
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid = 5;
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	//cout<<valid<<endl;
	if (valid >= 5)//valid != 0)
	{
		char *t_arg2 = new char[run_id.size() + 1];
		strcpy(t_arg2,run_id.c_str());
		process_strace((char*)string("Stide/"+app_prefix+".enum").c_str(), t_arg2);
		delete[] t_arg2;
		string blurb;
		if (with_preamble == 1) blurb ="2";
		string command2 = "./Stide/stide -c Stide/stide.config -d "+db+" < "+run_id+".processed"+blurb+" > "+run_id+".outcome";
		system(command2.c_str());
		// now process .outcome file
		string fin_file = run_id + ".outcome"; 
		ifstream fin(fin_file.c_str());
		assert(fin);
		char ch;
		string line = "";
		string anomaly;
		int done = 0;
		string history;
		while (fin.get(ch) && !done)
		{
			if( ch !='\n') 
				line+= ch;
			else // process the line
			{
				
				if (line.find("Percentage anomalous") != string::npos)
				//if (line.find("Number of anomalies") != string::npos)
				{
					
					int idx = line.find("=");
					anomaly = line.substr(idx + 2, line.size() - idx - 2);
					//cout<<"anom="<<anomaly.c_str()<<endl;
					done = 1;
					//cout<<history<<endl;
					//cout<<"anom = "<<anomaly<<endl;;
					
					//cout<<"anomaly ="<<anomaly<<"<"<<endl;
				}
				history += line; history+="\n";
				line = "";
			}
		}
		if (anomaly.find("nan") != string::npos)
		{
			cout<<"NaN anomaly"<<endl;
			arate = 100;
		}
		else
		{
			arate = atof(anomaly.c_str());
		}
		if (arate == 0)
			cout<<"----0 Rate -----"<<history<<endl;
		// Clean-up
		string command3 = "rm -f "+run_id+"*";
		history = "";
		//system(command3.c_str());
		fin.close();
		//if (arate < 10) cout<<"fit5 anom: "<<arate<<endl;
	}
	ret = (double) valid;
	ret += (double) (100 - arate) / (double) 20; // normalize it to 5
	anomrate = arate;
	return (ret);
};



///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation 2: this function calculates validity and anom rate and adds them together
// without considering the success of the attack.
//
//

double Population::calc_fitness2(int ind, double &anomrate, double &success)
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "Stide/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("Stide/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string command2 = "./Stide/stide -c Stide/stide.config -d Stide/"+app_prefix+".db < "+run_id+".processed2 > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			if (line.find("Percentage anomalous") != string::npos)
			//if (line.find("Number of anomalies") != string::npos)
			{
				int idx = line.find("=");
				anomaly = line.substr(idx + 2, line.size() - idx - 2);
				//cout<<"anom="<<anomaly.c_str()<<endl;
				done = 1;
				//cout<<"anomaly ="<<anomaly<<"<"<<endl;
			}
			line = "";
		}
	}
	if (anomaly.find("nan") != string::npos)
	{
		cout<<"NaN anomaly"<<endl;
		arate = 100;
	}
	else
	{
		arate = atof(anomaly.c_str());
	}
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	//cout<<"fit5 anom: "<<arate<<" valid:"<<valid<<endl;
	ret = (double) valid;
	ret += (double) (100 - arate) / (double) 20; // normalize it to 5
	anomrate = arate;
	//
	//if ((arate == 100) && (ret > 5))
	//{
	//	cout<<"should not happen 1"<<endl;
	//}
	//if ((arate < 100) && (ret == 5))
	//{
	//	cout<<"should not happen 2"<<endl;
	//}
	//
	return (ret);
}





///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation 3: this function calculates validity and anom rate and adds them together
// without considering the success of the attack.
//       IT IS FITNESS 2 BUT DOES NOT ACCOUNT FOR THE VALIDITY 
//       see ret = (double) valid; line

double Population::calc_fitness3(int ind, double &anomrate, double &success)
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "Stide/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("Stide/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string command2 = "./Stide/stide -c Stide/stide.config -d Stide/"+app_prefix+".db < "+run_id+".processed2 > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			if (line.find("Percentage anomalous") != string::npos)
			//if (line.find("Number of anomalies") != string::npos)
			{
				int idx = line.find("=");
				anomaly = line.substr(idx + 2, line.size() - idx - 2);
				//cout<<"anom="<<anomaly.c_str()<<endl;
				done = 1;
				//cout<<"anomaly ="<<anomaly<<"<"<<endl;
			}
			line = "";
		}
	}
	if (anomaly.find("nan") != string::npos)
	{
		cout<<"NaN anomaly"<<endl;
		arate = 100;
	}
	else
	{
		arate = atof(anomaly.c_str());
	}
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	//cout<<"fit5 anom: "<<arate<<" valid:"<<valid<<endl;
	ret = (double) 0;//valid;
	ret += (double) (100 - arate) / (double) 20; // normalize it to 5
	anomrate = arate;
	//
	//if ((arate == 100) && (ret > 5))
	//{
	//	cout<<"should not happen 1"<<endl;
	//}
	//if ((arate < 100) && (ret == 5))
	//{
	//	cout<<"should not happen 2"<<endl;
	//}
	//
	return (ret);
}
///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for pH
//       
double Population::calc_fitness_pH(int ind, double &anomrate, double &success, double &delay)
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "../resources/pH/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("../resources/pH/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	// note that below,  <win_size (9)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pH  ../resources/pH/"+app_prefix+".db  "+run_id+".processed2  9 1 12 30 10 172800 128 > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for pH
//       
double Population::calc_fitness_pH(string phen, string db, double &anomrate, double &success, double &delay, int with_preamble)
{
	//cout<<"calculating..."<<endl;
	//string phen = print_individual(ind);
	string run_id = "pH/analyzer-run";
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  5;
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("pH/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string blurb;
	if (with_preamble == 1) blurb ="2";
	// note that below,  <win_size (9)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pH/pH  "+db+"  "+run_id+".processed"+blurb+"  9 1 12 30 10 172800 128 > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for HMM
//       
double Population::calc_fitness_hmm(int ind, double &anomrate, double &success)
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "hmm/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("hmm/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string command2 = "./hmm/HMM  hmm/"+app_prefix+".db  "+run_id+".processed2   > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				string flags = line.substr(0, idx1);
				string total = line.substr(idx1+1, idx2 - idx1 -1);
				arate = (double)((double) atoi(flags.c_str()) / (double) atoi(total.c_str())); 
				if (isnan(arate)) arate = 1;
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for HMM
//       (FOR USE WITH ANALYZER.CPP)
double Population::calc_fitness_hmm(string phen, string db, double &anomrate, double &success, int with_preamble)
{
	//cout<<"calculating..."<<endl;
	//string phen = print_individual(ind);
	string run_id = "hmm/analyzer-run";
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  5;
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("hmm/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string blurb;
	if (with_preamble == 1) blurb ="2";
	string command2 = "./hmm/HMM  "+db+"  "+run_id+".processed"+blurb+"   > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				string flags = line.substr(0, idx1);
				string total = line.substr(idx1+1, idx2 - idx1 -1);
				arate = (double)((double) atoi(flags.c_str()) / (double) atoi(total.c_str())); 
				if (isnan(arate)) arate = 1;
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}




///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for NN
//       
double Population::calc_fitness_NN(int ind, double &distance, double &success, double &max_dist) // using arate for distance and delay for max distance
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "NN/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("NN/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	prepare_NN_input(run_id+".processed2");
	string command2 = "./NN/neural_net_sim NN/DBs/"+app_prefix+".iw NN/DBs/"+app_prefix+".lw NN/DBs/"+app_prefix+".b1 NN/DBs/"+app_prefix+".b2 300 15 "+run_id+".processed2_nninput   > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				string str_distance = line.substr(0, idx1);
				string str_max_dist = line.substr(idx1+1, idx2 - idx1 -1);
				distance = atof(str_distance.c_str());
				max_dist = atof(str_max_dist.c_str());
				
				
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	// NN returns a value between 0 and sqrt(1200 (300 dims variying between -1 and 1)), converting it to percentage
	distance = (distance / (double) sqrt((double) 1200))*100; // normalize the distance to vary between 0 and 1 and x100 to make it an anom rate
	ret += (double) (100 - distance) / (double) 20; // normalize it to 5
	
	return (ret);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for NN
//       
double Population::calc_fitness_NN(string phen, string db, double &distance, double &success, double &max_dist, int with_preamble) // using arate for distance and delay for max distance
{
	cout<<"- This function is created on Aug 21 from asmGPsys3.9, in the future scaling may be required on this function output."<<endl;
	cout<<"- with_preamble function not implemented."<<endl;
	
	//cout<<"calculating..."<<endl;
	//string phen = print_individual(ind);
	string run_id = "NN/analyzer-run";
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  5;
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("NN/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	prepare_NN_input(run_id+".processed2");
	cout<<"command2 variable may need checking"<<endl;
	string command2 = "./NN/neural_net_sim "+db+".iw "+db+".lw "+db+".b1 "+db+".b2 300 15 "+run_id+".processed2_nninput   > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				string str_distance = line.substr(0, idx1);
				string str_max_dist = line.substr(idx1+1, idx2 - idx1 -1);
				distance = atof(str_distance.c_str());
				max_dist = atof(str_max_dist.c_str());
				
				
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	// NN returns a value between 0 and sqrt(1200 (300 dims variying between -1 and 1)), converting it to percentage
	distance = (distance / (double) sqrt((double) 1200))*100; // normalize the distance to vary between 0 and 1 and x100 to make it an anom rate
	ret += (double) (100 - distance) / (double) 20; // normalize it to 5
	
	return (ret);
}




///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for pH
//
///////////////////////
/////// RETIRED!!! This function has a methodology mistake,  won't produce meaningful results       
/*
double Population::calc_fitness_pHmr(int ind, double &anomrate, double &success, double &delay)
{
	//cout<<"calculating..."<<endl;
	cout<<"RETIRED!!! This function has a methodology mistake,  won't produce meaningful results  "<<endl;
	string phen = print_individual(ind);
	string run_id = "pHmr/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("pHmr/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	// note that below,  <win_size (20)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pHmr/pHmr  pHmr/"+app_prefix+".db  "+run_id+".processed2  20 1 12 30 10 172800 128 pHmr/schema.dat > "+run_id+".outcome";
	//cout<<command2<<endl;
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for pH
// Second function: uses the training schema      
double Population::calc_fitness_pHmr2(int ind, double &anomrate, double &success, double &delay, int is_training_schema) // is_training_schema 1 for training, 0 for testing
{
	//cout<<"calculating..."<<endl;
	string phen = print_individual(ind);
	string run_id = "pHmr/" + runID;
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  check_valid(ind);
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("pHmr/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string blurb = "";
	if (is_training_schema)
		blurb = "_training";
	// note that below,  <win_size (20)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string command2 = "./pHmr/pHmr  pHmr/"+app_prefix+".db  "+run_id+".processed2  20 1 12 30 10 172800 128 pHmr/schema"+blurb+".dat > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Fitness calculation for pH (FOR USE WITH ANALYZER.CPP)
// Second function: uses the training schema      
double Population::calc_fitness_pHmr2(string phen, string db, double &anomrate, double &success, double &delay, int is_training_schema, int with_preamble) // is_training_schema 1 for training, 0 for testing
{
	//cout<<"calculating..."<<endl;
	//string phen = print_individual(ind);
	string run_id = "pHmr/analyzer-run";
	ofstream fout(run_id.c_str());
	fout<<phen.c_str();//phen;
	fout.close();
	int syscall_count = 0;
	//string command1 = "cat ./syscall.blurb partial.prog > test.prog ";
	//system(command1.c_str());
	int valid =  5;
	success = valid;
	double ret = 0;
	double arate = 100; // worst case scenario
	char *t_arg2 = new char[run_id.size() + 1];
	strcpy(t_arg2,run_id.c_str());
	process_strace((char*)string("pHmr/"+app_prefix+".enum").c_str(), t_arg2);
	delete[] t_arg2;
	string blurb = "";
	if (is_training_schema)
		blurb = "_training";
	// note that below,  <win_size (20)> <delay factor (1)> <tolerize_lim (12)> <anomaly_lim (30)> <suspend_execve (10)> <suspend_execve_time (172800)> <lfc 128>
	string blurb2;
	if (with_preamble == 1) blurb2 ="2";
	string command2 = "./pHmr/pHmr  "+db+"  "+run_id+".processed"+blurb+"  20 1 12 30 10 172800 128 pHmr/schema"+blurb2+".dat > "+run_id+".outcome";
	system(command2.c_str());
	// now process .outcome file
	string fin_file = run_id+".outcome"; 
	ifstream fin(fin_file.c_str());
	assert(fin);
	char ch;
	string line = "";
	string anomaly;
	int done = 0;
	int line_count = 0;
	int header_read = 0;
	while (fin.get(ch) && !done)
	{
		if( ch !='\n') 
			line+= ch;
		else // process the line
		{
			line_count++;
			if(header_read == 1)
			{
				// assumes the line after header contains the data we need
				int idx1 = line.find(",", 0);
				int idx2 = line.find(",", idx1+1);
				int idx3 = line.find(",", idx2+1);
				int idx4 = line.find(",", idx3+1);
				int idx5 = line.find(",", idx4+1);
				int idx6 = line.find(",", idx5+1);
				string mismatch = line.substr(0, idx1);
				string total_ct = line.substr(idx1+1, idx2 - idx1 -1);
				string delay_str = line.substr(idx2+1, idx3 - idx2 -1);
				string max_lfc = line.substr(idx3+1, idx4 - idx3 -1);
				string execve_stopped_at = line.substr(idx4+1, idx5 - idx4 -1);
				string sensitized_at = line.substr(idx5+1, idx6 - idx5 -1);
				string tolerized_at = line.substr(idx6+1);
				arate = (double)((double) atoi(mismatch.c_str()) / (double) atoi(total_ct.c_str())); 
				if (isnan(arate)) arate = 1;
				delay = atof(delay_str.c_str());
				// we don't need the rest of the file so...
				break;
			}
			if (line.find("# ") != string::npos)
			// First line with header
			{
				header_read = 1;
			}
			line = "";
			
		}
	}
	
	
	
	// Clean-up
	string command3 = "rm -f "+run_id+"*";
	//system(command3.c_str());
	fin.close();
	ret = (double) valid;
	anomrate = arate*100; // pH returns a value between 0 and 1, converting it to percentage
	ret += (double) (100 - anomrate) / (double) 20; // normalize it to 5
	
	return (ret);
}





///////////////////////////////////////////////////////////////////////////////////////////////
// process strace
//
//

void Population::process_strace(char *argv1, char *argv2)
{
	//cout<< argv1<<endl;
	//cout<< argv2<<endl;
	char ch;
	string line;
	ifstream fin2 (argv2);
	string outfile (argv2);
	string outfile2 (argv2);
	outfile  +=".processed";
	outfile2 +=".enum"; 
	ofstream fout(outfile.c_str());
	//ofstream fout2(outfile2.c_str());
	//assert(fin);
	assert(fin2);
	assert(fout);
	//assert(fout2);
	if (DEBUG)
	{
		for(int i=0;i<syscalls.size();++i)
			cout<<">>"<<syscalls.at(i)<<"<<"<<endl;
	}
	line = "";
	string pid = "1111"; // it is from the pre-attack blurb
	// okay, the if statement below is not neccesary since stide does not use pid
	// for detection but I wanted to address this as simple as possible. All 4 apps
	// ise the same pid both in preamble and the exploit. So the following code is 
	// a simple way to ensure that preamble pid matches with exploit.
	if (app_prefix == "traceroute")
	{
		pid = "796";
	}
	if (app_prefix == "ftp")
	{
		pid = "804";
	}
	if (app_prefix == "restore")
	{
		pid = "9033";
	}
	if (app_prefix == "samba")
	{
		pid = "926";
	}
	
	
	string sid;
	int sid2 = -1;
	//fin2.get(ch); // skip the first space
	while (fin2.get(ch))
	{
		//cout<<"["<<ch<<"]"<<endl;
		if( ch !=' ' && ch !='\n')
		{
			line+=ch;
		}
		else
		{
			//process token
	
			// is it a parameter? if not look it up in syscall table
			
			
			if ( (line !="") && (line.find("'") == string::npos ) && (line.find("buffer") == string::npos) )
			{
						if (DEBUG) cout<<"Token>"<<line<<"<"<<endl;
				// check if it is exit (if so sid = 1)
				//cout<<line<<endl;
				if (line.find("exit") != string::npos)
				{
					sid2 = 1; //exit
				}
				else // look it up 
				{
					for (int i=0;i<syscalls.size(); ++i)
					{
					 //cout<<line<<">"<<syscalls.at(i)<<"<"; 
						if (syscalls.at(i) == line)
						{
							sid2 = i + 1;
						}
					}
				}
				if (sid2 == -1)
				{
					syscalls.push_back(line);
					sid2 = syscalls.size(); // because we start from 1  
					cout<<"Warning (SHOULD NOT HAPPEN): Couldn't find a match for "<<line<<endl; 
				} 
			fout<<pid<<" "<<sid2<<endl;
			}
			line ="";
		}
	}
	
	
	sid2 = 1;
	fout<<pid<<" "<<sid2<<endl; // append exit at the end (taken out of GE)
	fin2.close();
	fout.close();
	// file is ready, now concatenate it with the blurb
	// make sure stide uses Stide/test.prog.processed2 instead of Stide/test.prog.processed
	// if you disable this uncomment the following:
	// string command_pre2 = "cp  Stide/test.prog.processed  Stide/test.prog.processed2";
	string run_id(argv2);
	string command_pre2 = "cat ../resources/pH/"+app_prefix+".pre "+run_id+".processed > "+run_id+".processed2";
	system(command_pre2.c_str());
	
	//fout2.close();
};


///////////////////////////////////////////////////////////////////////////////////////////////
// check valid
//
//

int Population::check_valid(int ind)
{
	int score = 0;
	//cout<<pst<<endl;
	int stages[] = {-1, -1, -1};
	find_OWC(ind, &stages[0], &stages[1], &stages[2]);

	// we have the location of attack commands
	
	score = 0;
	if (stages[0] != -1) ++score; // O exists 
	if (stages[1] != -1) ++score; // W exists
	if (stages[2] != -1) ++score; // C exists
	if ( (stages[0] != -1) && (stages[1] != -1) && (stages[0] < stages[1]) ) ++score; // OW exist and ordering valid
	if ( (stages[2] != -1) && (stages[1] != -1) && (stages[2] > stages[1]) ) ++score; // WC exist and ordering valid
	//if (score > 4 && DEBUG)
	//if (score < 5)	cout<<"   O:"<<stages[0]<<" W:"<<stages[1]<< "C:"<<stages[2]<<endl;
	//cout<<"line is >"<<line<<"<"<<endl;
	return score;
};


///////////////////////////////////////////////////////////////////////////////////////////////
// write_results()
//
//
void Population::write_results(string filename)
{
	ofstream fout(filename.c_str());

	// dump the individual statistics (fitness and anom rate)
	// like a lookup table to pinpoint the best individual 
	double max_fit = -1;
	int max_fit_i = -1;  
	double min_anom = 100;
	int min_anom_i = -1;
	double min_raw_anom = 100;
	int min_raw_anom_i = -1;
	for(int i=0; i<popsize; ++i)
	{
		if (fitness[i] > max_fit)
		{
			max_fit = fitness[i];
			max_fit_i = i;
		}
		if (anomaly[i] < min_anom)
		{
			min_anom = anomaly[i];
			min_anom_i = i;
		}
		if (raw_anomaly[i] < min_raw_anom)
		{
			min_raw_anom = raw_anomaly[i];
			min_raw_anom_i = i;
		}		
	}
	/*
	double min_arate_wp = 100;
	int min_arate_wp_idx = -1;
	double cur_awp;
	for(int i=0; i<popsize; ++i)
	{
		cur_awp = calculateAnomaly_withoutPreamble(i);
		if (cur_awp < min_arate_wp)
		{
			min_arate_wp = cur_awp;
			min_arate_wp_idx = i;
		}
	}
	*/
	fout<<"Best Fitness: "<<max_fit<<" at "<<max_fit_i<<endl;
	fout<<"Best Anomaly: "<<min_anom<<" at "<<min_anom_i<<endl;
	fout<<"Best Anomaly (wo Preamble): "<<min_raw_anom<<" at "<<min_raw_anom_i<<endl;
	fout<<"**********************************************"<<endl;
	fout<<"************* Population Summary *************"<<endl;
	fout<<"**********************************************"<<endl;
	fout<<"i\t fitness\t anomaly\t raw_anomaly\t rank\t success\t length\t delay\t"<<endl;

	for(int i=0; i<popsize; ++i)
	{
		//cur_awp = calculateAnomaly_withoutPreamble(i);
		fout<<i<<"\t "<<fitness[i]<<"\t "<<anomaly[i]<<"\t "<<raw_anomaly[i]<<"\t "<<rank[i]<<"\t "<<success[i]<<"\t "<<length[i]<<"\t "<<delay[i]<<endl;
	}
	fout<<"**********************************************"<<endl;
	fout<<"*************     Programs       *************"<<endl;
	fout<<"**********************************************"<<endl;
	// Now dump the individuals as system call sequences
	for(int i=0; i<popsize; ++i)
	{
		fout<<"***** ["<<i<<"] Ft="<<fitness[i]<<" An="<<anomaly[i]<<" rAn="<<raw_anomaly[i]<<" Dl="<<delay[i]<<endl;
		fout<<print_individual(i)<<endl;	
	}
	//////
	//
	// V3 NOTE: I don't think the code segment below is being used
	//   Bypassing it due to the old way it accesses population array
	//
	// dump the population in unsigned int format
	// where each line is one individual
	//	for(int j=0; j<popsize*numpages*pagesize; ++j)
	//	{
	//		if (j != 0) fout<<", ";
	//		fout<<(BYTE4) population[j];
	//		if (j % popsize == 0) fout<<"\n"; 
	//		
	//	}
	fout<<endl;
	fout.close();
};


///////////////////////////////////////////////////////////////////////////////////////////////
// check_results()
//
//
void Population::check_results(int tour)
{
	for (int i=0; i< popsize; ++i)
	{
		if ((fitness[i] > 5) && (anomaly[i] == 100) )
		{
			cout<<"ind"<<i<<" is wrong at tournament "<<tour<<endl;
			cout<<"fit="<<fitness[i]<<" anom="<<anomaly[i]<<endl;
		}
	}


}

///////////////////////////////////////////////////////////////////////////////////////////////
// find_OWC()
//
//
void Population::find_OWC(int ind, int *O, int *W, int *C)
{
	*O = -1; *W = -1; *C = -1;
	string pst = print_individual(ind);
	// find the first open /etc/passwd, last close /etc/passwd and a write in between
	// Maybe parameters should be optional
	////////////////////////////////////////////////////
	// SEARCH LOOP 1: Find the first open and last close
	int idx1 = 0;
	int idx2 = pst.find ("\n", idx1);
	string token;
	int line = 0;
	while (idx2 != string::npos)
	{
		token = pst.substr(idx1, idx2 - idx1);
		//cout<<token<<endl;
		
		if(token.find("open") != string::npos)
		{
			if (CHECK_PARAMS && token.find("/etc/passwd"))
			{
				if (*O == -1) *O = line; 
			}
			else
			{
				if (*O == -1) *O = line;
			}
		}
		if(token.find("close") != string::npos)
		{
			if (CHECK_PARAMS && token.find("/etc/passwd"))
			{
				if (*C < line ) *C = line; 
			}
			else
			{
				if (*C < line ) *C = line; 
			}
			
		}
		idx1 = idx2 + 1;
		idx2 = pst.find ("\n", idx1);
		line++;
	}
	idx1 = 0;
	idx2 = pst.find ("\n", idx1);
	token = "";
	line = 0;
	////////////////////////////////////////////////////
	// SEARCH LOOP 2: Find the any write in between
	while (idx2 != string::npos)
	{
		token = pst.substr(idx1, idx2 - idx1);
		//cout<<token<<endl;
		if(token.find("write") != string::npos)
		{
			if (CHECK_PARAMS && token.find("/etc/passwd"))
			{
				if ((*W == -1) || (*W > *C) || (*W < *O))  { *W = line; } // break while if one write after open before close is found
			}
			else
			{
				if ((*W == -1) || (*W > *C) || (*W < *O))  { *W = line; } // break while if one write  after open before close is found
			}
		}
		idx1 = idx2 + 1;
		idx2 = pst.find ("\n", idx1);
		line++;
	}
	
	idx1 = 0;
	idx2 = pst.find ("\n", idx1);
	token = "";
	line = 0;

};

///////////////////////////////////////////////////////////////////////////////////////////////
// updatePDF()
//
//
void Population::updatePDF(int ind)
{
	vector<double> vl;
	vector<double> cn;
	for (int j=0; j<decoder->INS.size; ++j)
	{
		vl.push_back(j);
		cn.push_back(0);
	}
	
	for(int j=0; j<length[ind]; ++j)
		{
			BYTE4 inst = *(population[ind] + j);
			BYTE4 in = (inst >> 16) & 0x0000FFFF; // from decoder.decode_instr
			cn.at(in - 1) = cn.at(in - 1) + 1; 		
		}
	distros[ind].initialize(vl, cn); // re initialize the corresponding PDF
	
};

 /***********************************************************
 *
 */
 int Population::get_max_rank()
 {
	int ret = 1;
	// max rank should be calculated for the popultion, not for the population and overflow
	for(int i=0; i<popsize; ++i)
	{
		if ((int)rank[i] > ret) ret = (int)rank[i];
	}
	return ret;
 };

 /***********************************************************
 *
 */
 void Population::pareto_rank2()
 {
	// implemented from Deb's NSGA2 paper
	// Setting the size to POP_SIZE constant.
	// Had issues with the below line with the modern
	// C++ (on Mac 10.10)
	vector<int> S[POP_SIZE]; //Set of solutions dominated by i
	vector<int> Front; //Current front
	vector<int> Q; // Next front
	int N[popsize]; //domination counter
	for(int i=0; i<popsize; ++i)
	{
		N[i] = 0;
		for(int j=0; j<popsize; ++j)
		{
			if(A_dominates_B(i, j) == 1) // i dominates j
				S[i].push_back(j);
			else if (A_dominates_B(j, i) == 1) // j dominates i
				N[i]++;
			else if (A_dominates_B(i, j) == -1) // i and j indifferent assign j the lower rank
				{if (i != j) S[i].push_back(j);}
		}
		if (N[i] == 0)
		{
			rank[i] = 1;
			Front.push_back(i);
		}	
	}
	int index = 1; // i = 1 in paper
	while (!Front.empty())
	{
		// Needed for Pareto rank histogram
		// Back to NSGA2
		Q.clear();
		for(int i=0; i<Front.size(); ++i)
		{
			for(int j=0; j<S[Front.at(i)].size(); ++j) // p = Front.at(i)
			{
				N[S[Front.at(i)].at(j)]--; //q = S[Front.at(i)].at(j)
				if (N[S[Front.at(i)].at(j)] == 0)
				{
					rank[S[Front.at(i)].at(j)] = index + 1;
					Q.push_back(S[Front.at(i)].at(j));
				}
			}
		}
		index ++;
		Front = Q;
	}
	// Exact implementation of Deb's algorithm ends here
	max_rank = index - 1;
	//max_rank = get_max_rank();
	//cout<<"Max rank: "<<max_rank<<endl;
 };

 /***********************************************************
 *
 */
 void Population::pareto_rank()
 {
	// implemented from Andy's thesis
	int outcome;
	int T[popsize][popsize];
	
	for(int i=0; i<popsize; ++i)
	{
		rank[i] = 1;
		for(int j=0; j<popsize; ++j)
		{
			T[i][j] = 0;
		}
	}
	for(int i=0; i<popsize; ++i)
	{
		for(int j=0; j<popsize; ++j)
		{
			outcome = A_dominates_B(i,j);
			if (outcome == -1) 
			{
				if ((i != j) && (T[i][j] == 0))
				{
					T[i][j]++;
					T[j][i]++;
					rank[j]++;
				}	
			}
			else
			{
				rank[j] = rank[j] + outcome;
			}
		}
	}
	max_rank = get_max_rank();
 };

 
 
/***********************************************************
 *
 */
 int Population::A_dominates_B(int indA, int indB)
 {
	// You have to know where the sub-ojectives are
	// in asmGP they are size of ind, success and anomrate
	// kind of hard coded for now I might generalize this later
	// 1: A dominates B, -1: indifferent, 0: otherwise
	// Algorithm implemented from Andy's thesis
	int flag = 0;
	if ( anomaly[indA] <= anomaly[indB] ) // anomaly: smaller better (changed to larger better)
	{
		flag = flag + ( anomaly[indA] < anomaly[indB] );
		if ( success[indA] >= success[indB] ) // success: larger better
		{
			flag = flag + ( success[indA] > success[indB] );
			if ( length[indA] >= length[indB] ) // length: larger better
			{
				flag = flag + ( length[indA] > length[indB] );
				if ( raw_anomaly[indA] <= raw_anomaly[indB] ) // raw anomaly: smaller  better
				{
					flag = flag + (  raw_anomaly[indA] <  raw_anomaly[indB] );
					if ( delay[indA] <= delay[indB] ) // delay: changed to smaller better
					{
						flag = flag + ( delay[indA] < delay[indB] );
					}
			
				}
				else { return 0; }
			}
			else { return 0; }
		}
		else { return 0; }		
	}
	else { return 0; } 
	if (flag > 0) // A dominates B
		return 1;
	else // A and be are equal in all objectives
		{ return -1;}
 };

 /***********************************************************
  *
  */
 int Population::rankOf(int ind)
 {
	int ret_rank = max_rank + 1; // set it to beyond max rank
	int outcome;
	for (int i=0; i<popsize; ++i)
	{
		if (ret_rank > rank[i])
		{
			outcome = A_dominates_B(ind, i); 
			if (outcome == 1)
				ret_rank = (int) rank[i];
			if (outcome == -1)
				ret_rank = (int) rank[i] + 1; // ** UNDER DEVELOPMENT ** : increment rank so that identical ones have different ranks.
		}
	}
	return ret_rank;
 };
 
 /***********************************************************
  *   For the pHmr, train the detecteor with the new schema mask and calculate fitness
  */
void Population::recalculateAllFitness()
 {
	for (int i=0; i<popsize; ++i)
	{
		calculateFitness(i);
	}
 };
 
  /***********************************************************
  *
  */
 void Population::prepare_NN_input(string filename)
 {
	double vect[300];
	for (int i=0; i<300; ++i)
		vect[i] = 0;
	ifstream fin(filename.c_str());
	assert(fin);
	char ch;
	string line = "";
	int count = 0;
	double total = 0;
	while (fin.get(ch))
	{
		if( (ch !='\n') && (ch != ' ') ) 
			line+= ch;
		else // process the token
		{
			int token = -1;
			if (line != "")
			{
				++count;
				token = atoi(line.c_str());
				//cout<<"token: "<<token<<endl;
		
				if(count % 2 == 0)
				{
					vect[token-1]++;
					total++;
				}
			}
			line = "";
		}
	}
	fin.close();
	// we have the counts and total, now normalize it
	for(int i=0; i<300; ++i)
	{
		vect[i] = vect[i] / total;
		if (isnan(vect[i])) cout<<"Warning: NaN found in input vector."<<endl;
	}
	string outfile = filename + "_nninput";
	ofstream fout(outfile.c_str());
	assert(fout);
	/* Save the cout flags. */
	ios_base::fmtflags originalFormat = fout.flags();
	/* Set the flags as needed. */ 
	fout << showpoint << fixed << setprecision(30);
	
	for(int i=0; i<300; ++i)
	{
		fout<<"   "<<vect[i]<<endl;
	}
	fout.close();
 };
 
