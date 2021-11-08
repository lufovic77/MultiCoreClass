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
    char tmpStr[30];
    int i, j, N, pos, range, ret;

	struct timespec start,stop;

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

	clock_gettime(CLOCK_REALTIME, &start);

    //auto strArr = new char[N][30];
    auto strArr = new struct padded_char[N]; 
    //string strArr[N];

    for(i=0; i<N; i++)
        inputfile>>strArr[i].str;
    // |A|D|I|j|d
    // |B| | | | 
    // 이런식으로 저장되어 있음. 
    inputfile.close();

	clock_gettime(CLOCK_REALTIME, &start);
    auto outputArr = new struct padded_char[N];
    
    //omp_set_nested(true);

    int len, idx; 
    for (int decimal = 29; decimal>=0; decimal--){
        #pragma omp parallel private(i) shared(histogram, strArr, outputArr, decimal) 
        {
            #pragma omp for private(i) schedule(static)
            for(i=0;i<59;i++)
                histogram[i].value = 0;
  
        }

        #pragma omp parallel private(i) shared(histogram, strArr, outputArr, decimal) 
        {

            int h_private[59] = {0};

            #pragma omp for private(idx, len, i) schedule(static) 
            for (i = 0; i < N; i++){
                //int len = strArr[i].length();
                len = strlen(strArr[i].str);
                if(decimal < len)
                    idx = strArr[i].str[decimal]-'A'+1;
                else
                    idx = 0;
                h_private[idx] ++;
                //#pragma omp atomic
                //histogram[idx].value+=1;
            }
            
            #pragma omp critical
            for(int n=0; n<59; ++n) {
                histogram[n].value += h_private[n];
            }
            
        }

      //      #pragma omp single
      //      {
                for (i = 1; i < 59; i++)
                histogram[i].value += histogram[i-1].value;
                for (i = N - 1; i >= 0; i--) {
                //int len = strArr[i].length();
                    len = strlen(strArr[i].str);
                    if(decimal < len) 
                        idx = strArr[i].str[decimal]-'A'+1;
                    else
                        idx = 0;          
                    strncpy(outputArr[(histogram[idx].value)-1].str, strArr[i].str, 30);
                    histogram[idx].value--;
                }
          //  }

        #pragma omp parallel private(i) shared(histogram, strArr, outputArr, decimal) 
        {
            #pragma omp for private(i) schedule(static)
            for (i = 0; i < N; i++)
                strncpy(strArr[i].str,outputArr[i].str,  30);

            //#pragma omp barrier
        }
    }
    

    cout<<"\nStrings (Names) in Alphabetical order from position " << pos << ": " << endl;
    for(i=pos; i<N && i<(pos+range); i++)
        cout<< i << ": " << strArr[i].str<<endl;
    cout<<endl;

	clock_gettime(CLOCK_REALTIME, &stop);


    delete[] strArr;
    delete[] outputArr;
    std::cout << "Elapsed time: " << (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION << " sec" << "\n";
    return 0;
}
