run:
	g++ -O2 -std=c++0x main.cpp BlockingQueue.h -pthread -g -o main -I$(TACC_PAPI_INC) -L$(TACC_PAPI_LIB) -lpapi && ./main
