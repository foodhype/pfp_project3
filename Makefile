run:
	g++ -std=c++0x main.cpp -o main -I$(TACC_PAPI_INC) -L$(TACC_PAPI_LIB) -lpapi && ./main; \
	rm main	
