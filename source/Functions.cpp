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

# include "Functions.hpp"
/********************************************************************************
 * Trims the leading and trailing whitespaces
 */
string string_trim(string str)
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

/********************************************************************************
 * Given the file name, counts the lines by counting \n's (skips back to back \n's)
 */
int line_count(string filename)
{
	ifstream fin(filename.c_str());
	assert(fin);	
	int count = 0;
	char ch1 = ' ', ch2 = ' ';
	while (fin.get(ch2))
	{
		if (ch2 == '\n' && ch1 != '\n') count++;
		ch1 = ch2;
	}
	fin.close();
	return count;
};

/********************************************************************************
 * Reporting function (Currently unused)
 */
void send_message(string message, int priority)
{
	if (DebugLevel >= priority)
	{
		if (priority == 0) cout<<" [ X ] "<<message<<endl;
		else if (priority == 1) cout<<" [ ? ] "<<message<<endl;
		else cout<<" [ i ] "<<message<<endl;
	}
};

/********************************************************************************
 * Calculates fitness for 2 boxes problem over 50 examplars
 */
double twoboxes_fitness(string prog, NumberGen *numg, double *eachreg)
{
	double *x1 = new double[50];
	double *x2 = new double[50];
	double *x3 = new double[50];
	double *x4 = new double[50];
	double *x5 = new double[50];
	double *x6 = new double[50];
	double eax = 0, ebx = 0, ecx = 0, edx = 0;
	double fitness = 100000000; // an impossible max fit
	double *ptr1;
	double *ptr2;
	for (int i=0;i<4;++i)
		eachreg[i] = 0;
	// prep test cases
	for (int i = 0; i<50; ++i)
	{
		x1[i] = numg->ran3();
		x2[i] = numg->ran3();
		x3[i] = numg->ran3();
		x4[i] = numg->ran3();
		x5[i] = numg->ran3();
		x6[i] = numg->ran3();
	}
	for (int j = 0; j < 50 ; ++j )
	{
		eax = 0; ebx = 0; ecx = 0; edx = 0;
		string line = "";
		for (int i=0; i < prog.size(); ++ i)
		{	
			if ( prog.at(i) == '\n')
			{
				if(line.size() > 3) /// At least it is an instruction
				{
					int idx1 = line.find(" ");
					int idx2 = line.find(",");
					string ins = line.substr(0, idx1);
					string p1 = line.substr(idx1 + 1, idx2 - idx1 -1);
					string p2 = line.substr(idx2 + 2, line.size() - idx2);
					//cout<<"ins="<<ins<<"-"<<p1<<"-"<<p2<<endl;
					if (p1 == "EAX") ptr1 = &eax;
					if (p1 == "EBX") ptr1 = &ebx;
					if (p1 == "ECX") ptr1 = &ecx;
					if (p1 == "EDX") ptr1 = &edx;
					if (p2 == "EAX") ptr2 = &eax;
					if (p2 == "EBX") ptr2 = &ebx;
					if (p2 == "ECX") ptr2 = &ecx;
					if (p2 == "EDX") ptr2 = &edx;
					if (p2 == "x1")  ptr2 = &x1[j];
					if (p2 == "x2")  ptr2 = &x2[j];
					if (p2 == "x3")  ptr2 = &x3[j];
					if (p2 == "x4")  ptr2 = &x4[j];
					if (p2 == "x5")  ptr2 = &x5[j];
					if (p2 == "x6")  ptr2 = &x6[j];
					//cout<<"P1="<<*ptr1<<" P2="<<*ptr2<<endl;
					if ( ins == "ADD" )
						*ptr1 = *ptr1 + *ptr2;
					if ( ins == "SUB" )
						*ptr1 = *ptr1 - *ptr2;
					if ( ins == "MUL" )
						*ptr1 = *ptr1 * *ptr2;
					if ( ins == "DIV" )
					{
						if (*ptr2 !=0) *ptr1 = *ptr1 / *ptr2;
					}
				}
				line = "";
			}
			else
			{
				line += prog.at(i); 
			}
		}
		// At the end of the program
		double correct = x1[j] * x2[j] * x3[j] - x4[j] * x5[j] * x6[j];
		//cout<<"Correct="<<correct;
		eachreg[0] += ((eax - correct) * (eax - correct));
		eachreg[1] += ((ebx - correct) * (ebx - correct));
		eachreg[2] += ((ecx - correct) * (ecx - correct)); 
		eachreg[3] += ((edx - correct) * (edx - correct));   
	}
	delete[] x1; delete[] x2; delete[] x3; delete[] x4; delete[] x5; delete[] x6;
	//delete ptr1; delete ptr2;
	for (int k=0; k<4; ++k)
	{
		//cout<<"eachreg["<<k<<"]="<<eachreg[k]<<endl;
		if (eachreg[k] < fitness) fitness = eachreg[k]; 
	}
	//cout<<" Fit="<<fitness<<endl;
	return fitness;
};
/********************************************************************************
 * Writes the text to a file.
 */
void write2file(string text, string filename)
{
	ofstream fout(filename.c_str());
	if (!fout) cout << "write2file: Unable to open " << filename << ".\n";
	fout<<text.c_str();
	fout.close();
};

/********************************************************************************
 * My itoa, make sure the value in buffer is COPIED before use
 */
string my_itoa(int input)
{
	static char buffer[16];
    snprintf(buffer,sizeof(buffer),"%d",input);
    string ret (buffer);
	return ret;
};

/***********************************************************
 *
 */
 void create_pHmr_schema(string out_prog, string schema_file, int run, int win_size)
 {
	NumberGen num(run);
	int *tmp =  new int[win_size]; assert(tmp); 
	for(int i=0;i<win_size;++i)
	{
		// HARDCODED VALUE: 20 is the window size that pHmr monitors,
		// depending on the schema mask it only sees 9 of them
		tmp[i] = num.uniform_intgen(0, 20 - 1);
		for(int j=0;j<i;++j)
		{
			if (tmp[j] == tmp[i]) i--;
		}
		//cout<<tmp[i]<<endl;
	}
	string filename1 = "pHmr/"+schema_file;
	//string filename2 = out_prog + ".schema";
	ofstream fout1(filename1.c_str());
	//ofstream fout2(filename2.c_str());
	assert(fout1);//assert(fout2);
	int a_big_number = 9999999;
	for(int i=0;i<win_size;++i)
	{
		int min = a_big_number;
		int min_loc = -1;
		for(int j=0;j<win_size;++j)
		{
			if (tmp[j] < min)
			{
				min = tmp[j];
				min_loc = j;
			}
		}
		if(i != 0)
		{ 
			fout1<<",";
			//fout2<<",";
		}
		fout1<<min;
		///fout2<<min;
		tmp[min_loc] = a_big_number + 1;
	}
	fout1<<"\n";
	delete[] tmp;
	fout1.close();
	string command = "cat pHmr/" + schema_file + ">> " + out_prog + ".schemas";
	system(command.c_str());
	//fout2.close();
 };
/***********************************************************
 *
 */
void train_pHmr(string schema_file, string dump_file, string app)
{
	//dump the old DB with schema
	if ( (app != "traceroute") && (app != "ftp") && (app != "restore") && (app != "samba") )
	{
		cout<<"train_pHMmr: Invalid application... quiting."<<endl;
		exit(1);
	}
	if(app == "traceroute")
	{
		string cmd = ""; 
		cmd = "pHmr/train_pHmr 20 N/A pHmr/traceroute.db traces/traceroute/case1.strace.processed pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/traceroute.db pHmr/traceroute.db traces/traceroute/case2.strace.processed pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/traceroute.db pHmr/traceroute.db traces/traceroute/case3.strace.processed pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/traceroute.db pHmr/traceroute.db traces/traceroute/case4.strace.processed pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/traceroute.db pHmr/traceroute.db traces/traceroute/case6.strace.processed pHmr/"+schema_file;
		system(cmd.c_str());
	}
	if(app == "ftp")
	{
		string cmd = ""; 
		cmd = "pHmr/train_pHmr 20 N/A pHmr/ftp.db traces/ftp/normal1.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal2.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal3.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal4.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal5.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal6.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal7.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal8.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal9.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/ftp.db pHmr/ftp.db traces/ftp/normal10.ftp.processed  pHmr/"+schema_file;
		system(cmd.c_str());
	}
	if(app == "restore")
	{
		string cmd = ""; 
		cmd = "pHmr/train_pHmr 20 N/A pHmr/restore.db traces/restore/restore_helpscr.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/restore.db pHmr/restore.db traces/restore/restore_test1_full.strace.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/restore.db pHmr/restore.db traces/restore/restore_test1_incr.strace.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/restore.db pHmr/restore.db traces/restore/restore_test2_full.strace.processed  pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/restore.db pHmr/restore.db traces/restore/restore_test2_incr.strace.processed  pHmr/"+schema_file;
		system(cmd.c_str());
	}
	if(app == "samba")
	{
		string cmd = ""; 
		cmd = "pHmr/train_pHmr 20 N/A pHmr/samba.db traces/samba/smb_ls_cd_ls_edit.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/samba.db pHmr/samba.db traces/samba/smb_ls_cp.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/samba.db pHmr/samba.db traces/samba/smb_mntbadPW.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/samba.db pHmr/samba.db traces/samba/smb_mount.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/samba.db pHmr/samba.db traces/samba/smb_passwd.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
		cmd = "pHmr/train_pHmr 20 pHmr/samba.db pHmr/samba.db traces/samba/smb_umount.strace.processed   pHmr/"+schema_file;
		system(cmd.c_str());
	}
	string command = "echo '************************************' >> "+dump_file;
	system(command.c_str());
	command = "cat pHmr/" + schema_file + " >> "+dump_file;
	system(command.c_str());
	command = "echo '************************************' >> "+dump_file;
	system(command.c_str());
	command = "cat pHmr/" + app + ".db >> "+dump_file;
	system(command.c_str());
	//system("pwd");
	
	
	
};


