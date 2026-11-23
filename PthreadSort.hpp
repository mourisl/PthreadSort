#ifndef _MOURISL_PTHREAD_SORT
#define _MOURISL_PTHREAD_SORT

#include <algorithm>

#include <stdio.h>
#include <pthread.h>

template <class T>
struct _pthread_sort_arg
{
  T *S ;
  size_t start, end ; // inclusive interval on S
  bool (*comp)(const T&, const T&) ;

  int threadCnt ;
} ;

template <class T>
class PthreadSort
{
private:
  static void StdSort(T *S, size_t n, bool (*comp)(const T&, const T&))
  {
    if (comp == NULL)
      std::sort(S, S + n) ;
    else
      std::sort(S, S + n, *comp) ;
  }

  // Sort a segment 
  static void *Sort_Thread(void *pArg)
  {
    struct _pthread_sort_arg<T> &arg = *((struct _pthread_sort_arg<T> *)pArg) ;
    size_t start = arg.start ;
    size_t end = arg.end ;
    //printf("region sort: %lu %lu. %lu\n", start, end, arg.S[10]);
    if (start < end)
      StdSort(arg.S + start, end - start + 1, arg.comp) ;
    //printf("region sort: %lu %lu. %lu\n", start, end, arg.S[10]);
    pthread_exit(NULL) ;
  }

	static void *Merge_Thread(void *pArg)
	{
		struct _pthread_sort_arg<T> &arg = *((struct _pthread_sort_arg<T> *)pArg) ;
		size_t start = arg.start ;
		size_t end = arg.end ;

		if (start < end)
		{
		}
	}

  static void Swap(T &a, T &b)
  {
    T tmp ;
    tmp = a ; a = b ; b = tmp ;
  }

public:
  static void Sort(T *S, size_t n, int threadCnt, bool (*comp)(const T&, const T&) = NULL)
  {
    size_t i ;
    size_t step = n ;
    if (n < threadCnt || threadCnt == 1)
    {
      StdSort(S, n, comp) ;
      return ;
    }
    
    pthread_t thread ;
    pthread_attr_t attr ;

    pthread_attr_init( &attr ) ;
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) ;

    // Sort segment in parallel
		struct _pthread_sort_arg<T> *sortArg = new struct _pthread_sort_arg<T> ; 
		size_t remainder = n % threadCnt ;
		for (i = 0 ; i < threadCnt ; i++)
		{
			sortArg.S = S ;
			if (i < remainder)
			{
				sortArg.start = i * (n / threadCnt) + i ;
				sortArg.end =  (i + 1) * (n / threadCnt) + i - 1 ;
			}
			else
			{
				sortArg.start = i * (n / threadCnt) + remainder ;
				sortArg.end =  (i + 1) * (n / threadCnt) + remainder - 1 ;
			}

			sortArg.comp = comp ;
			sortArg.threadCnt = threadCnt ;
		}

    pthread_create(&thread, &attr, Sort_Thread, (void *)(&sortArg)) ;
    pthread_join(thread, NULL) ;

		// Merge the sorted segments
		// Pick the split point for each segment
		
		// Parallel merge

    pthread_attr_destroy(&attr) ;

		delete sortArg ;
  }
} ;

#endif
