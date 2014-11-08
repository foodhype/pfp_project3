run:
	g++ -O2 -std=c++0x main.cpp LIFOScheduler.h FIFOScheduler.h PriorityScheduler.h -pthread -g -o main -I$(TACC_PAPI_INC) -L$(TACC_PAPI_LIB) -lpapi && ./main
