#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

#include "PthreadSort.hpp"

//g++ -std=c++11 -lpthread example.cpp

using namespace std::chrono ;
using timer = std::chrono::high_resolution_clock;

int main()
{
  int n = 500000000 ;
  std::vector<int> A, B ;
  int i ;
  for (i = 0 ; i < n ; ++i)
  {
    int val = n - i ;
    A.push_back(val) ;
    B.push_back(val) ;
  }
  
  auto start = timer::now();
  std::sort(A.begin(), A.end()) ;
  auto end = timer::now() ;
  std::cout<<duration_cast<milliseconds>(end - start).count() << std::endl ;
  
  start = timer::now();
  PthreadSort<int>::Sort(&B[0], n, 8) ; // Or S.data() for --std=c++11 
  end = timer::now() ;
  std::cout<<duration_cast<milliseconds>(end - start).count() << std::endl ;

  int mismatchCnt = 0 ;
  for (i = 0 ; i < n ; ++i)
  {
    if (A[i] != B[i])
      ++mismatchCnt ;
  }
  std::cout<<"mismatch = " << mismatchCnt << std::endl ;
  return 0 ;
}
