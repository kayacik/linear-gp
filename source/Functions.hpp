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
#include <vector>
#include "Parameters.hpp"
#include "NumberGen.hpp"
// for the functions transfered from simpGE
#define DEBUG 0 

using namespace std;

string string_trim(string str);
int line_count(string filename);
void send_message(string str, int priority);
double twoboxes_fitness(string prog, NumberGen *numg, double *eachreg);
void write2file(string text, string filename);
string my_itoa(int input);
// functions transfere from simpGE
void process_strace(char *argv1, char *argv2);
double calc_fitness(string phen, int *syscall_count);
int check_valid(string phen, int *length);
void create_pHmr_schema(string out_prog, string schema_file, int run, int winsize);
void train_pHmr(string schema_file, string dump_file, string app);


