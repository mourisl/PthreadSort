#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <parallel/algorithm>
#include <omp.h>

#include "PthreadSort.hpp"

//g++ -O3 -std=c++11 -lpthread -fopenmp example.cpp

using namespace std::chrono ;
using timer = std::chrono::high_resolution_clock;

int main()
{
  int n = 100000000 ;
  std::vector<double> A, B ;
  int i ;
  for (i = 0 ; i < n ; ++i)
  {
    int val = n - i ;
    //int val = ((n - i) * 9723) % 11 ;
    A.push_back(val) ;
    B.push_back(val) ;
  }

	/*std::sort(A.begin(), A.begin() + 10) ;
	std::sort(A.begin() + 10, A.begin() + 20) ;
	std::cout<<std::endl ;

	double buffer[20] ;
	PthreadSort<double>::Merge(&A[0], 0, 10, 10, 10, buffer, NULL) ;
	for (i = 0 ; i < 20 ; ++i)
		std::cout<<buffer[i]<<" " ;
	std::cout<<std::endl ;
	return 0 ;*/

  auto start = timer::now();
  std::sort(A.begin(), A.end()) ;
  auto end = timer::now() ;
  std::cout<<duration_cast<milliseconds>(end - start).count() << std::endl ;
  
	{
		start = timer::now();
		PthreadSort<double>::Sort(&B[0], n, 8) ; // Or S.data() for --std=c++11 
		end = timer::now() ;
		std::cout<<duration_cast<milliseconds>(end - start).count() << std::endl ;

		int mismatchCnt = 0 ;
		for (i = 0 ; i < n ; ++i)
		{
			if (A[i] != B[i])
				++mismatchCnt ;
		}
		std::cout<<"mismatch = " << mismatchCnt << std::endl ;
	}
	
	{
		for (i = 0 ; i < n ; ++i)
			B[i] = n - i ;
		omp_set_num_threads(8) ;
		start = timer::now();
		__gnu_parallel::sort(B.begin(), B.end()) ;
		end = timer::now() ;
		std::cout<<duration_cast<milliseconds>(end - start).count() << std::endl ;

		int mismatchCnt = 0 ;
		for (i = 0 ; i < n ; ++i)
		{
			if (A[i] != B[i])
				++mismatchCnt ;
		}
		std::cout<<"mismatch = " << mismatchCnt << std::endl ;
	}
  return 0 ;
}
