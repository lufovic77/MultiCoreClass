#include<iostream>
#include<fstream>
#include<string.h>
#include <algorithm>
#include <string>
#include <omp.h>
#include <mpi.h>
#include <chrono>

#define BILLION  1000000000L

using namespace std;

int main(int argc, char* argv[])
{
    int i, j, N, pos, range, ret;

    if(argc<5){
	    cout << "Usage: " << argv[0] << " filename number_of_strings pos range" << endl;
	    return 0;
    }

    ifstream inputfile(argv[1]);
    if(!inputfile.is_open()){
	    cout << "Unable to open file" << endl;
	    return 0;
    }

    ret=sscanf(argv[2],"%d", &N);
    if(ret==EOF || N<=0){
	    cout << "Invalid number" << endl;
	    return 0;
    }

    ret=sscanf(argv[3],"%d", &pos);
    if(ret==EOF || pos<0 || pos>=N){
	    cout << "Invalid position" << endl;
	    return 0;
    }

    ret=sscanf(argv[4],"%d", &range);
    if(ret==EOF || range<0 || (pos+range)>N){
	    cout << "Invalid range" << endl;
	    return 0;
    }

    //auto strArr = new struct padded_char[N]; 

    int comm_size;
    int my_rank;
    
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    auto strArr = new char[N][31];
    //char * strArr;
    if(my_rank == 0){
        for(i=0; i<N; i++){
            inputfile>>strArr[i];
        }   
    }

    inputfile.close(); 

std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    auto outputArr = new char [N][31];
    int len, idx;
    int histogram[59] = {0};

    char* str_buffer;
    if (( str_buffer = (char*)malloc((N/comm_size)*31*sizeof(char))) == NULL) {
        printf("Malloc error");
        exit(1);
    }
    auto str_private = new char[N/comm_size][31];
    
    for (int decimal = 29; decimal>=0; decimal--){
        for( i=0;i< 59;i++)
            histogram[i] = 0;

        // worker
        int h_private[59] = {0};
        if(my_rank != 0){
            MPI_Recv(str_buffer, 31*(N/comm_size), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            #pragma omp parallel private(i) shared(histogram, strArr, decimal) 
            {
                int h_private_tmp[59] = {0};
                #pragma omp for private(idx, len) schedule(static) 
                for(i=0;i<N/comm_size;i++){
                    idx = 0;
                    strncpy(str_private[i], str_buffer + 31*i, 31);
                    len = strlen(str_private[i]);
                    if(decimal < len)
                        idx = str_private[i][decimal]-'A'+1;
                    else
                        idx = 0;
                    h_private_tmp[idx]++;
                }
                #pragma omp critical
                for(int n=0; n<59; ++n) 
                    h_private[n] += h_private_tmp[n];
            }
            MPI_Send(h_private, 59, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        else{ // master
            for(i=1;i<comm_size;i++){
                int start = (N/comm_size) * i;
                int end = (N/comm_size) * (i+1);
                idx = 0;
                
                for (int q=start; q<end; q++) {
                    for(int j=0;j<31;j++)
                        str_buffer[idx++] = strArr[q][j];
                }
                MPI_Send(str_buffer, 31*(N/comm_size), MPI_CHAR, i, 0, MPI_COMM_WORLD);
                MPI_Recv(h_private, 59, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                histogram[0] += h_private[0];
                for (int q = 1; q < 59; q++){
                    histogram[q] += h_private[q];
                    h_private[q-1] = 0;
                }
                h_private[58] = 0;
            }
            #pragma omp parallel
            {
                #pragma omp  for schedule(static) collapse(2)
                for (int q=0; q<N/comm_size; q++) {
                        for(int j=0;j<31;j++)
                            str_buffer[q*31 + j] = strArr[q][j];
                }
                int h_private_tmp[59] = {0};
                #pragma omp for private(idx, len) schedule(static) 
                for(i=0;i<N/comm_size;i++){
                    idx = 0;
                    strncpy(str_private[i], str_buffer + 31*i, 31);
                    len = strlen(str_private[i]);
                    if(decimal < len)
                        idx = str_private[i][decimal]-'A'+1;
                    else
                        idx = 0;
                    h_private_tmp[idx]++;
                }
                #pragma omp critical
                for(int n=0; n<59; ++n) 
                    h_private[n] += h_private_tmp[n];
            }

            for (int q = 0; q < 59; q++)
                histogram[q] += h_private[q];

        }

        if(my_rank == 0){
            for (i = 1; i < 59; i++)
            histogram[i] += histogram[i-1];

            for (i = N - 1; i >= 0; i--) {
                len = strlen(strArr[i]);
                if(decimal < len) 
                    idx = strArr[i][decimal]-'A'+1;
                else
                    idx = 0;          
                strncpy(outputArr[(histogram[idx])-1], strArr[i], 31);
                histogram[idx]--;
            }

            #pragma omp parallel private(i) shared(strArr, outputArr, decimal) 
            {
                #pragma omp for private(i) schedule(static)
                for (i = 0; i < N; i++)
                    strncpy(strArr[i],outputArr[i],  31);
            }
        }
    }

    if(my_rank == 0){
        
        cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
        for(i=pos; i<N && i<(pos+range); i++){
            cout<< i << ": " << strArr[i]<<endl;
        }
        

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        //delete[] strArr;
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()/(float(1000.0)) << "[s]" << std::endl;

    }
    delete[] outputArr;
    delete[] strArr;
    
    MPI_Finalize();
    return 0;
}
