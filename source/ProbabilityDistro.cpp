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

#include "ProbabilityDistro.hpp"
/***********************************************************
 *
 */
ProbabilityDistro::ProbabilityDistro()
{

};

/***********************************************************
 *
 */
void ProbabilityDistro::initialize(vector<double> vals, vector<double> cnts)
{
	// this function reads in values and counts, CDF PDF related calculations
	// should not be here
	//cout<<"Initializing..."<<endl; 
	if (vals.size() != cnts.size())
	{
		cout<<"Error at ProbailityDistro: Values and Counts has to be of same size"<<endl;
		exit(1);
	}
	//for (int i=0;i<vals.size();++i)
	//	values.push_back(vals.at(i));
	//for (int i=0;i<cnts.size();++i)
	//	counts.push_back(cnts.at(i));
	values.assign( vals.begin(), vals.end() );
	counts.assign( cnts.begin(), cnts.end() );
	PDF.resize(values.size());
	CDF.resize(values.size());
	calculate();

};

/***********************************************************
 *
 */
ProbabilityDistro::~ProbabilityDistro()
{
	//cout<<"terminated"<<endl;
	values.clear();
	counts.clear();
	PDF.clear();
	CDF.clear();
};

/***********************************************************
 *
 */
void ProbabilityDistro::calculate()
{
	// calculate total first
	total = 0;
	for (int i=0; i< counts.size(); ++i)
		total += counts.at(i);
	//CDF depends on  accurate PDF so calculate that first
	calculatePDF(); 
	calculateCDF();

};

/***********************************************************
 *
 */
void ProbabilityDistro::calculatePDF()
{
	//PDF.clear();// just in case it has been used before
	for (int i=0; i<counts.size(); ++i)
	{
		PDF.at(i) = (counts.at(i) / total);
		//PDF.push_back(counts.at(i) / total);
	}

};

/***********************************************************
 *
 */
void ProbabilityDistro::calculateCDF()
{
	//CDF.clear();// just in case it has been used before
	double current = 0;
	for (int i=0; i<PDF.size(); ++i)
	{
		current += PDF.at(i);
		CDF.at(i) = current;
		//CDF.push_back(current);
	}
};

/***********************************************************
 *
 */
void ProbabilityDistro::printPDF_CDF()
{
	cout<<"Value\t Count\t PDF\t CDF"<<endl;
	for (int i=0; i<PDF.size(); ++i)
	{
		cout<<values.at(i)<<"\t "<<counts.at(i)<<"\t "<<PDF.at(i)<<"\t "<<CDF.at(i)<<endl;
	}

};


/***********************************************************
 * sequential search
 */
double ProbabilityDistro::getValue2(double number)
{
	//cout<<"no:"<<number<<endl;
	if ((number <0) || (number > 1))
	{
		cout<<"Warning in ProbabilityDistro::getValue: Random number is not in the range [0 1]"<<endl;
	}
	double low = 0;
	double ret = -1;
	//cout<<"cdf:"<<CDF.size()<<endl;
	for (int i=0; i<CDF.size(); ++i)
	{
		if ((number >= low) && (number < CDF.at(i)) )
			return values.at(i);
		low = CDF.at(i);
	}
	return -1;// should not reach here
};

/***********************************************************
 * binary search
 */
double ProbabilityDistro::getValue(double number)
{
	//cout<<"no:"<<number<<endl;
	if ((number <0) || (number > 1))
	{
		cout<<"Warning in ProbabilityDistro::getValue: Random number is not in the range [0 1]"<<endl;
	}
	double low = 0;
	double ret = -1;
	int max = CDF.size() - 1;
	double hi = max;
	int found = 0;
	int loc;
	int move;
	while(!found)
	{
		//found++;
		move = (int)((hi - low) / 2);
		if ((move == 0 ) && (low == max -1)) ++move; // if it reaches the upper end
		loc =   move + (int) low;
		//cout<<"num:"<<number<<" CDF:"<<CDF.at(loc)<<" CDF2:"<<CDF.at(loc)<<endl;
		//cout<<"hi:"<<hi<<" lo:"<<low<<" loc:"<<loc<<" size:"<<CDF.size()<<endl;
		if ((loc == 0) && (number < CDF.at(loc))) // if it reaches the lower end
		{
			return values.at(loc);
		}
		//cout<<"here?"<<endl;
		if ((number >= CDF.at(loc - 1)) && (number < CDF.at(loc)) )
		{
			//cout<<"found!!"<<endl;
			return values.at(loc);
		}
		if (number > CDF.at(loc))
		{
			//cout<<"Moving up"<<endl;
			low = loc;
		}
		if(number < CDF.at(loc))
		{	
			//cout<<"Moving down"<<endl;
			hi = loc;
		}
	}
	return -1;// should not reach here
};

/***********************************************************************************
 * Assign Operator
 */
ProbabilityDistro& ProbabilityDistro::operator=(ProbabilityDistro& pd) 
{ 
	if (this != &pd)
	{
		
		values = pd.values;
		counts = pd.counts;
		PDF = pd.PDF;
		CDF = pd.CDF;
		total = pd.total;
	}
	return *this;
};

/***********************************************************************************
 * Equals Operator
 */
bool ProbabilityDistro::operator==(ProbabilityDistro& pd) 
{ 
	if (total == pd.total)
		if (values == pd.values)
			if(counts == pd.counts)
				return true; // you can keep going but if values and counts are the same the rest should be too
			else
				return false;
		else
			return false;
	else
		return false;
};