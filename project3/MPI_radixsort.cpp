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

    auto inputArr = new char[N][30];
    char * strArr;
    if(my_rank == 0){
        for(i=0; i<N; i++){
            inputfile>>inputArr[i];
        }

        if (( strArr = (char*)malloc(N*30*sizeof(char))) == NULL) {
            printf("Malloc error");
            exit(1);
        }

        for (i=0; i<N; i++) {
            for(int j=0;j<30;j++)
                strArr[i*30 + j] = inputArr[i][j];
        }

        for(i=0;i<N*30;i++)
        printf("|%c|", strArr[i]);
    }

    inputfile.close();

std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    auto outputArr = new char [N][30];
    int len, idx;
    int histogram[59] = {0};
    int buffer[59] = {0};

    char* str_buffer;
    if (( str_buffer = (char*)malloc((N/comm_size)*30*sizeof(char))) == NULL) {
        printf("Malloc error");
        exit(1);
    }
    
    for (int decimal = 29; decimal>=0; decimal--){
        cout<<decimal<<endl;
        for( i=0;i< 59;i++)
            histogram[i] = 0;

        int h_private[59] = {0};
        //for(int q = 1;q<comm_size;q++){
            //offset 계산해서 쪼개야한다 - 4개의 노드를 기준으로 하자 
            /*int start = (N/comm_size) * q;
            int end = (N/comm_size) * (q+1);
            idx = 0;
            for(int s = start;s<end;s++){
                cout<<strArr[s]<<endl;
                strncpy(str_buffer[idx++], strArr[s], 30);
            }*/
            //MPI_Send(str_buffer, 30, MPI_CHAR, q, 0, MPI_COMM_WORLD);
        MPI_Scatter(strArr, 30*(N/comm_size), MPI_CHAR, // send one row, which contains p integers
        str_buffer, 30*(N/comm_size), MPI_CHAR, // receive one row, which contains p integers
        0, MPI_COMM_WORLD);

        cout<<str_buffer<<endl;
        
        //MPI_Recv(h_private, 59, MPI_INT, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
/*
        for (i = 0; i < 59; i++)
            histogram[i] += h_private[i];

        if(my_rank != 0){ // worker
            int h_private[59] = {0};
            MPI_Recv(str_private, 30, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            cout<<"rank "<<my_rank<<endl;
            for(int i=0;i<N/comm_size;i++){
                cout<<str_private[i]<<endl;
                len = strlen(str_private[i]);
                if(decimal < len)
                    idx = str_private[i][decimal]-'A'+1;
                else
                    idx = 0;
                h_private[idx]++;
            }
            MPI_Send(h_private, 59, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

        if(my_rank == 0){
             
            for(int i=0;i<N/comm_size;i++){
                len = strlen(strArr[i]);
                if(decimal < len)
                    idx = strArr[i][decimal]-'A'+1;
                else
                    idx = 0;
                histogram[idx]++;
            }

            for (i = 0; i < 59; i++)
            cout<<histogram[i];
            cout<<endl;
            for (i = 1; i < 59; i++)
            histogram[i] += histogram[i-1];

            for (i = N - 1; i >= 0; i--) {
                len = strlen(strArr[i]);
                if(decimal < len) 
                    idx = strArr[i][decimal]-'A'+1;
                else
                    idx = 0;          
                strncpy(outputArr[(histogram[idx])-1], strArr[i], 30);
                histogram[idx]--;
            }

            #pragma omp parallel private(i) shared(strArr, outputArr, decimal) 
            {
                #pragma omp for private(i) schedule(static)
                for (i = 0; i < N; i++)
                    strncpy(strArr[i],outputArr[i],  30);
            }
            cout<<decimal<<endl;
        }
        */
    }

    if(my_rank == 0){
        /*
        cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
        for(i=pos; i<N && i<(pos+range); i++)
            cout<< i << ": " << strArr[i]<<endl;
        
        
        delete[] outputArr;
        delete[] strArr;
*/
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        //delete[] strArr;
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()/(float(1000.0)) << "[s]" << std::endl;

        MPI_Finalize();
    }
    
    return 0;
}
