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

#include "Lists.hpp"
/***********************************************************
 *
 */
Instructions::Instructions()
{

};

/***********************************************************
 *
 */
Instructions::Instructions(int lsize)
{	
		
};

/***********************************************************
 *
 */
Instructions::Instructions(string filename, vector<string> lookup)
{
	int len = line_count(filename);
	size = len;
	//cout<<"There are "<<len<<" instructions."<<endl;
	vector<double> pdf_prbs; // 2nd parameter for ProbabilityDistro
	vector<double> pdf_vals; // 1st parameter for ProbabilityDistro
	probs = new double[len];
	values = new string[len];
	p1link = new int[len];
	p2link = new int[len];
	ifstream fin(filename.c_str());
	assert(fin);
	int available = 0;
	char ch;
	string value;
	int count = 0;
	vector<string> ret;
	while (fin.get(ch))
	{
		if (ch != ';')
		{	if( ch !='\n') value+= ch; }
		else
		{
			++count;
			value = string_trim(value);
			switch(count)
			{
				case 1: 
				{
					probs[available] = atof(value.c_str());
					break;
				}
				case 2: 
				{
					values[available] = value;
					break;
				}
				case 3:
				{
					if (value == "N/A") p1link[available] = -1;
					else
					{ for (int i=0;i<lookup.size(); ++i) if (lookup.at(i) == value ) p1link[available] = i; }
					break;
				}
				case 4:
				{
					if (value == "N/A") p2link[available] = -1;
					else
					{ for (int i=0;i<lookup.size(); ++i) if (lookup.at(i) == value ) p2link[available] = i; }
					break;
				}
				default: cout<<"Default reached in Instructions(string filename)"<<endl;break;				
			}	
			value = "";
			if (count == 4) { count = 0; ++ available; }
		}	
	}
	fin.close();	
};

/***********************************************************
 * Assignment operator
 */
Instructions& Instructions::operator=(const Instructions& in)
{
	if (this != &in)
	{
		size = in.size;
		//delete[] probs; delete[] values; delete[] p1link; delete[] p2link;
		//cout<<"COPY: There are "<<size<<" instructions."<<endl;
		probs = new double[size];
		values = new string[size];
		p1link = new int[size];
		p2link = new int[size];
		for (int i=0; i<size; ++i)
		{
			probs[i] = in.probs[i];
			values[i] = in.values[i];
			p1link[i] = in.p1link[i];
			p2link[i] = in.p2link[i];
		}
	}
	return *this;
};

/***********************************************************
 *
 */
Instructions::~Instructions( )
{	
	//cout<<"Instructions destructor called"<<endl;
	delete[] probs;
	delete[] values;
	delete[] p1link;
	delete[] p2link;	
};

/***********************************************************
 *
 */
void Instructions::print( )
{
	for (int i=0;i<size;++i) cout<<"Element "<<i<<" : "<<probs[i]<<" "<<values[i]<<" "<<p1link[i]<<" "<<p2link[i]<<endl;
};

/***********************************************************
 *
 */
Operands::Operands()
{

};

/***********************************************************
 *
 */
Operands::Operands(int lsize)
{	
	
};

/***********************************************************
 *
 */
Operands::Operands(string filename)
{
	int len = line_count(filename);
	size = len;
	//cout<<"There are "<<len<<" instructions."<<endl;
	probs = new double[len];
	values = new string[len];
	length = new int[len];
	ifstream fin(filename.c_str());
	assert(fin);
	int available = 0;
	char ch;
	string value;
	int count = 0;
	vector<string> ret;
	while (fin.get(ch))
	{
		if (ch != ';')
		{	if( ch !='\n') value+= ch; }
		else
		{
			++count;
			value = string_trim(value);
			switch(count)
			{
				case 1: probs[available] = atof(value.c_str()); break;
				case 2: values[available] = value; break;
				case 3: length[available] = atoi(value.c_str()); break;
				default: cout<<"Default reached in Operands(string filename)"<<endl;break;				
			}	
			value = "";
			if (count == 3) { count = 0; ++ available; }
		}	
	}
	fin.close();
	// build operand size lookups
	op32_len = 0; op16_len = 0; op8_len = 0;
	for (int i=0; i<size; ++i)
	{
		switch (length[i])
		{
		case 32: op32_len++; break;
		case 16: op16_len++; break;
		case 8 : op8_len++;  break;
		default: cout<<"Warning: Non-standard operand length "<<length[i]<<"."<<endl;
		}
	}
	op32 = new int[op32_len];
	op16 = new int[op16_len];
	op8 = new int[op8_len];
	int av32 = 0, av16 = 0, av8 = 0; //next available
	for (int i=0; i<size; ++i)
	{
		switch (length[i])
		{
			case 32: op32[av32] = i; av32++; break;
			case 16: op16[av16] = i; av16++; break;
			case 8 : op8[av8]   = i; av8++ ; break;
		}
	}
	//for(int i=0;i<op32_len;++i) cout<<"op32["<<i<<"]="<<op32[i]<<endl;
	//for(int i=0;i<op16_len;++i) cout<<"op16["<<i<<"]="<<op16[i]<<endl;
	//for(int i=0;i<op8_len;++i) cout<<"op8["<<i<<"]="<<op8[i]<<endl;
	
};

/***********************************************************
 * copy constructor
 */
Operands::Operands(const Operands& op)
{
	size = op.size;
	//cout<<"COPY: There are "<<size<<" instructions."<<endl;
	probs = new double[size];
	values = new string[size];
	length = new int[size];
	for(int i=0; i<size; ++i)
	{
		probs[i] = op.probs[i];
		values[i] = op.values[i];
		length[i] = op.length[i];
	}
	op32_len = op.op32_len;
	op16_len = op.op16_len;
	op8_len = op.op8_len;
	op32 = new int[op32_len];
	op16 = new int[op16_len];
	op8 = new int[op8_len];
	for (int i=0; i<op32_len; ++i) op32[i] = op.op32[i]; 
	for (int i=0; i<op16_len; ++i) op16[i] = op.op16[i];
	for (int i=0; i<op8_len; ++i) op8[i] = op.op8[i];
};

/***********************************************************
 * Assignment operator
 */
Operands& Operands::operator=(const Operands& op)
{
	if ( this != &op)
	{
		//delete[] probs; 
		//delete[] values;
		// delete[] length;
		//  delete[] op32;
		//   delete[] op16;
		//    delete[] op8;
		size = op.size;
		//cout<<"COPY: There are "<<size<<" instructions."<<endl;
		probs = new double[size];
		values = new string[size];
		length = new int[size];
		for(int i=0; i<size; ++i)
		{
			probs[i] = op.probs[i];
			values[i] = op.values[i];
			length[i] = op.length[i];
		}
		op32_len = op.op32_len;
		op16_len = op.op16_len;
		op8_len = op.op8_len;
		op32 = new int[op32_len];
		op16 = new int[op16_len];
		op8 = new int[op8_len];
		for (int i=0; i<op32_len; ++i) op32[i] = op.op32[i]; 
		for (int i=0; i<op16_len; ++i) op16[i] = op.op16[i];
		for (int i=0; i<op8_len; ++i) op8[i] = op.op8[i];
	}
	return *this;
};

/***********************************************************
 *
 */
Operands::~Operands()
{
	//cout<<"Operands destructor called"<<endl;
	delete[] probs;
	delete[] values;
	delete[] length;
	delete[] op32;
	delete[] op16;
	delete[] op8;

};

/***********************************************************
 *
 */
void Operands::print()
{
	for (int i=0;i<size;++i) cout<<"Element "<<i<<" : "<<probs[i]<<" "<<values[i]<<" "<<length[i]<<endl;
};




