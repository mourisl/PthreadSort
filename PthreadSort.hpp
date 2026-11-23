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

	// Is a should precede b in the sorted order?
	// Use less than by default.
	static bool APrecB(const T &a, const T &b, bool (*comp)(const T&, const T&))
	{
		if (comp != NULL)
			return comp(a, b) ;
		else
			return a < b ;
	}

	// For the k-smallest element in S[s1..e1] and S[s2..e2], find the rank of k in S[s1..e1], e.g., number of elements in S[s1..e1] that are smaller than the k-smallest element in S[s1..e1] and S[s2..e2].
	size_t CoRank(T *S, size_t s1, size_t e1, size_t s2, size_t e2, size_t k, bool (*comp)(const T&, const T&))
	{
		size_t l, r, m ;
		l = s1, r = e1 ;
		if (e1 - s1 + 1 > k)
			r = s1 + k - 1 ;
		if (e2 - s2 + 1 < k)
			l = s1 + k - (e2 - s2 + 1) ;
		
		while (l <= r)
		{
			m = (l + r) / 2 ;

			if (APrecB(S[m], S[s2 + k - (m - s1) - 1], comp)) // The guess is too few.
				l = m + 1 ;
			else
				r = m - 1 ;
		}

		return m - s1 ; // (m-1)-s1+1
	}

	// Assume s1<s2. 
	// allow target to overlapping with S[s1, e1] and S[s2, e2]
	void Merge(T *S, size_t s1, size_t e1, size_t s2, size_t e2, T *target, bool (*comp)(const T&, const T&))
	{
		size_t i, j ;
		i = s1, j = s2 ;
		while (i <= e1 && j <= s2)
		{
			if (APrecB(S[i], S[j], comp))
				targe[] = S[i++] ;
			else
				*target++ = S[j++] ;
		}
	}

public:
  static void Sort(T *S, size_t n, int threadCnt, bool (*comp)(const T&, const T&) = NULL)
  {
    size_t i ;
    size_t step = n ;
    if (n < threadCnt || n < 10000 || threadCnt == 1)
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
