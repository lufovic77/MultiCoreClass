#include<iostream>
#include<fstream>
#include<string.h>
#include <algorithm>
#include <string>
#include <omp.h>
#include <mpi.h>

#define BILLION  1000000000L

using namespace std;
struct padded_int{
    int value;
    char padding[60];
};

struct padded_char{
    char str[31];
    char padded[33];
};

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

    auto strArr = new struct padded_char[N];
    //char * strArr;
    if(my_rank == 0){
        for(i=0; i<N; i++){
            inputfile>>strArr[i].str;
        }   
    }

    inputfile.close(); 

    auto outputArr = new struct padded_char[N];
    int len, idx;
    auto histogram = new struct padded_int[59];

    auto str_buffer = new char [N/comm_size*31];
    auto str_private = new char[N/comm_size][31];
    
    for (int decimal = 29; decimal>=0; decimal--){
        for( i=0;i< 59;i++){
            histogram[i].value = 0;
        }

        // worker
        int h_private[59] = {0};
        if(my_rank != 0){
            MPI_Recv(str_buffer, 31*(N/comm_size), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            #pragma omp parallel private(i) shared(histogram, strArr, decimal) 
            {
                auto h_private_tmp = new struct padded_int[59];
                for( i=0;i< 59;i++)
                    h_private_tmp[i].value = 0;
                #pragma omp for private(idx, len) schedule(static) 
                for(i=0;i<N/comm_size;i++){
                    idx = 0;
                    strncpy(str_private[i], str_buffer + 31*i, 31);
                    len = strlen(str_private[i]);
                    if(decimal < len)
                        idx = str_private[i][decimal]-'A'+1;
                    else
                        idx = 0;
                    h_private_tmp[idx].value++;
                }
                #pragma omp critical
                for(int n=0; n<59; ++n) 
                    h_private[n] += h_private_tmp[n].value;
            }
            MPI_Send(h_private, 59, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        else{ // master
            for(i=1;i<comm_size;i++){
                int start = (N/comm_size) * i;
                int end = (N/comm_size) * (i+1);
                idx = 0;
                
                for (int q=start; q<end; q++) 
                    strncpy(str_buffer+(idx++)*31, strArr[q].str, 31);
                MPI_Send(str_buffer, 31*(N/comm_size), MPI_CHAR, i, 0, MPI_COMM_WORLD);
                MPI_Recv(h_private, 59, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                histogram[0].value += h_private[0];
                for (int q = 1; q < 59; q++){
                    histogram[q].value += h_private[q];
                    h_private[q-1] = 0;
                }
                h_private[58] = 0;
            }
            #pragma omp parallel
            {   
                auto h_private_tmp = new struct padded_int[59];
                for( i=0;i< 59;i++)
                    h_private_tmp[i].value = 0;

                #pragma omp for schedule(static)
                for (int q=0; q<N/comm_size; q++) 
                    strncpy(str_buffer+q*31,  strArr[q].str, 31);

                #pragma omp for private(idx, len) schedule(static) 
                for(i=0;i<N/comm_size;i++){
                    idx = 0;
                    strncpy(str_private[i],str_buffer + 31*i, 31);
                    len = strlen(str_private[i]);
                    if(decimal < len)
                        idx = str_private[i][decimal]-'A'+1;
                    else
                        idx = 0;
                    h_private_tmp[idx].value++;
                }
                #pragma omp critical
                for(int n=0; n<59; ++n) 
                    h_private[n] += h_private_tmp[n].value;
            }

            for (int q = 0; q < 59; q++)
                histogram[q].value += h_private[q];

        }

        if(my_rank == 0){
            for (i = 1; i < 59; i++)
            histogram[i].value += histogram[i-1].value;

            for (i = N - 1; i >= 0; i--) {
                len = strlen(strArr[i].str);
                if(decimal < len) 
                    idx = strArr[i].str[decimal]-'A'+1;
                else
                    idx = 0;          
                strncpy(outputArr[(histogram[idx].value)-1].str, strArr[i].str, 31);
                histogram[idx].value--;
            }

            #pragma omp parallel shared(strArr, outputArr, decimal) 
            {
                #pragma omp for private(i) schedule(static)
                for (i = 0; i < N; i++)
                    strncpy(strArr[i].str,outputArr[i].str,  31);
            }
        }
    }

    if(my_rank == 0){
        
        cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
        for(i=pos; i<N && i<(pos+range); i++){
            cout<< i << ": " << strArr[i].str<<endl;
        }
    }
    delete[] outputArr;
    delete[] strArr;
    
    MPI_Finalize();
    return 0;
}
