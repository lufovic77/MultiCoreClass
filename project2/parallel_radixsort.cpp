#include<iostream>
#include<fstream>
#include<string.h>
#include <algorithm>
#include <string>
#include <chrono>

using namespace std;

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
/*
    for(i=1; i<N; i++)
    {
        for(j=1; j<N; j++)
        {
            if(strncmp(strArr[j-1], strArr[j],30)>0)
            {
                strncpy(tmpStr, strArr[j-1], 30);
                strncpy(strArr[j-1], strArr[j], 30);
                strncpy(strArr[j], tmpStr, 30);
            }
        }
    } // bubble sort
*/
    //string outputArr[N];
    auto outputArr = new char [N][30];

    #pragma omp parallel num_threads(4)
    for (int decimal = 29; decimal>=0; decimal--){
        int histogram[59] = {0};
        int len, idx;
        #pragma omp parallel for private(idx, len, i) shared(decimal, histogram) 
        for (i = 0; i < N; i++){
            //int len = strArr[i].length();
            len = strlen(strArr[i]);
            if(decimal < len)
                idx = strArr[i][decimal]-'A'+1;
            else
                idx = 0;
            
            histogram[idx]++;
        }

        #pragma omp critical
        if(decimal == 29){
        for(i=0;i<50;i++)
        cout<<histogram[i];
        cout<<endl;
        }

        #pragma omp critical
        for (i = 1; i < 59; i++)
            histogram[i] += histogram[i - 1];
    
        // Build the output array

        //#pragma omp parallel for private(idx, len, i) shared(decimal, histogram, outputArr)

        #pragma omp critical
        for (i = N - 1; i >= 0; i--) {
            //int len = strArr[i].length();
            len = strlen(strArr[i]);
            if(decimal < len) 
                idx = strArr[i][decimal]-'A'+1;
            else
                idx = 0;
            //outputArr[histogram[idx]-1] = strArr[i];

            // 여기서 dependency가 생기는게 문제다 지금 !!!!!!!!!
            strncpy(outputArr[histogram[idx]-1], strArr[i], 30);
            histogram[idx]--;
        
        }
        //Jd 
        //Y 
        //Q 
        //BXWXGjX 
        //`eHUd[mb 
        //U 
        //kAvHxTSid 
        //N 
        //b

        #pragma omp critical
        for (i = 0; i < N; i++)
            strncpy(strArr[i],outputArr[i],  30);
            //strArr[i] = outputArr[i];
    }
    

    cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
    for(i=pos; i<N && i<(pos+range); i++)
        cout<< i << ": " << strArr[i]<<endl;
    cout<<endl;



    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();


    //delete[] strArr;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    return 0;
}
