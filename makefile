cc=g++
cflags=-c
OBJS=objs

all: objectFolder $(OBJS)/Population.o $(OBJS)/Decoder.o $(OBJS)/Operations.o $(OBJS)/Lists.o $(OBJS)/Functions.o $(OBJS)/NumberGen.o

SyscallExperiment: objectFolder $(OBJS)/SyscallExperiment.o $(OBJS)/Population.o $(OBJS)/Decoder.o $(OBJS)/Operations.o $(OBJS)/Lists.o $(OBJS)/ProbabilityDistro.o $(OBJS)/Functions.o $(OBJS)/NumberGen.o 
	$(cc) $(OBJS)/SyscallExperiment.o $(OBJS)/Population.o $(OBJS)/Decoder.o $(OBJS)/Operations.o $(OBJS)/Lists.o $(OBJS)/ProbabilityDistro.o $(OBJS)/Functions.o $(OBJS)/NumberGen.o -o SyscallExperiment

objectFolder:
	mkdir -p $(OBJS)/
	
$(OBJS)/Population.o: source/Population.hpp source/Population.cpp
	$(cc) $(cflags) source/Population.cpp -o $(OBJS)/Population.o

$(OBJS)/Decoder.o: source/Decoder.hpp source/Decoder.cpp
	$(cc) $(cflags) source/Decoder.cpp -o $(OBJS)/Decoder.o

$(OBJS)/Operations.o: source/Operations.hpp source/Operations.cpp
	$(cc) $(cflags) source/Operations.cpp -o $(OBJS)/Operations.o

$(OBJS)/Lists.o: source/Lists.hpp source/Lists.cpp
	$(cc) $(cflags) source/Lists.cpp -o $(OBJS)/Lists.o

$(OBJS)/ProbabilityDistro.o: source/ProbabilityDistro.hpp source/ProbabilityDistro.cpp
	$(cc) $(cflags) source/ProbabilityDistro.cpp -o $(OBJS)/ProbabilityDistro.o

$(OBJS)/Functions.o: source/Functions.hpp source/Functions.cpp
	$(cc) $(cflags) source/Functions.cpp -o $(OBJS)/Functions.o

$(OBJS)/NumberGen.o: source/NumberGen.hpp source/NumberGen.cpp
	$(cc) $(cflags) source/NumberGen.cpp -o $(OBJS)/NumberGen.o

$(OBJS)/RuntimeFitness.o: source/asmRuntime/RuntimeFitness.cc source/asmRuntime/RuntimeFitness.hpp
	$(cc) $(cflags) source/asmRuntime/RuntimeFitness.cc -o $(OBJS)/RuntimeFitness.o

$(OBJS)/asmRuntime.o: source/asmRuntime/asmRuntime.cc source/asmRuntime/asmRuntime.hpp
	$(cc) $(cflags) source/asmRuntime/asmRuntime.cc -o $(OBJS)/asmRuntime.o

$(OBJS)/Analyzer.o: source/Analyzer.cpp 
	$(cc) $(cflags) source/Analyzer.cpp -o $(OBJS)/Analyzer.o

$(OBJS)/experiment.o: source/experiment.cpp
	$(cc) $(cflags) source/experiment.cpp -o $(OBJS)/experiment.o

$(OBJS)/SyscallExperiment.o: source/SyscallExperiment.cpp
	$(cc) $(cflags) source/SyscallExperiment.cpp -o $(OBJS)/SyscallExperiment.o

$(OBJS)/analyzePopulation.o: source/analyzePopulation.cpp
	$(cc) $(cflags) source/analyzePopulation.cpp -o $(OBJS)/analyzePopulation.o

clean:
	rm -rf $(OBJS)/ SyscallExperiment

	
