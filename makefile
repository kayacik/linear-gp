cc=g++
cflags=-c
OBJS=objs
BIN=bin
SRC=source

all: Folders $(OBJS)/Population.o $(OBJS)/Decoder.o $(OBJS)/Operations.o $(OBJS)/Lists.o $(OBJS)/Functions.o $(OBJS)/NumberGen.o

experiment: Folders detector-pH detector-pHsm detector-NNet detector-HMM $(OBJS)/SyscallExperiment.o $(OBJS)/Population.o $(OBJS)/Decoder.o $(OBJS)/Operations.o $(OBJS)/Lists.o $(OBJS)/ProbabilityDistro.o $(OBJS)/Functions.o $(OBJS)/NumberGen.o 
	$(cc) $(OBJS)/SyscallExperiment.o $(OBJS)/Population.o $(OBJS)/Decoder.o $(OBJS)/Operations.o $(OBJS)/Lists.o $(OBJS)/ProbabilityDistro.o $(OBJS)/Functions.o $(OBJS)/NumberGen.o -o $(BIN)/syscall-experiment

Folders:
	mkdir -p $(OBJS)/ $(BIN)/

detector-pH:
	$(cc) $(SRC)/pH/pH.cpp -o $(BIN)/pH
	$(cc) $(SRC)/pH/train_pH.cpp -o $(BIN)/train-pH

detector-pHsm:
	$(cc) $(SRC)/pHsm/pHsm.cpp -o $(BIN)/pHsm
	$(cc) $(SRC)/pHsm/train_pHsm.cpp -o $(BIN)/train-pHsm

detector-NNet:
	$(cc) $(SRC)/NeuralNetSim/neural_net_sim.cpp -o $(BIN)/nnsim

detector-HMM:
	$(cc) $(SRC)/HiddenMarkovModel/HMM.cpp -o $(BIN)/hmm
	$(cc) $(SRC)/HiddenMarkovModel/train_HMM.cpp -o $(BIN)/train-hmm
	
$(OBJS)/Population.o: $(SRC)/Population.hpp $(SRC)/Population.cpp
	$(cc) $(cflags) $(SRC)/Population.cpp -o $(OBJS)/Population.o

$(OBJS)/Decoder.o: $(SRC)/Decoder.hpp $(SRC)/Decoder.cpp
	$(cc) $(cflags) $(SRC)/Decoder.cpp -o $(OBJS)/Decoder.o

$(OBJS)/Operations.o: $(SRC)/Operations.hpp $(SRC)/Operations.cpp
	$(cc) $(cflags) $(SRC)/Operations.cpp -o $(OBJS)/Operations.o

$(OBJS)/Lists.o: $(SRC)/Lists.hpp $(SRC)/Lists.cpp
	$(cc) $(cflags) $(SRC)/Lists.cpp -o $(OBJS)/Lists.o

$(OBJS)/ProbabilityDistro.o: $(SRC)/ProbabilityDistro.hpp $(SRC)/ProbabilityDistro.cpp
	$(cc) $(cflags) $(SRC)/ProbabilityDistro.cpp -o $(OBJS)/ProbabilityDistro.o

$(OBJS)/Functions.o: $(SRC)/Functions.hpp $(SRC)/Functions.cpp
	$(cc) $(cflags) $(SRC)/Functions.cpp -o $(OBJS)/Functions.o

$(OBJS)/NumberGen.o: $(SRC)/NumberGen.hpp $(SRC)/NumberGen.cpp
	$(cc) $(cflags) $(SRC)/NumberGen.cpp -o $(OBJS)/NumberGen.o

$(OBJS)/RuntimeFitness.o: $(SRC)/asmRuntime/RuntimeFitness.cc $(SRC)/asmRuntime/RuntimeFitness.hpp
	$(cc) $(cflags) $(SRC)/asmRuntime/RuntimeFitness.cc -o $(OBJS)/RuntimeFitness.o

$(OBJS)/asmRuntime.o: $(SRC)/asmRuntime/asmRuntime.cc $(SRC)/asmRuntime/asmRuntime.hpp
	$(cc) $(cflags) $(SRC)/asmRuntime/asmRuntime.cc -o $(OBJS)/asmRuntime.o

$(OBJS)/Analyzer.o: $(SRC)/Analyzer.cpp 
	$(cc) $(cflags) $(SRC)/Analyzer.cpp -o $(OBJS)/Analyzer.o

$(OBJS)/experiment.o: $(SRC)/experiment.cpp
	$(cc) $(cflags) $(SRC)/experiment.cpp -o $(OBJS)/experiment.o

$(OBJS)/SyscallExperiment.o: $(SRC)/SyscallExperiment.cpp
	$(cc) $(cflags) $(SRC)/SyscallExperiment.cpp -o $(OBJS)/SyscallExperiment.o

$(OBJS)/analyzePopulation.o: $(SRC)/analyzePopulation.cpp
	$(cc) $(cflags) $(SRC)/analyzePopulation.cpp -o $(OBJS)/analyzePopulation.o

clean:
	rm -rf $(OBJS)/ $(BIN)/

	
