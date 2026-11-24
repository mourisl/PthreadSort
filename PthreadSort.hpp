#ifndef _MOURISL_PTHREAD_SORT
#define _MOURISL_PTHREAD_SORT

#include <algorithm>

#include <stdio.h>
#include <pthread.h>


template <class T>
class PthreadSort
{
	template <class T>
	struct _pthread_sort_arg
	{
		T *S ;
		size_t start, end ; // inclusive interval on S
		bool (*comp)(const T&, const T&) ;

		int threadCnt ;
	} ;

	template <class T>
	struct _pthread_merge_arg
	{
		T *S ;
		T *buffer ;

		size_t s1, e1, s2, e2 ; // inclusive interval on S
		bool (*comp)(const T&, const T&) ;

		int threadCnt ;
	} ;

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
		Merge(arg.S, arg.s1, arg.e1, arg.s2, arg.e2, arg.buffer + arg.s1, arg.comp) ;

		// copy back
		size_t i ;
		size_t totalCnt = arg.e1 - arg.s1 + 1 + arg.e2 - arg.s2 + 1 ;
		for (i = 0 ; i < totalCnt ; i++)
			arg.S[arg.s1 + i] = arg.buffer[arg.s1 + i] ;
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
	void Merge(T *S, size_t s1, size_t e1, size_t s2, size_t e2, T *target, bool (*comp)(const T&, const T&))
	{
		size_t i, j, k ;
		i = s1, j = s2 ;
		k = 0 ;
		while (i <= e1 && j <= s2)
		{
			if (APrecB(S[i], S[j], comp))
			{
				targe[k] = S[i] ;
				++i ;
			}
			else
			{
				target[k] = S[j] ;
				++j ;
			}
			++k ;
		}

		while (i <= e1)
		{
			target[k] = S[i] ;
			++i ;
			++k ;
		}

		while (j <= e2)
		{
			target[k] = S[j] ;
			++j ;
			++k ;
		}
	}

public:
  static void Sort(T *S, size_t n, int threadCnt, bool (*comp)(const T&, const T&) = NULL)
  {
    size_t i, j ;
    size_t step = n ;
    if (n < threadCnt || n < 10000 || threadCnt == 1)
    {
      StdSort(S, n, comp) ;
      return ;
    }
    
    pthread_t *threads = new pthread_t[threadCnt] ;
    pthread_attr_t attr ;

    pthread_attr_init( &attr ) ;
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) ;

		T *buffer = new T[n] ; // buffer for merging
		size_t *mergeCutRank = new size_t[threadCnt] ; 

    // Sort segment in parallel
		struct _pthread_sort_arg<T> *sortArgs = new struct _pthread_sort_arg<T>[threadCnt] ; 
		size_t remainder = n % threadCnt ;
		for (i = 0 ; i < threadCnt ; i++)
		{
			sortArgs[i].S = S ;
			if (i < remainder)
			{
				sortArgs[i].start = i * (n / threadCnt) + i ;
				sortArgs[i].end =  (i + 1) * (n / threadCnt) + i - 1 ;
			}
			else
			{
				sortArgs[i].start = i * (n / threadCnt) + remainder ;
				sortArgs.end =  (i + 1) * (n / threadCnt) + remainder - 1 ;
			}

			sortArgs[i].comp = comp ;
			sortArgs[i].threadCnt = threadCnt ;
		}

		for (i = 0 ; i < threadCnt ; ++i)
			pthread_create(&thread[i], &attr, Sort_Thread, (void *)(&sortArgs[i])) ;

		for (i = 0 ; i < threadCnt ; ++i)
			pthread_join(thread[i], NULL) ;

		// Merge the sorted segments
		struct _pthread_merge_arg<T> *mergeArgs = new struct _pthread_merge_arg<T>[threadCnt] ;
		size_t stride = 1 ; // the stride for the segment
		for (stride = 1 ; stride < threadCnt ; stride *= 2)
		{
			for (i = 0 ; i < threadCnt ; i += 2 * stride)
			{
				size_t s1 = sortArgs[i].start ;
				size_t e1 = sortArgs[i + stride - 1].end ;

				size_t s2 = sortArgs[i + stride].start ;
				if (i + 2 * stride - 1 < threadCnt)
					e2 = sortArgs[i + 2 * stride - 1].end ;
				else
					e2 = sortArgs[threadCnt - 1].end ;

				// Find the cutting point 
				size_t totalCnt = e1 - s1 + 1 + e2 - s2 + 1 ;
				size_t chunkSize = totalCnt / threadCnt ;
				for (j = 0 ; j < threadCnt - 1 ; j++)
				{
					size_t k = (j + 1) * chunkSize ;
					mergeCutRank[j] = CoRank(S, s1, e1, s2, e2, k, comp) ;
				}
				mergeCutRank[j] = e1 - s1 + 1 ; 

				for (j = 0 ; j < threadCnt ; j++)
				{
					mergeArgs[j].S = S ;
					if (j == 0)
						mergeArgs[j].s1 = s1 ; // Notice the s1,e1 in mergeArgs are subblocks in S[s1..e1] and S[s2..e2]
					else
						mergeArgs[j].s1 = s1 + mergeCutRank[j - 1] ;

					mergeArgs[j].e1 = s1 + mergeCutRank[j] - 1 ;

					if (j == 0)
						mergeArgs[j].s2 = s2 ;
					else
						mergeArgs[j].s2 = s2 + j * chunkSize - mergeCutRank[j - 1] ;

					if (j < threadCnt - 1)
						mergeArgs[j].e2 = s2 + (j + 1) * chunkSize - mergeCutRank[j] - 1 ;
					else
						mergeArgs[j].e2 = e2 ;

					mergeArgs[j].buffer = buffer ;
				
					mergeArgs[j].comp = comp ;
					mergeArgs[j].threadCnt = threadCnt ;
				}

				for (i = 0 ; i < threadCnt ; ++i)
					pthread_create(&thread[i], &attr, Merge_Thread, (void *)(&mergeArgs[i])) ;
				for (i = 0 ; i < threadCnt ; ++i)
					pthread_join(thread[i], NULL) ;
			}
		}

    pthread_attr_destroy(&attr) ;

		delete[] threads ;
		delete[] sortArgs ;
		delete[] mergeCutRank ;
		delete[] mergeArgs ;
  }
} ;

#endif
