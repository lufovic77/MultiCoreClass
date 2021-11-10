#include<iostream>
#include<fstream>
#include<string.h>
#include <algorithm>
#include <string>
#include <omp.h>

#define BILLION  1000000000L

using namespace std;

struct padded_int{
    int value;
    char padding[60];
} histogram[59];

struct padded_char{
    char str[30];
    char padded[34];
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

    auto strArr = new struct padded_char[N]; 
    //string strArr[N];

    for(i=0; i<N; i++)
        inputfile>>strArr[i].str;
    inputfile.close();

    auto outputArr = new struct padded_char[N];
    
    //omp_set_nested(true);

    int len, idx; 
    for (int decimal = 29; decimal>=0; decimal--){
        #pragma omp parallel private(i) shared(histogram) 
        {
            #pragma omp for schedule(static)
            for(i=0;i<59;i++)
                histogram[i].value = 0;
  
        }

        #pragma omp parallel private(i) shared(histogram, strArr, decimal) 
        {

            int h_private[59] = {0};

            #pragma omp for private(idx, len) schedule(static) 
            for (i = 0; i < N; i++){
                len = strlen(strArr[i].str);
                if(decimal < len)
                    idx = strArr[i].str[decimal]-'A'+1;
                else
                    idx = 0;
                h_private[idx] ++;
            }
            
            #pragma omp critical
            for(int n=0; n<59; ++n) {
                histogram[n].value += h_private[n];
            }
            
        }

        for (i = 1; i < 59; i++)
        histogram[i].value += histogram[i-1].value;

        for (i = N - 1; i >= 0; i--) {
            len = strlen(strArr[i].str);
            if(decimal < len) 
                idx = strArr[i].str[decimal]-'A'+1;
            else
                idx = 0;          
            strncpy(outputArr[(histogram[idx].value)-1].str, strArr[i].str, 30);
            histogram[idx].value--;
        }

        #pragma omp parallel private(i) shared(strArr, outputArr, decimal) 
        {
            #pragma omp for private(i) schedule(static)
            for (i = 0; i < N; i++)
                strncpy(strArr[i].str,outputArr[i].str,  30);
        }
    }
    

    cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
    for(i=pos; i<N && i<(pos+range); i++)
        cout<< i << ": " << strArr[i].str<<endl;
    cout<<endl;


    delete[] strArr;
    delete[] outputArr;
    return 0;
}
