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
/***********************************************************************************
 * Void construcor, don't use it for now.
 */
Decoder::Decoder() { };

/***********************************************************************************
 * Main constructor, filename is the instruction set file. 
 */
Decoder::Decoder(string filename)
{
	lookup = build_lookup(filename);
	OPR = new Operands[lookup.size()];
	OPR_len = lookup.size();
	int tunsd = 0;
	BYTE4 tmask;
	for (int i=0;i<lookup.size();++i)
	{
		OPR[i] = Operands(PRMDIR +lookup.at(i));
		//cout<<"OPR loc assigned"<<endl;
		OPR[i].mask = get_mask4( OPR[i].size, &(OPR[i].unused) ); // Write operand Mask
		// Assumes .regs extension for register parameter file
		//         .imms extension for immediate parameter file
		if ( (lookup.at(i)).find(".regs") != string::npos) { OPR[i].type = 0; update_lookup(&lookupR, OPR[i]); }
		else 
		{ 
			if ( (lookup.at(i)).find(".imms") != string::npos) { OPR[i].type = 1; update_lookup(&lookupI, OPR[i]); }
			else cout<<DETAILS<<" Parameter type can not be determined from file extension ("<<lookup.at(i)<<")."<<endl; 
		} 
	}
	read_instructions(filename);
	INS_len = INS.size;
	// Initialize the permanent lookup tables
	IMM  = new string[lookupI.size()]; 
	REG  = new string[lookupR.size()];
	IMM_len = lookupI.size();
	REG_len = lookupR.size(); 
	finalize_lookup(lookupI, IMM);
	finalize_lookup(lookupR, REG);
	INS_mask = get_mask2(INS_len, &INS_unused); // Unneccesary for now
	REG_mask = get_mask1(REG_len, &REG_unused); //        "
	IMM_mask = get_mask1(IMM_len, &IMM_unused); //        "
	INS.mask = get_mask4( INS.size, &(INS.unused) ); // Write instruction Mask
	// Destroy the temporary lookup tables (optional)
	// lookupI.~vector<string>();
	// lookupR.~vector<string>();
};

/***********************************************************************************
 * Assign Operator
 */
Decoder& Decoder::operator=(Decoder& dec) 
{ 
	if (this != &dec)
	{
		delete[] OPR; delete[] REG; delete[] IMM;
		delete &INS;
		OPR_len = dec.OPR_len;
		OPR = new Operands[OPR_len];
		for (int i=0; i<OPR_len; ++i) OPR[i] = dec.OPR[i];
		INS_len = dec.INS_len;
		INS = dec.INS;
		IMM_len = dec.IMM_len;
		REG_len = dec.REG_len; 
		IMM  = new string[IMM_len];
		for (int i=0; i<IMM_len; ++i) IMM[i] = dec.IMM[i]; 
		REG  = new string[REG_len];
		for (int i=0; i<REG_len; ++i) REG[i] = dec.REG[i];
		INS_mask = get_mask2(INS_len, &INS_unused); // Unneccesary for now
		REG_mask = get_mask1(REG_len, &REG_unused); //        "
		IMM_mask = get_mask1(IMM_len, &IMM_unused); //        "
		INS.mask = get_mask4( INS.size, &(INS.unused) ); // Write instruction Mask
		NUMG = dec.NUMG;
	}
	return *this;
};

/***********************************************************************************
 * Destructor
 */
Decoder::~Decoder() 
{ 
	//cout<<"Decoder destructor called"<<endl;
	delete[] IMM;
	delete[] REG;
	delete[] OPR; 
};

/***********************************************************************************
 * Builds a lookup table (vector) for all the .regs and .imms filenames, that 
 * info. is then used to link instruction table to operands array
 */
vector<string> Decoder::build_lookup(string filename)
{
	ifstream fin(filename.c_str());
	assert(fin);
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
			if (count > 2) //last 2 parameters
			{
				value = string_trim(value);
				int found = 0;
				for (int i=0;i<ret.size();++i)
					{ if (ret.at(i) == value) found = 1; }
				if (found == 0 && value.find("N/A") == string::npos) ret.push_back(value);
			}
			value = "";
			if (count == 4) count = 0;
		}	
	}
	return ret;
};

/***********************************************************************************
 * Uses the Instructions(string) constructors to fill in class variable
 */
void Decoder::read_instructions(string filename)
{
	INS = Instructions(filename, lookup);
	//INS.print();
};

/***********************************************************************************
 * Assuming register and immediate temporary lookups were allocated, this appends 
 * unseen values to the end of the vector.
 */
void Decoder::update_lookup(vector<string> *lookup, Operands ops)
{
	for(int i=0;i<ops.size;++i)
	{
		int found = 0;
		for(int j=0; j<lookup->size() && !found; ++j)
		{
			if( lookup->at(j) == ops.values[i] ) found = 1;
		}
		if ( !found ) lookup->push_back(ops.values[i]); 
	}
};

/***********************************************************************************
 * Given the number of elements (instructions, registers, immediate), it returns
 * the (4 byte) suitable mask and updates unused to show unused combinations
 */
BYTE4 Decoder::get_mask4(int length, int *unused)
{
	if (length > (pow((double)2, (double)32) - 1) ) 
		cout<<"Warning: get_mask4 is unable to process the length of "<<length<<". Please revise the code."<<endl;
	int numbits = 0;
	BYTE4 ret = 0;
	int tmp = length;
	for(int i=0; i<32; ++i)
	{
	  tmp = tmp >> 1;
	  if (tmp == 0) { numbits = i + 1; break; } 
	}
	for (int i=0;i<numbits;++i) ret+= (BYTE4) pow((double)2, (double)i);
	*unused =  (BYTE4) pow((double)2, (double)numbits) - 1 - length;
	return ret;
};

/***********************************************************************************
 * Given the number of elements (instructions, registers, immediate), it returns
 * the (2 byte) suitable mask and updates unused to show unused combinations
 */
BYTE2 Decoder::get_mask2(int length, int *unused)
{
	if (length > (pow((double)2, (double)16) - 1) ) 
		cout<<"Warning: get_mask2 is unable to process the length of "<<length<<". Please revise the code."<<endl;
	int numbits = 0;
	BYTE2 ret = 0;
	int tmp = length;
	for(int i=0; i<32; ++i)
	{
	  tmp = tmp >> 1;
	  if (tmp == 0) { numbits = i + 1; break; } 
	}
	for (int i=0;i<numbits;++i) ret+= (BYTE2) pow((double)2, (double)i);
	*unused =  (BYTE2) pow((double) 2, (double)numbits) - 1 - length;
	return ret;
};

/***********************************************************************************
 * Given the number of elements (instructions, registers, immediate), it returns
 * the (1 byte) suitable mask and updates unused to show unused combinations
 */
BYTE1 Decoder::get_mask1(int length, int *unused)
{
	if (length > (pow((double) 2, (double) 8) - 1) ) 
		cout<<"Warning: get_mask1 is unable to process the length of "<<length<<". Please revise the code."<<endl;
	int numbits = 0;
	BYTE1 ret = 0;
	int tmp = length;
	for(int i=0; i<32; ++i)
	{
	  tmp = tmp >> 1;
	  if (tmp == 0) { numbits = i + 1; break; } 
	}
	for (int i=0;i<numbits;++i) ret+= (BYTE1) pow((double)2, (double)i);
	*unused =  (BYTE1) pow((double)2, (double)numbits) - 1 - length;
	return ret;
};

/***********************************************************************************
 * Assumes the arrays passed as the 2nd and 3rd parameters are allocated
 */
void Decoder::finalize_lookup(vector<string> lookup, string *sarr)
{
	vector<string> sorted = lookup;
	sort(sorted.begin(), sorted.end());
	//send_message("Sorted lookup as follows:", 3);
	for (int i=0;i<sorted.size();++i)
	{
		sarr[i] = string(sorted.at(i));
		//send_message(i+" : "+sarr[i], 3); 
	}
};

/***********************************************************************************
 *
 */
BYTE4 Decoder::random_instruction()
{
	BYTE4 ret = 0x00000000;
	long seed = time(NULL);
	BYTE4 inst_no = (BYTE4) NUMG->uniform_intgen(0, INS_len - 1);
	int p1_op = INS.p1link[inst_no];
	int p2_op = INS.p2link[inst_no];
	int p1_size = -1, p2_size = -1;
	BYTE4 p1 = UNDEF_BYTE4, p2 = UNDEF_BYTE4; // undefined condiditon
	if (p1_op != -1) p1 = random_parameter(p1_op, &p1_size);
	if (p2_op != -1) p2 = random_parameter(p2_op, &p1_size);
	ret = ret | ((inst_no + 1)<<16);
	// if p1 and p2 is undefined, append o to the instruction 
	if (p1 != UNDEF_BYTE4) ret = ret | ((p1+  1)<<8); else ret = ret | (0x00000000)<<8; 
	if (p2 != UNDEF_BYTE4) ret = ret | (p2 + 1); else ret = ret | (0x00000000);
	//printf("Random --> instruction= Ox%.8x   ",inst_no); printf(" param1= Ox%.8x",p1); printf(" param2= Ox%.8x    ",p2);printf("Return Value= Ox%.8x\n",ret);
	return ret;
};

/***********************************************************************************
 * Remember, the values in the PDF should match with the ordering in the
 * Instructions class (i.e. open -> 6 at PDF means open-> 6 in Instructions)
 */
BYTE4 Decoder::random_instruction(ProbabilityDistro *pd)
{
	BYTE4 ret = 0x00000000;
	long seed = time(NULL);
	double vl = pd->getValue(NUMG->ran3());
	//cout<<vl<<endl;
	BYTE4 inst_no = (BYTE4) vl;
	//cout<<inst_no<<endl;
	int p1_op = INS.p1link[inst_no];
	int p2_op = INS.p2link[inst_no];
	int p1_size = -1, p2_size = -1;
	BYTE4 p1 = UNDEF_BYTE4, p2 = UNDEF_BYTE4; // undefined condiditon
	if (p1_op != -1) p1 = random_parameter(p1_op, &p1_size);
	if (p2_op != -1) p2 = random_parameter(p2_op, &p1_size);
	ret = ret | ((inst_no + 1)<<16);
	// if p1 and p2 is undefined, append o to the instruction 
	if (p1 != UNDEF_BYTE4) ret = ret | ((p1+  1)<<8); else ret = ret | (0x00000000)<<8; 
	if (p2 != UNDEF_BYTE4) ret = ret | (p2 + 1); else ret = ret | (0x00000000);
	//printf("Random --> instruction= Ox%.8x   ",inst_no); printf(" param1= Ox%.8x",p1); printf(" param2= Ox%.8x    ",p2);printf("Return Value= Ox%.8x\n",ret);
	return ret;
};

/***********************************************************************************
 * create an instruction with random parameters
 * the ordering is from the instruction list (i.e. params/*.inst)
 * 1st line is the 0 element
 */
BYTE4 Decoder::specific_instruction(int number)
{
	BYTE4 ret = 0x00000000;
	long seed = time(NULL);
	BYTE4 inst_no = 0x00000000; 
	if ((number >= 0) && (number <= INS_len - 1) )
	{ 
		inst_no = (BYTE4) number;//NUMG->uniform_intgen(0, INS_len - 1);
	}
	else
	{
		cout<<"specific_instruction: "<<number<<" is not valid."<<endl;
		inst_no = NUMG->uniform_intgen(0, INS_len - 1);
	}
	int p1_op = INS.p1link[inst_no];
	int p2_op = INS.p2link[inst_no];
	int p1_size = -1, p2_size = -1;
	BYTE4 p1 = UNDEF_BYTE4, p2 = UNDEF_BYTE4; // undefined condiditon
	if (p1_op != -1) p1 = random_parameter(p1_op, &p1_size);
	if (p2_op != -1) p2 = random_parameter(p2_op, &p1_size);
	ret = ret | ((inst_no + 1)<<16);
	// if p1 and p2 is undefined, append o to the instruction 
	if (p1 != UNDEF_BYTE4) ret = ret | ((p1+  1)<<8); else ret = ret | (0x00000000)<<8; 
	if (p2 != UNDEF_BYTE4) ret = ret | (p2 + 1); else ret = ret | (0x00000000);
	//printf("Random --> instruction= Ox%.8x   ",inst_no); printf(" param1= Ox%.8x",p1); printf(" param2= Ox%.8x    ",p2);printf("Return Value= Ox%.8x\n",ret);
	return ret;
};


/***********************************************************************************
 * create an instruction with random parameters
 * the ordering is from the instruction list (i.e. params/*.inst)
 * 1st line is the 0 element
 */
BYTE4 Decoder::specific_instruction(int number, int param1, int param2)
{
	BYTE4 ret = 0x00000000;
	long seed = time(NULL);
	BYTE4 inst_no = 0x00000000; 
	if ((number >= 0) && (number <= INS_len - 1) )
	{ 
		inst_no = (BYTE4) number;//NUMG->uniform_intgen(0, INS_len - 1);
	}
	else
	{
		cout<<"specific_instruction: "<<number<<" is not valid."<<endl;
		inst_no = NUMG->uniform_intgen(0, INS_len - 1);
	}
	int p1_op = INS.p1link[inst_no];
	int p2_op = INS.p2link[inst_no];
	int p1_size = -1, p2_size = -1;
	BYTE4 p1 = UNDEF_BYTE4, p2 = UNDEF_BYTE4; // undefined condiditon
	if ((p1_op != -1) && (param1 != -1)) p1 = specific_parameter(p1_op, &p1_size, param1);
	if ((p2_op != -1) && (param2 != -1)) p2 = specific_parameter(p2_op, &p2_size, param2);
	ret = ret | ((inst_no + 1)<<16);
	// if p1 and p2 is undefined, append o to the instruction 
	if (p1 != UNDEF_BYTE4) ret = ret | ((p1+  1)<<8); else ret = ret | (0x00000000)<<8; 
	if (p2 != UNDEF_BYTE4) ret = ret | (p2 + 1); else ret = ret | (0x00000000);
	//printf("Random --> instruction= Ox%.8x   ",inst_no); printf(" param1= Ox%.8x",p1); printf(" param2= Ox%.8x    ",p2);printf("Return Value= Ox%.8x\n",ret);
	return ret;
};


/***********************************************************************************
 *
 */
BYTE4 Decoder::random_parameter(int ops_index, int *size)
{
	//cout<<"in random parameter"<<endl;
	BYTE4 ret = 0;
	int ret_tmp = -1;
	long seed = time(NULL);
	if (*size == -1) // Don't care about the operand size, anything will do
	{ 
		int p_index = NUMG->uniform_intgen(0, OPR[ops_index].size - 1);
		//cout<<"p_index="<<p_index<<" where size is "<<OPR[ops_index].size<<endl;		
		*size = OPR[ops_index].length[p_index];
		// We don't do lookups anymore, we use the index in OPR object. 
		//if (OPR[ops_index].type == 0) ret_tmp = (BYTE4) operandIdx_lookup(REG, OPR[ops_index].values[p_index], REG_len);
		//if (OPR[ops_index].type == 1) ret_tmp = (BYTE4) operandIdx_lookup(IMM, OPR[ops_index].values[p_index], IMM_len);
		ret_tmp = (BYTE4) p_index;
		if (ret_tmp >= 0) ret = ret_tmp; else send_message("Decoder::random_parameter() : Cannot determine operand "+ OPR[ops_index].values[p_index] + ".", 1);
	}
	else
	{
		int p2_index1, p2_index2;
		//cout<<"SIZE="<<*size;
		switch(*size)
		{
			case 32: 
			{	
				if(OPR[ops_index].op32_len == 0) cout<<"Warning: No 32 bit operands found in OPR["<<ops_index<<"]."<<endl;
				p2_index1 = NUMG->uniform_intgen(0, OPR[ops_index].op32_len - 1);
				p2_index2 = OPR[ops_index].op32[p2_index1];break;
			}
			case 16: 
			{	
				if(OPR[ops_index].op16_len == 0) cout<<"Warning: No 16 bit operands found in OPR["<<ops_index<<"]."<<endl;
				p2_index1 = NUMG->uniform_intgen(0, OPR[ops_index].op16_len - 1);
				p2_index2 = OPR[ops_index].op16[p2_index1];break;
			}
			case 8: 
			{	
				if(OPR[ops_index].op8_len == 0) cout<<"Warning: No 8 bit operands found in OPR["<<ops_index<<"]."<<endl;
				p2_index1 = NUMG->uniform_intgen(0, OPR[ops_index].op8_len - 1);
				p2_index2 = OPR[ops_index].op8[p2_index1];break;
			}
			default:
			{
				cout<<"Warning: Non standard operand found in Decoder::random_parameter."<<endl;break;
			}
		}
		//cout<<"(index1="<<p2_index1<<" index2="<<p2_index2<<") ";
		// We don't do lookups anymore, we use the index in OPR object.
		//if (OPR[ops_index].type == 0) ret_tmp = (BYTE4) operandIdx_lookup(REG, OPR[ops_index].values[p2_index2], REG_len);
		//if (OPR[ops_index].type == 1) ret_tmp = (BYTE4) operandIdx_lookup(IMM, OPR[ops_index].values[p2_index2], IMM_len); 
		ret_tmp = (BYTE4) p2_index2;
		if (ret_tmp >= 0) ret = ret_tmp; else cout<<"Warning (Decoder::random_parameter() : Cannot determine operand "<< OPR[ops_index].values[p2_index2]<<endl;
	}
	return ret;
};


/***********************************************************************************
 *
 */
BYTE4 Decoder::specific_parameter(int ops_index, int *size, int parameter)
{
	//cout<<"ops_index:"<<ops_index<<" size:"<<*size<<" param:"<<parameter<<endl;
	//cout<<"in random parameter"<<endl;
	BYTE4 ret = 0;
	int ret_tmp = -1;
	long seed = time(NULL);
	if ((*size == -1) && (parameter >= 0) && (parameter <= OPR[ops_index].size - 1))// Don't care about the operand size, anything will do
	{ 
		
		int p_index = parameter;
		//cout<<"p_index="<<p_index<<" where size is "<<OPR[ops_index].size<<endl;		
		*size = OPR[ops_index].length[p_index];
		// We don't do lookups anymore, we use the index in OPR object. 
		//if (OPR[ops_index].type == 0) ret_tmp = (BYTE4) operandIdx_lookup(REG, OPR[ops_index].values[p_index], REG_len);
		//if (OPR[ops_index].type == 1) ret_tmp = (BYTE4) operandIdx_lookup(IMM, OPR[ops_index].values[p_index], IMM_len);
		ret_tmp = (BYTE4) p_index;
		if (ret_tmp >= 0) ret = ret_tmp; else send_message("Decoder::random_parameter() : Cannot determine operand "+ OPR[ops_index].values[p_index] + ".", 1);
	}
	else
	{
		// check the random_paramter fn for the excluded part
		cout<<"Error: specific_parameter should not be called with a size other than -1 or the specified parameter is out of bounds."<<endl;
	}
	return ret;
};



/***********************************************************************************
 *
 */
int Decoder::operandIdx_lookup(string *array, string target, int array_len)
{
	int ret = -1;
	for(int i=0; i<array_len; ++i)
	{
		if(array[i] == target) 
		{
			ret = i;
			break;
		}
		if (array[i] > target) break; //because array is sorted
	} 
	return ret;
};

/***********************************************************************************
 *
 */
string Decoder::decode_instruction(BYTE4 inst)
{
	string ret;
	BYTE4 p2 = inst & 0x000000FF;
	BYTE4 p1 = (inst >> 8) & 0x000000FF;
	BYTE4 in = (inst >> 16) & 0x0000FFFF;
	//printf("Decoded --> instruction= Ox%.8x p1=Ox%.8x p2=Ox%.8x  \n",in, p1, p2);
	// subtract one from the values (indexes start from 0 whereas 0 BYTE4 is reserved)
	//cout<<"LEN R,M"<<REG_len<<" "<<IMM_len<<endl;
	//cout<<"INS="<<INS.values[in - 1]<<endl;
	ret += INS.values[in - 1]; ret +=" ";
	int p1link = INS.p1link[in - 1];
	int p2link = INS.p2link[in - 1];
	//cout<<"P1,P2 "<<p1link<<" "<<p2link<<"Types "<<(OPR[p1link]).type<<(OPR[p2link]).type<<endl;
	if ( p1link != -1 && p1 != 0 )
	//	{ if ( (OPR[p1link]).type == 0 ) { ret += REG[p1 - 1]; } else { ret += IMM[p1 - 1]; } };
		{ ret += OPR[p1link].values[p1 - 1]; }
	if ( p2link != -1 && p2 != 0 )
	//	{ ret+=", "; if ( (OPR[p2link]).type == 0 ) { ret += REG[p2 - 1]; } else { ret += IMM[p2 - 1]; } };
		{ ret+=", "; ret += OPR[p2link].values[p2 - 1]; }
	//ret += OPR[p1link].values[p1 - 1];
	//ret += OPR[p2link].values[p2 - 1];
	//cout<<"Hello from "<<DETAILS<<". Nice!"<<endl;
	return ret; 
};

/***********************************************************************************
 * Runs a quick (boundary) check on INS and OPR based on instruction 
 */
bool Decoder::validate_instruction(BYTE4 inst)
{
	bool outcome = true;
	BYTE4 p2 = inst & 0x000000FF;
	BYTE4 p1 = (inst >> 8) & 0x000000FF;
	BYTE4 in = (inst >> 16) & 0x0000FFFF;
	if ( !(in > 0 && in <= INS.size) ) 
	{	
		outcome = false; //cout<<"Instruction "<<in<<" is out of bounds."<<endl;
	}
	int p1link = INS.p1link[in - 1];
	int p2link = INS.p2link[in - 1];
	if ( p1link != -1 && p1 > OPR[p1link].size)
	{
		outcome = false; //cout<<"Parameter 1 "<<p1<<" is out of bounds."<<endl;
	}
	if ( p2link != -1 && p2 > OPR[p2link].size)
	{
		outcome = false; //cout<<"Parameter 2 "<<p2<<" is out of bounds."<<endl;
	}
	return outcome;
};




