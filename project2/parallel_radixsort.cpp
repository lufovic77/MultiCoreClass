#include<iostream>
#include<fstream>
#include<string.h>
#include <algorithm>
#include <string>
#include <chrono>

using namespace std;

struct padded_int{
    int value;
    char padding[60];
} histogram[59];

int main(int argc, char* argv[])
{
    char tmpStr[30];
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

std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();


    auto strArr = new char[N][30];
    //string strArr[N];

    for(i=0; i<N; i++)
        inputfile>>strArr[i];
    // |A|D|I|j|d
    // |B| | | | 
    // 이런식으로 저장되어 있음. 
    inputfile.close();
    auto outputArr = new char[N][30];
    
    for (int decimal = 29; decimal>=0; decimal--){
        for(i=0;i<59;i++)
            histogram[i].value = 0;
        int len, idx;
        #pragma omp parallel num_threads(10)
        {
            #pragma omp for private(idx, len, i) 
            for (i = 0; i < N; i++){
                //int len = strArr[i].length();
                len = strlen(strArr[i]);
                if(decimal < len)
                    idx = strArr[i][decimal]-'A'+1;
                else
                    idx = 0;
                #pragma omp atomic
                histogram[idx].value++;
            }
            #pragma omp single
            {
                for (i = 1; i < 59; i++)
                   histogram[i].value += histogram[i-1].value;
            }
            // Build the output array

            #pragma omp for ordered private(idx, len, i) 
            for (i = N - 1; i >= 0; i--) {
                //int len = strArr[i].length();
                len = strlen(strArr[i]);
                if(decimal < len) 
                    idx = strArr[i][decimal]-'A'+1;
                else
                    idx = 0;            
                #pragma omp ordered 
                {
                    strncpy(outputArr[(histogram[idx].value)-1], strArr[i], 30);
                    histogram[idx].value--;
                }
            
            }
            #pragma omp for private(i) 
                for (i = 0; i < N; i++)
                    strncpy(strArr[i],outputArr[i],  30);
        }
    }
    

    cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
    for(i=pos; i<N && i<(pos+range); i++)
        cout<< i << ": " << strArr[i]<<endl;
    cout<<endl;



    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();


    //delete[] strArr;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()/(float(1000.0)) << "[s]" << std::endl;
    return 0;
}
