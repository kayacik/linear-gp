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
 *					     (Header File)
 *
 * For fitness calculation, I need to evaulate wheter the
 * given set of assembly instructions (i.e. the shellcode)
 * would execute succesfully. Given the definition of 
 * instructions (in the form of class functions) and the 
 * set of registers that the runtime environment has, this
 * program determines the values in registers after the 
 * execution.
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <math.h>
#include <sstream>
#include <vector>
#include <ctype.h>
#include <stdint.h>
#include <bitset>
#define UNK 0xFFFFFFFF
#define DUMP_ERR 1
#define DUMP_INFO 1
#define REG_COUNT 10
#define SRCREG 110
#define DSTREG 111
#define STACK_SIZE 1000
// for bitset
#define BYTE4_SIZE 32
/// Masks ///
#define RESET_HI16 0x0000FFFF
#define RESET_LO16 0xFFFF0000
#define RESET_HI8  0xFFFF00FF
#define RESET_LO8  0xFFFFFF00
using namespace std;
typedef unsigned int REG32; // Size of REG32 should be 4
enum { CF = 0, PF = 2, AF = 4, ZF = 6, SF = 7, TF = 8, IF = 9, DF = 10, OF = 11, NT = 14, RF = 16 };

class asmRuntime
{
	//////// Public members //////////
	public:
	//////// Public variables ////////
	REG32 registers[REG_COUNT]; 
	string names[REG_COUNT];//={"EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP", "EIP", "EFLAGS"};
	REG32 *stack;
	REG32 flags[32]; // There are 32 flags (used + unused) in IA32 architecture
	int stack_last;
	string program;
	//////// Public functions ////////
	asmRuntime();
	~asmRuntime();
	asmRuntime(string program);
	asmRuntime(char *file);
	int doCheck();
	int decodeInstruction(string ins);
	int run();
	void print_err(char *str);
	void print_info(char *str);
	void report();
	string toUpper(string str);
	string toLower(string str);
	string trim(string str);
	unsigned int str2hex(string str); 
	unsigned int* findRegister(string rname, int *type);
	int stack_search(unsigned int* pattern, int plen, int from);
	//////// Private members //////////
	private:
	//////// Private variables ////////
	//////// Private functions ////////
	void initializeREGS();
	void initializeStack();
	void executeMOV(string p1, string p2);
	void executeCDQ(void);
	void executePUSH(string p1);
	void executeXOR(string p1, string p2);
	void executeINT(string p1);
	void executeBITS(string p1);
	void executeADD(string p1, string p2);
	void executeSUB(string p1, string p2);
	void executeINC(string p1);
	void executeDEC(string p1);
	void executeMUL(string p1);
	void executeDIV(string p1);
	void executeAND(string p1, string p2);
	void executeOR (string p1, string p2);
	void executeNOT(string p1);
	void applyMask(unsigned int *reg, int type, int where);
	void getCoordinates(string p, int *type, unsigned int **ptr, unsigned int *value);
	unsigned int getMask(unsigned int *reg, int type, int where);
	REG32 getValue(unsigned int *reg, int type);
	REG32 setValue(unsigned int *reg, int type, REG32 result);
	REG32 randomRegVal();
	int typeCheck(int type1, int type2);
	int stackAvailable();
	int checkSign(unsigned int val);
	int checkParity(unsigned int val);
	int checkZero(unsigned int val);
	bitset<BYTE4_SIZE>   binaryADD(bitset<BYTE4_SIZE> A, bitset<BYTE4_SIZE> B, int *cflag, int *oflag);
	bitset<BYTE4_SIZE>   binarySUB(bitset<BYTE4_SIZE> A, bitset<BYTE4_SIZE> B, int *cflag, int *oflag);
	bitset<2*BYTE4_SIZE> binaryMUL(bitset<BYTE4_SIZE> A, bitset<BYTE4_SIZE> B, int *cflag, int *oflag);
	bitset<2*BYTE4_SIZE> binaryADD(bitset<2*BYTE4_SIZE> A, bitset<2*BYTE4_SIZE> B, int *cflag, int *oflag);
};
