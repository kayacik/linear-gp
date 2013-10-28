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

/********************************************************
 *                  Assembly Runtime Class
 *		         Hilmi Gunes Kayacik, Jun 2005
 *				  (Function Implementations)
 * 
 * For fitness calculation, I need to evaulate wheter the
 * given set of assembly instructions (i.e. the shellcode)
 * would execute succesfully. Given the definition of 
 * instructions (in the form of class functions) and the 
 * set of registers that the runtime environment has, this
 * program determines the values in registers after the 
 * execution.
 ********************************************************/
#include "asmRuntime.hpp"

/********************************************************
 * Dummy constructor, not meant to be functional
 */
asmRuntime::asmRuntime()
{
	program = "";
	doCheck();
	initializeREGS();
	initializeStack();
};

/********************************************************
 * Destructor, free any pointers here
 */
asmRuntime::~asmRuntime()
{
	delete[] stack;
	//print_info("asmRuntime destructor called.");
};

/********************************************************
 * In case the assembly program is already stored in
 * a C string 
 */
asmRuntime::asmRuntime(string prog)
{
	program = prog;
	doCheck();
	initializeREGS();
	initializeStack();
};

/********************************************************
 * If the assembly program is strored in a file, this
 * constructor will load it to program variable
 */
asmRuntime::asmRuntime(char *file)
{
	program = "";
	ifstream fin(file);
	assert(fin);
	char ch;
	while (fin.get(ch))
		 program += ch;
	doCheck();
	initializeREGS();
	initializeStack();
};

/********************************************************
 * After the program is loaded, this function will fetch
 * each instruction and call the decode function.
 */
int asmRuntime::run()
{
	string inst;
	assert(doCheck() == 0);
	int decodeID = 0;
	int l1 = 0, l2 = 0;
	l2 = program.find("\n", l1);
	while (l2 != string::npos)
	{
		inst = program.substr(l1, l2 - l1);
		//cout << inst <<endl;
		decodeID = decodeInstruction(inst);
		if (decodeID == 5) return decodeID;
		l1 = l2 + 1;
		l2 = program.find("\n", l1);
	}
	// last line
	inst = program.substr(l1, program.length() - l1); 
	//cout << inst <<endl;
	decodeID = decodeInstruction(inst);
	return decodeID;
};


/********************************************************
 * Stuff happens here ;)
 */
int asmRuntime::decodeInstruction(string instruction)
{
	int ret = 0;
	string name, par1, par2;
	int l1 =0, l2 = 0, l3 = 0;
	instruction = trim(instruction);
	l1 = instruction.find(" ", 0); // operator
	l2 = instruction.find(",", 0); // operand1
	l3 = instruction.size();	   // operand2
	//cout<<instruction<<"- decoding - l1="<<l1<<" l2="<<l2<<" l3="<<l3<<endl;
	if ( l1 == string::npos && l2 == string::npos && l3 > 0 )
	{// No operand instruction (e.g. cdq)
		name = trim(toUpper(instruction.substr(0, l3)));
		par1 = "NONE";
		par2 = "NONE";
	}
	else if ( l1 > 0 && l2 == string::npos && l3 > 0 )
	{// One operand instruction (e.g. int 0x80)
		name = trim(toUpper(instruction.substr(0, l1)));
		par1 = trim(toUpper(instruction.substr(l1 + 1, l3 - l1)));
		par2 = "NONE";
	} 
	else if ( l1 > 0 && l2 > 0 && l3 > 0 )
	{// Two operand instruction (e.g. mov eax,ebx)
		name = trim(toUpper(instruction.substr(0, l1)));
		par1 = trim(toUpper(instruction.substr(l1 + 1, l2 - l1 - 1)));
		par2 = trim(toUpper(instruction.substr(l2 + 1, l3 - l2)));
	}
	else
	{// None of the above (should not happen)
		string error ="Instruction \'" + instruction + "\' is not recognized.";
		if (instruction.size() > 0) print_info((char*) error.c_str()); // Error if not a empty line
		name = "NONE"; par1 = "NONE"; par2 = "NONE";
		return -1;
	}
	//cout<<"Instruction: "<<name<<" parameter1: "<<par1<<" parameter2: "<<par2<<endl;
	////////////////////////////////////////////////////
	// This is where the instructions are executed
	// It maps a class function executeXYZ to the
	// assembly instruction XYZ. It is up to the
	// programmer to write the function and add an
	// if statement here. 
	////////////////////////////////////////////////////
	if (name == "MOV"){
		executeMOV(par1, par2);
		return 1;}
	else if (name == "CDQ"){
		executeCDQ();
		return 2;}
	else if (name == "PUSH"){
		executePUSH(par1);
		return 3;}
	else if (name == "XOR"){
		executeXOR(par1, par2);
		return 4;}
	else if (name == "INT"){
		executeINT(par1);
		return 5;}
	else if (name == "BITS"){
		executeBITS(par1);
		return 6;}
	else if (name == "ADD"){
		executeADD(par1, par2);
		return 7;}
	else if (name == "SUB"){
		executeSUB(par1, par2);
		return 8;}
	else if (name == "INC"){
		executeINC(par1);
		return 9;}
	else if (name == "DEC"){
		executeINC(par1);
		return 10;}
	else if (name == "MUL"){
		executeMUL(par1);
		return 11;}
	else if (name == "DIV"){
		executeDIV(par1);
		return 12;}
	else if (name == "AND"){
		executeAND(par1, par2);
		return 13;}
	else if (name == "OR"){
		executeOR(par1, par2);
		return 14;}
	else if (name == "NOT"){
		executeNOT(par1);
		return 15;}
	else {
		print_err("Unknown instruction found.");
		cout<<name<<endl;
	}
};

/********************************************************
 * Make sure that the REG32 type is 32 bit and the program
 * is loaded in program (min prog. size is assumed 3)
 * Returns 0 for no-error, a number otherwise.
 */
int asmRuntime::doCheck()
{
	int ret = 0;
	if (sizeof(REG32) != 4) 
	{
		print_err("REG32 is not 32 bits, check the typedef.");
		ret = 1; 
	}
	if (program.size() < 3)
	{
		print_err("No program present.");
		ret = 2;
	}
	if (sizeof(uint64_t) != 8) { print_err("8 byte uint64_t is needed for division."); ret = 3;}
	return ret;
} ;

/********************************************************
 * A print function for error messages (without exiting)
 */
void asmRuntime::print_err(char *str)
{
	if (DUMP_ERR) cout<<"[x] "<< str <<endl;
};

/********************************************************
 * A print function for info. messages (without exiting)
 */
void asmRuntime::print_info(char *str)
{
	if (DUMP_INFO) cout<<"[i] "<< str <<endl;
};

/********************************************************
 * 
 */
string asmRuntime::toUpper(string str)
{
	string ret;
	for (int i=0;i<str.size();++i)
	{
		ret += toupper(str[i]);
	}
	return ret;
};

/********************************************************
 * 
 */
string asmRuntime::toLower(string str)
{
	string ret;
	for (int i=0;i<str.size();++i)
	{
		ret += tolower(str[i]);
	}
	return ret;
};

/********************************************************
 * 
 */
string asmRuntime::trim(string str)
{
	string ret;	
	int lead = -1, trail = -1, start = 0, end = 0;
	// determine if there is a leading whitespace
	if (str[0] == ' ')
	{
		for (int i=0;i<str.size();++i)
		{
			if (str[i] == ' ') lead = i; else break;
		}
	}
	// determine if there is a trailing whitespace
	if (str[str.size()-1] == ' ')
	{
		for (int i=str.size()-1;i>=0;--i)
		{
			if (str[i] == ' ') trail = i; else break;
		}
	}
	if (lead == -1) start = 0; else start = lead + 1;
	if (trail == -1) end = str.size(); else end = trail;
	for (int i=start;i<end;++i)
	{
		ret += str[i];
	}
	return ret;
};


/********************************************************
 * Implementation of MOV P1,P2						   */
void asmRuntime::executeMOV(string p1, string p2)
{
	int p1type = 0, p2type = 0;
	unsigned int *p1ptr = NULL, *p2ptr = NULL;
	unsigned int part1 = 0, part2 = 0, result = 0;
	
	/*
	if (p1.find("0X") != string::npos) //Hex value
		part1 = str2hex(p1);
	else
		{
		p1ptr = findRegister(p1, &p1type);
		assert (p1ptr);
		part1 = getValue(p1ptr, p1type);
		}
	//printf("Part1 is 0x%.8x\n",part1);	
	if (p2.find("0X") != string::npos) //Hex value
		part2 = str2hex(p2);
	else
		{
		p2ptr = findRegister(p2, &p2type);
		assert (p2ptr);
		part2 = getValue(p2ptr, p2type);
		}
	*/
	//printf("Part2 is 0x%.8x\n",part2);	
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	if (p1type < 0) print_info("MOV: First parameter is an immediate, instruction invalid and has no effect.");
	else
		{ // Carry out the MOV op
			if ( typeCheck(p1type, p2type) == -1 ) 
				print_err("Type error in MOV operation.");
			else{
				result = part2;
				setValue(p1ptr, p1type, result);
				}
		}
};

/********************************************************
 * Implementation of CDQ							   */
void asmRuntime::executeCDQ()
{
	int type1 = 0, type2 = 0;
	unsigned int* edx = findRegister("EDX", &type1);
	unsigned int* eax = findRegister("EAX", &type2);
	if (! type1 == type2 ) 
	{
		print_err("In CDQ, types don't match");
		return;
	}
	*edx = *eax;
};

/********************************************************
 * Implementation of PUSH P1						   */
void asmRuntime::executePUSH(string p1)
{
	int p1type = 0, dummy = 0;
	unsigned int *regloc = NULL, *esp = NULL;
	unsigned int dat = 0; 
	if (p1.find("0X") != string::npos) //Hex value
		dat = str2hex(p1);
	else
		{
		regloc = findRegister(p1, &p1type);
		assert (regloc);
		dat = *regloc;
		if (p1type != 32) print_info("Attempting to push a non 32 bit data to stack");
		}
	if (stackAvailable())
		{
		stack[stack_last + 1] = dat;
		stack_last++;
		}
	else{
		print_err("executePUSH: Stack size exceeded.");
		}
	
	esp = findRegister("ESP", &dummy);
	assert (esp);
	*esp = (REG32) &stack[stack_last];
	
};

/********************************************************
 * Implementation of XOR P1,P2						   */
void asmRuntime::executeXOR(string p1, string p2)
{
	int p1type = 0, p2type = 0;
	unsigned int *p1ptr = NULL, *p2ptr = NULL;
	unsigned int dat1 = 0, dat2 = 0, part1 = 0, part2 = 0, result = 0;
	
	/*
	if (p1.find("0X") != string::npos) //Hex value
		dat1 = str2hex(p1);
	else
		{
		p1ptr = findRegister(p1, &p1type);
		assert (p1ptr);
		part1 = getValue(p1ptr, p1type);
		//printf("Part1 is 0x%.8x\n",part1);
		}
	if (p2.find("0X") != string::npos) //Hex value
		dat2 = str2hex(p2);
	else
		{
		p2ptr = findRegister(p2, &p2type);
		assert (p2ptr);
		part2 = getValue(p2ptr, p2type);
		//printf("Part2 is 0x%.8x\n",part2);
		}
	*/	
		
	// 1. Get operand info
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	// 2. Carry out the operation
	if (p1type < 0) print_info("XOR: First parameter is an immediate, instruction has no effect.");
	else
		{ // Carry out the XOR op
			if ( typeCheck(p1type, p2type) == -1 ) 
				print_err("Type error in XOR operation.");
			else{
				result = part1 ^ part2;
				setValue(p1ptr, p1type, result);
				}
		}
	// 3. Modify flags
	flags[OF] = 0;
	flags[CF] = 0;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);	
};

/********************************************************
 * Implementation of INT P1							   */
void asmRuntime::executeINT(string p1)
{
	int dummy = 0;
	REG32 *eax = findRegister("EAX", &dummy);
	// cout<<"[i] Interrupt "<<p1<<" (System Call: ";
	// printf("0x%.8x)\n",*eax);
	// Since this is a simulation, nothing to be done here
};

/********************************************************
 * Implementation of BITS P1 (Dummy implementation)	   */
void asmRuntime::executeBITS(string p1)
{
	// Nothing to be done for this
};

/********************************************************
 * Implementation of ADD R, R/I	   */
void asmRuntime::executeADD(string p1, string p2)
{
	int p1type = 0, p2type = 0, cfl, ofl;
	unsigned int *p1ptr = NULL, *p2ptr = NULL;
	unsigned int part1 = 0, part2 = 0;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	bitset<BYTE4_SIZE> bs1, bs2, bs3;
	bs1 |= part1;
	bs2 |= part2;
	bs3 = binaryADD(bs1, bs2, &cfl, &ofl);
	unsigned int result = (unsigned int) bs3.to_ulong();
	if (p1type > 0) *p1ptr = result; // if register, assign result
	flags[CF] = cfl;
	flags[OF] = ofl;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);	 
};

/********************************************************
 * Implementation of INC R 	   */
void asmRuntime::executeINC(string p1)
{
	int p1type = 0, cfl, ofl;
	unsigned int *p1ptr = NULL;
	unsigned int part1 = 0, part2 = 0x00000001;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	bitset<BYTE4_SIZE> bs1, bs2, bs3;
	bs1 |= part1;
	bs2 |= part2;
	bs3 = binaryADD(bs1, bs2, &cfl, &ofl);
	unsigned int result = (unsigned int) bs3.to_ulong();
	if (p1type > 0) *p1ptr = result; // if not immediate, assign result
	flags[OF] = ofl;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);	 
};

/********************************************************
 * Implementation of DEC R 	   */
void asmRuntime::executeDEC(string p1)
{
	int p1type = 0, cfl, ofl;
	unsigned int *p1ptr = NULL;
	unsigned int part1 = 0, part2 = 0x00000001;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	bitset<BYTE4_SIZE> bs1, bs2, bs3;
	bs1 |= part1;
	bs2 |= part2;
	bs3 = binarySUB(bs1, bs2, &cfl, &ofl);
	unsigned int result = (unsigned int) bs3.to_ulong();
	if (p1type > 0) *p1ptr = result; // if not immediate, assign result
	flags[OF] = ofl;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);	 
};


/********************************************************
 * Implementation of SUB R, R/I	   */
void asmRuntime::executeSUB(string p1, string p2)
{
	int p1type = 0, p2type = 0, cfl, ofl;
	unsigned int *p1ptr = NULL, *p2ptr = NULL;
	unsigned int part1 = 0, part2 = 0;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	bitset<BYTE4_SIZE> bs1, bs2, bs3;
	bs1 |= part1;
	bs2 |= part2;
	bs3 = binarySUB(bs1, bs2, &cfl, &ofl);
	unsigned int result = (unsigned int) bs3.to_ulong();
	if (p1type > 0) *p1ptr = result; // if not immediate, assign result
	flags[CF] = cfl;
	flags[OF] = ofl;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);			 	 
};

/********************************************************
 * Implementation of MUL R/I	   */
void asmRuntime::executeMUL(string p1)
{
	int p1type = 0, p2type = 0, p3type = 0, pxtype = 0, cfl, ofl;
	unsigned int *p1ptr = NULL, *p2ptr = NULL, *p3ptr = NULL, *pxptr = NULL;
	unsigned int part1 = 0, part2 = 0, part3 = 0, partx = 0;
	string p2 = "EAX";
	string p3 = "EDX";
	string ahstr = "AH";
	string dxstr = "DX";  
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	bitset<BYTE4_SIZE> P2; P2|= part2; // content of eax
	bitset<BYTE4_SIZE> P1; P1|= part1;
	bitset<2*BYTE4_SIZE> RESULT;
	switch ( abs(p1type) )
	{
		case 4:
			P2 &= 0x000000FF; // mask everything except al
			break;
		case 8:
			P2 >>=8; P2 &= 0x000000FF; // mask everything except ah
			break;
		case 16:
			P2 &= 0x0000FFFF; // mask everything except eax
			break;
		case 32:
			// do nothing
			break;
		default:
			cout<<"executeMUL: Multiply only works on 8, 16 , 32 bits, found "<<abs(p1type)<<endl;
			break;
	}
	RESULT = binaryMUL(P1, P2, &cfl, &ofl);	
	
	bitset<BYTE4_SIZE> lo;
	bitset<BYTE4_SIZE> hi;
	for (int i=0;i< BYTE4_SIZE; ++i)
	{
		if (RESULT.test(i) == 1) lo.set(i);
	}
	unsigned int hi_ul = 0;
	unsigned int lo_ul = lo.to_ulong(); 
	flags[CF] = 0; flags[OF] = 0; 
	switch ( abs(p1type) )
	{
		case 8: //AX <-AL *r/m8
			setValue( p1ptr, 16,  (lo_ul & 0x0000FFFF) );
			getCoordinates(ahstr, &pxtype, &pxptr, &partx);
			if (partx != 0) { flags[CF] = 1; flags[OF] = 1; }
			break;
		case 16: //DX:AX <-AX *r/m16
			setValue( p1ptr, 16,  (lo_ul & 0x0000FFFF) );
			getCoordinates(p3, &p3type, &p3ptr, &part3);
			setValue( p3ptr, 16,  ( (lo_ul >> 16) & 0x0000FFFF ) ); 
			getCoordinates(dxstr, &pxtype, &pxptr, &partx);
			if (partx != 0) { flags[CF] = 1; flags[OF] = 1; }
			break;
		case 32: //EDX:EAX <-EAX *r/m32
			for (int i=0;i< BYTE4_SIZE; ++i)
			{
				if (RESULT.test(i + BYTE4_SIZE) == 1) hi.set(i);
			}
			hi_ul = (unsigned int) hi.to_ulong();
			getCoordinates(p3, &p3type, &p3ptr, &part3);
			setValue( p1ptr, 32, lo_ul );
			setValue( p3ptr, 32, hi_ul );
			getCoordinates(p3, &pxtype, &pxptr, &partx);
			if (partx != 0) { flags[CF] = 1; flags[OF] = 1; }
			break;
		default:
			// The error message above need not be repeated
			break;
	}
};

/********************************************************
 * Implementation of DIV R/I	   */
void asmRuntime::executeDIV(string p1)
{
	int p1type = 0, p2type = 0, p3type = 0, cfl, ofl;
	unsigned int *p1ptr = NULL, *p2ptr = NULL, *p3ptr = NULL;
	unsigned int part1 = 0, part2 = 0, part3 = 0;
	string p2 = "EAX";
	string p3 = "EDX";
	uint64_t DS = 0;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	if (part1 == 0) return; // In case of divide by 0 do nothing
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	uint64_t tmp = 0;
	switch ( abs(p1type) )
	{
		case 4: // AX / rm8
			DS = DS | (part2 & 0x0000FFFF);
			break;
		case 8: // AX / rm8
			DS = DS | (part2 & 0x0000FFFF);
			break;
		case 16: // DX:AX / rm16
			DS = DS | (part2 & 0x0000FFFF);
			getCoordinates(p3, &p3type, &p3ptr, &part3);
			DS = DS | ( (part3<<16) & 0xFFFF0000 );
			break;
		case 32:
			DS = DS | (part2);
			getCoordinates(p3, &p3type, &p3ptr, &part3);
			tmp = (uint64_t) part3;
			tmp = tmp << 32;
			DS = tmp;
			break;
		default:
			cout<<"Divide: Multiply only works on 8, 16 , 32 bits, found "<<abs(p1type)<<endl;
			break;
	}
	REG32 remainder = DS % part1;
	REG32 quotient  = (REG32) (DS / part1);

	switch ( abs(p1type) )
	{
		case 4: //AL<-quotient AH<-Remainder
			setValue(p2ptr, 4, quotient);
			setValue(p2ptr, 8, remainder);
			break;
		case 8: //AL<-quotient AH<-Remainder
			setValue(p2ptr, 4, quotient);
			setValue(p2ptr, 8, remainder);
			break;
		case 16: //AX<-quotient DX<-Remainder
			setValue(p2ptr, 16, quotient);
			setValue(p3ptr, 16, remainder);
			break;
		case 32: //EAX<-quotient EDX<-Remainder
			setValue(p2ptr, 32, quotient);
			setValue(p3ptr, 32, remainder);
			break;
		default:
			// Error above applies
			break;
	}
	// Behavior of DIV is unspecified for all flags, hence nothing to be done 
};

/********************************************************
 * Implementation of AND R, R/I	   */
void asmRuntime::executeAND(string p1, string p2)
{
	int p1type = 0, p2type = 0;
	unsigned int *p1ptr = NULL, *p2ptr = NULL;
	unsigned int part1 = 0, part2 = 0, result = 0;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	if (p1type < 0) return; // In case of immediate dest, do nothing
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	result = part1 & part2;
	*p1ptr =  result;
	flags[OF] = 0; flags[CF] = 0;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);	 
};

/********************************************************
 * Implementation of OR R, R/I	   */
void asmRuntime::executeOR (string p1, string p2)
{
	int p1type = 0, p2type = 0;
	unsigned int *p1ptr = NULL, *p2ptr = NULL;
	unsigned int part1 = 0, part2 = 0, result = 0;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	if (p1type < 0) return; // In case of immediate dest, do nothing
	getCoordinates(p2, &p2type, &p2ptr, &part2);
	result = part1 | part2;
	*p1ptr =  result;
	flags[OF] = 0; flags[CF] = 0;
	flags[ZF] = checkZero(result);
	flags[SF] = checkSign(result);
	flags[PF] = checkParity(result);	
};

/********************************************************
 * Implementation of NOT R (1's complement)	   */
void asmRuntime::executeNOT(string p1)
{
	int p1type = 0;
	unsigned int *p1ptr = NULL;
	unsigned int part1 = 0, result = 0;
	getCoordinates(p1, &p1type, &p1ptr, &part1);
	if (p1type < 0) return; // In case of immediate dest, do nothing
	result = ~part1;
	*p1ptr = result;
	// No flags affected
};

/********************************************************
 * Initialize registers (as unknown)				   */
void asmRuntime::initializeREGS()
{
	srand( 25 ); //Seed the number generator once
	registers[0] = randomRegVal(); names[0] = string("EAX,AX,AL,AH");
	registers[1] = randomRegVal(); names[1] = string("EBX,BX,BL,BH");
	registers[2] = randomRegVal(); names[2] = string("ECX,CX,CL,CH");
	registers[3] = randomRegVal(); names[3] = string("EDX,DX,DL,DH");
	registers[4] = randomRegVal(); names[4] = string("ESI,SI");
	registers[5] = randomRegVal(); names[5] = string("EDI,DI");
	registers[6] = randomRegVal(); names[6] = string("EBP,BP");
	registers[7] = randomRegVal(); names[7] = string("ESP,SP");
	registers[8] = randomRegVal(); names[8] = string("EIP,IP");
	registers[9] = randomRegVal(); names[9] = string("EFLAGS,FLAGS");
};

/********************************************************
 * Initialize Stack										*/
void asmRuntime::initializeStack()
{
	stack = new REG32 [STACK_SIZE];
	stack_last = -1;
};
/********************************************************
 * Get the triplet info for each operand				*/
void asmRuntime::getCoordinates(string p, int *type, unsigned int **ptr, unsigned int *value)
{
	if (p.find("0X") != string::npos) //Immediate		
	{	
		*value = str2hex(p);
		*type = (p.size() - 2) * -4; // type is negative
		*ptr = NULL;
	}
	else if (p.find("[") != string::npos) //Memory Loc (for future use)
	{ }
	else // Register  
	{
		*ptr = findRegister(p, type);
		assert (*ptr);
		*value = getValue(*ptr, *type);
	}
};

/********************************************************
 * Prints the contents of the registers and stack	   */
void asmRuntime::report()
{
	cout<<"============= R E P O R T ============="<<endl;  
	cout<<"----------- Register Values -----------"<<endl;
	cout<<" "<< names[0]<<" : "; printf("0x%.8x\t",registers[0]); cout<< registers[0]<<endl; 
	cout<<" "<< names[1]<<" : "; printf("0x%.8x\t",registers[1]); cout<< registers[1]<<endl; 
	cout<<" "<< names[2]<<" : "; printf("0x%.8x\t",registers[2]); cout<< registers[2]<<endl; 
	cout<<" "<< names[3]<<" : "; printf("0x%.8x\t",registers[3]); cout<< registers[3]<<endl; 
	cout<<" "<< names[4]<<" : "; printf("0x%.8x\t",registers[4]); cout<< registers[4]<<endl;
	cout<<" "<< names[5]<<" : "; printf("0x%.8x\t",registers[5]); cout<< registers[5]<<endl; 
	cout<<" "<< names[6]<<" : "; printf("0x%.8x\t",registers[6]); cout<< registers[6]<<endl; 
	cout<<" "<< names[7]<<" : "; printf("0x%.8x\t",registers[7]); cout<< registers[7]<<endl; 
	cout<<" "<< names[8]<<" : "; printf("0x%.8x\t",registers[8]); cout<< registers[8]<<endl; 
	cout<<" "<< names[9]<<" : "; printf("0x%.8x\t",registers[9]); cout<< registers[9]<<endl; 
	cout<<"----------- Stack Contents ------------"<<endl;
	for(int i=0;i<=stack_last;++i)
		printf(" [%.8x] Byte %d:  0x%.8x\n",&stack[i],i,stack[i]);
	cout<<"----------- Flag  Contents ------------"<<endl;
	cout<<"CPAZSTIDONR"<<endl;
	cout<<flags[CF]<<flags[PF]<<flags[AF]<<flags[ZF]<<flags[SF]<<flags[TF]<<flags[IF]<<flags[DF]<<flags[OF]<<flags[NT]<<flags[RF]<<endl;
	cout<<"========= E N D  R E P O R T =========="<<endl; 

};


/********************************************************
 * Prints the contents of the registers and stack	   */
unsigned int asmRuntime::str2hex(string str)
{
	str = toUpper(str);
	int alldone = 0;
	int n = 0;
	int digit = 0;
	unsigned int ret = 0;
	//cout<<str<<endl;
	for (int i=str.size()-1;i>=0;--i)
	{
		if (str.at(i) == 'X') alldone = 1;
		if (!alldone)
		{
			switch (str.at(i))
			{
				case 'F': digit = 15; break;
				case 'E': digit = 14; break; 
				case 'D': digit = 13; break;
				case 'C': digit = 12; break;
				case 'B': digit = 11; break;
				case 'A': digit = 10; break;
				default : digit = atoi(str.substr(i,1).c_str()); break;
			}
			
			ret += (unsigned int) (pow((double)16, (double)n) * digit);
			//cout<<"n="<<n<<" digit="<<digit<<" ret="<<ret<<endl;
			n++; 
		}
	}
	//printf("from str2hex 0x%x\n", ret);
	return ret;
};

unsigned int* asmRuntime::findRegister(string rname, int *type)
{
	for (int i = 0; i < REG_COUNT; ++i)
	{
		if (names[i].find(rname) != string::npos)
		{
			//cout<<"Found "<<rname<<" at "<<i<<endl;
			// Determine operation type (32/16/8bit)
			if( rname.find("AL") != string::npos || \
				rname.find("BL") != string::npos || \
				rname.find("CL") != string::npos || \
				rname.find("DL") != string::npos  )         *type = 4;
			else if( rname.find("AH") != string::npos || \
					 rname.find("BH") != string::npos || \
					 rname.find("CH") != string::npos || \
					 rname.find("DH") != string::npos )     *type = 8;
			else if (rname.at(0) != 'E')					*type = 16;
			else if (rname.at(0) == 'E' && rname != "ES")	*type = 32; 
			return &registers[i];
		}
	}
	
	return NULL;
};


/********************************************************
 * An unused function									*/
void asmRuntime::applyMask(unsigned int *reg, int type, int where)
{
	if(type == 8)
	{
		if (where == SRCREG)
		{// reset the higher n bits 
			*reg = *reg & RESET_HI8;
		}
		else if (where == DSTREG)
		{// reset the lower n bits
			*reg = *reg & RESET_LO8;
		}
		else
		{
			print_err("applyMask: third argument is not SRCREG or DSTREG");
		}
	}
	else if (type == 16)
	{
		if (where == SRCREG)
		{// reset the higher n bits 
			*reg = *reg & RESET_HI16;
		}
		else if (where == DSTREG)
		{// reset the lower n bits
			*reg = *reg & RESET_LO16;
		}
		else
		{
			print_err("applyMask: third argument is not SRCREG or DSTREG");
		}
	}
	else
	{ // Bit 32, do nothing
	
	}
};

/********************************************************
 * An unused function									*/
unsigned int asmRuntime::getMask(unsigned int *reg, int type, int where)
{
	unsigned int ret = 0; 
	cout<<"Type is "<<type<<endl;
	if(type == 8)
	{
		if (where == SRCREG)
		{// reset the higher n bits 
			ret = RESET_HI8;
		}
		else if (where == DSTREG)
		{// reset the lower n bits
			ret = RESET_LO8;
		}
		else
		{
			print_err("applyMask: third argument is not SRCREG or DSTREG");
		}
	}
	else if (type == 16)
	{
		if (where == SRCREG)
		{// reset the higher n bits 
			ret = RESET_HI16;
		}
		else if (where == DSTREG)
		{// reset the lower n bits
			ret = RESET_LO16;
		}
		else
		{
			print_err("applyMask: third argument is not SRCREG or DSTREG");
		}
	}
	else
	{ // Bit 32, do nothing
		ret = 0xFFFFFFFF;
	}
	return ret;
};

/********************************************************
 * From 32 bit/ 16 bit / 8 bit HI / 8 bit LO register
 * read into a 32 bit register like structure, in case of 
 * 8 bit HI, align it with 8 bit LO
 */	
REG32 asmRuntime::getValue(unsigned int *reg, int type)
{
	switch (type)
	{
		case 32:
			return *reg;
			break;
		case 16:
			return ( 0x0000FFFF & *reg );
			break;
		case 8:
			return ( 0x0000FF00 & *reg ) >> 8;
			break;
		case 4:
			return ( 0x000000FF & *reg );
			break;
		default:
			print_err("getValue: Type not recognized");
			return 0x00000000;
			break;
	}
};

/********************************************************
 * From 32 bit register like structure, update the result 
 * to a 32 bit / 16 bit / 8 bit HI / 8 bit LO register
 * In case of 8 bit HI, re align it with <<8 
 */
REG32 asmRuntime::setValue(unsigned int *reg, int type, REG32 result)
{
	switch (type)
	{
		case 32:
			*reg = result;
			return *reg;
			break;
		case 16:
			*reg = ( 0xFFFF0000 & *reg );
			result = result & 0x0000FFFF;
			*reg = result | *reg;
			return *reg; 
			break;
		case 8:
			*reg = ( 0xFFFF00FF & *reg );
			result = result << 8;
			result = result & 0x0000FF00;
			*reg = ( result ) | *reg;
			return *reg;
			break;
		case 4:
			*reg =  ( 0xFFFFFF00 & *reg );
			result = result & 0x000000FF;
			*reg = result | *reg;
			return *reg; 
			break;
		default:
			print_err("setValue: Type not recognized");
			return *reg;
			break;
	}

};

/********************************************************
 *
 * 
 */	
int asmRuntime::typeCheck(int type1, int type2)
{
	type1 = abs(type1); // eliminate reg/imm diference
	type2 = abs(type2);
	if (type1 == type2)
		return 0;
	else if (type1 == 8 && type2 == 4)
		return 0;
	else if (type1 == 4 && type2 == 8)
		return 0;
	else if (type2 == 0)
		return 0;
	else
		return -1;
};

/********************************************************
 *
 * 
 */
int asmRuntime::stackAvailable()
{
	if (stack_last < STACK_SIZE - 1)
		return 1;
	else
		return 0;
}

/********************************************************
 *
 * 
 */
REG32 asmRuntime::randomRegVal()
{
	REG32 ret = 0;
	double r = 0;
	REG32 bit = 0;
	for(int i=0;i<sizeof(REG32);++i)
	{
		for(int j=0;j<8;++j)
		{
			r = (double) rand() / (double) RAND_MAX;
			if (r< 0.5) bit = 0; else bit = 1;
			ret += (REG32) pow((double) 2, (double) (i*8+j) ) * bit;
		}
	}
	return ret;
};

/********************************************************
 *
 * 
 */
int asmRuntime::stack_search(unsigned int* pattern, int plen, int from)
{
	int i = from, j = 0;
	while (i <= stack_last && j < plen)
	{// Either end of stack or pattern found
		if (stack [i] == pattern [j]) 
		{
			//printf("stack[%d] = pattern[%d] = 0x%.8x\n",i,j,stack[i]);
			++i; ++j;
		}
		else
		{
			++i; j = 0;
		}
	}
	//printf("i=%d, j=%d\n",i,j);
	if (plen == j) return i - j; // location of the pattern
	else return -1; // not found
};

/********************************************************
 *
 * 
 */
int asmRuntime::checkSign(unsigned int val)
{
	bitset<BYTE4_SIZE> bs;
	bs |= val;
	if (bs.test(BYTE4_SIZE-1)) return 1; else return 0;
};

/********************************************************
 *
 * 
 */
int asmRuntime::checkParity(unsigned int val)
{
	unsigned int temp = val & 0x000000FF;
	int setbits = 0;
	for (int i = 0; (i < 8) && (temp > 0); ++ i)
	{
		if ( (temp - pow( (double)2, (double) 8 - i - 1 )) > 0 ) 
		{
			setbits++;
			temp -= (int) pow( (double)2, (double) 8 - i - 1 );
		}
	}
	if (setbits % 2 == 0) return 1; else return 0;
};

/********************************************************
 *
 * 
 */
int asmRuntime::checkZero(unsigned int val)
{
	if (val == 0) return 1; else return 0;
};

/********************************************************
 * Low level binary addition function
 * it also gets the carry and overflow flags right
 */
bitset<BYTE4_SIZE> asmRuntime::binaryADD(bitset<BYTE4_SIZE> A, bitset<BYTE4_SIZE> B, int *cflag, int *oflag)
{
	bitset<BYTE4_SIZE> ret;
	bitset<BYTE4_SIZE + 1> carry;
	*cflag = 0;
	*oflag = 0;
	for (int i=0;i<BYTE4_SIZE;++i)
	{
		
		int a = A.test(i);
		int b = B.test(i);
		int c = carry.test(i);
		int result = a + b + c;
		switch (result)
		{
			case 0:
				ret.reset(i);
				carry.reset(i+1);
				break;
			case 1:
				ret.set(i);
				carry.reset(i+1);
				break;
			case 2:
				ret.reset(i);
				carry.set(i+1);
				break;
			case 3:
				ret.set(i);
				carry.set(i+1);
				break;
		};
	}
	if(carry.test(BYTE4_SIZE) == true) *cflag = 1;
	if(A.test(A.size()-1) == true && B.test(B.size()-1) == true && ret.test(ret.size()-1) == false) *oflag = 1; 
	if(A.test(A.size()-1) == false && B.test(B.size()-1) == false && ret.test(ret.size()-1) == true) *oflag = 1; 
	return ret;
};

/********************************************************
 * Low level function for binary subtraction
 * it mainly uses the binary addition above
 */
bitset<BYTE4_SIZE> asmRuntime::binarySUB(bitset<BYTE4_SIZE> A, bitset<BYTE4_SIZE> B, int *cflag, int *oflag)
{
	*cflag = 0;
	B.flip();
	bitset<BYTE4_SIZE> one; one.set(0);
	B = binaryADD(B, one, cflag, oflag);
	bitset<BYTE4_SIZE> ret; ret = binaryADD(A, B, cflag, oflag);
	return ret;
};
/********************************************************
 * Low level function for binary multiplication
 * 
 */
bitset<2*BYTE4_SIZE> asmRuntime::binaryMUL(bitset<BYTE4_SIZE> A, bitset<BYTE4_SIZE> B, int *cflag, int *oflag)
{
	*cflag = 0; *oflag = 0;
	vector<unsigned long> bss;
	for(int i=0;i<BYTE4_SIZE;++i)//2
	{
		int a = A.test(i);
		bitset<2*BYTE4_SIZE> bs;
		for(int j=0;j<BYTE4_SIZE;++j)//1
		{
			int b = B.test(j);
			if ((a*b) == 1) bs.set(j);
		}
		unsigned long tmp = bs.to_ulong(); 
		bs<<=i;
		bss.push_back(tmp);
	}
	bitset<2*BYTE4_SIZE> ret;
	for (int i=0;i<bss.size();++i)
	{
		bitset<2*BYTE4_SIZE> X;
		unsigned long x = bss.at(i);
		X |=x;
		ret = binaryADD( ret, X, cflag, oflag);
	}
	for(int i=ret.size()-1;i>=ret.size()/2;--i)
	{
		if (ret.test(i)) { *cflag = 1; *oflag = 1; break;}
	}
	return ret;
};

/********************************************************
 * Low level function for 64 bit binary addition, only
 * used by the multiplication for now
 */
bitset<2*BYTE4_SIZE> asmRuntime::binaryADD(bitset<2*BYTE4_SIZE> A, bitset<2*BYTE4_SIZE> B, int *cflag, int *oflag)
{
	bitset<2*BYTE4_SIZE> ret;
	bitset<2*BYTE4_SIZE+1> carry;
	*cflag = 0;
	*oflag = 0;
	for (int i=0;i<2*BYTE4_SIZE;++i)
	{
		
		int a = A.test(i);
		int b = B.test(i);
		int c = carry.test(i);
		int result = a + b + c;
		switch (result)
		{
			case 0:
				ret.reset(i);
				carry.reset(i+1);
				break;
			case 1:
				ret.set(i);
				carry.reset(i+1);
				break;
			case 2:
				ret.reset(i);
				carry.set(i+1);
				break;
			case 3:
				ret.set(i);
				carry.set(i+1);
				break;
		};
	}
	if(carry.test(2*BYTE4_SIZE) == true) *cflag = 1;
	if(A.test(A.size()-1) == true && B.test(B.size()-1) == true && ret.test(ret.size()-1) == false) *oflag = 1; 
	if(A.test(A.size()-1) == false && B.test(B.size()-1) == false && ret.test(ret.size()-1) == true) *oflag = 1; 
	return ret;
};

