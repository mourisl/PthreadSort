#include <stdio.h>
#include <vector>
#include <algorithm>
//#include "PthreadSort.hpp"

int main()
{
  int n = 10000000 ;
  std::vector<int> S ;
  int i ;
  for (i = 0 ; i < n ; ++i)
    S.push_back(n - i) ;
  std::sort(S.begin(), S.end()) ;
  return 0 ;
}
