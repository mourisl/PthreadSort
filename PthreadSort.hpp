#ifndef _MOURISL_PTHREAD_SORT
#define _MOURISL_PTHREAD_SORT

#include <algorithm>

#include <stdio.h>
#include <pthread.h>

template <class T>
class PthreadSort
{
	struct _pthread_sort_arg
	{
		T *S ;
		size_t start, end ; // inclusive interval on S
		bool (*comp)(const T&, const T&) ;

		int threadCnt ;
	} ;

	struct _pthread_merge_arg
	{
		T *S ;
		T *buffer ;

		size_t s1, n1, s2, n2 ; // intervals like [s1..s1+n1-1] and [s2..s2+n2-1]
														// use n1 to handle empty intervals
		size_t offset ; // the offset in S for the merged result
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
    struct _pthread_sort_arg &arg = *((struct _pthread_sort_arg *)pArg) ;
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
		struct _pthread_merge_arg &arg = *((struct _pthread_merge_arg *)pArg) ;
		Merge(arg.S, arg.s1, arg.n1, arg.s2, arg.n2, arg.buffer, arg.comp) ;
	
		pthread_exit(NULL) ;
	}

	static void *Copy_Thread(void *pArg)
	{
		struct _pthread_merge_arg &arg = *((struct _pthread_merge_arg *)pArg) ;
		// copy back
		size_t i ;
		for (i = arg.s1 ; i < arg.s1 + arg.n1 ; ++i)
			arg.S[i] = arg.buffer[i] ;
		pthread_exit(NULL) ;
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
	// k is 1-based, rank is also 1-based
	static size_t CoRank(T *S, size_t s1, size_t e1, size_t s2, size_t e2, size_t k, bool (*comp)(const T&, const T&))
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

			// We select S[s1..m] and the remaining elements from S[s2..m'], m'=s2+k-(m-s1+1)-1. If s[m]>=s[m'+1], then we guess too many elements from S[s1..e1], otherwise we guess too few elements.
			if (APrecB(S[s2 + k - (m - s1 + 1)], S[m], comp)) // The guess is too much. The order in the argument list makes sure we prefer elements from s1..e1
			{
				if (m > 0)
					r = m - 1 ;
				else
					break ;
			}
			else
				l = m + 1 ;
		}
		return l - s1 ; // (m-1)-s1+1
	}

public:
	// Assume s1<s2. 
	static void Merge(T *S, size_t s1, size_t n1, size_t s2, size_t n2, T *target, bool (*comp)(const T&, const T&))
	{
		size_t i, j, k ;
		i = 0, j = 0 ;
		k = 0 ;
		while (i < n1 && j < n2)
		{
			if (APrecB(S[s2 + j], S[s1 + i], comp))
			{
				target[k] = S[s2+j] ;
				++j ;
			}
			else
			{
				target[k] = S[s1+i] ;
				++i ;
			}
			++k ;
		}
		
		while (i < n1)
		{
			target[k] = S[s1+i] ;
			++i ;
			++k ;
		}

		while (j < n2)
		{
			target[k] = S[s2+j] ;
			++j ;
			++k ;
		}
	}

public:
	static bool IsSorted(T *S, size_t n, bool (*comp)(const T&, const T&) = NULL)
	{
		size_t i ;
		for (i = 1 ; i < n ; i++)
		{
			if (APrecB(S[i], S[i - 1], comp))
			{
				printf("IsSorted: %lu %lu. %lf %lf\n", i, n, S[i], S[i-1]) ;
				return false ;
			}
		}
		return true ;
	}

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
		struct _pthread_sort_arg *sortArgs = new struct _pthread_sort_arg[threadCnt] ; 
		size_t remainder = n % threadCnt ;
		for (i = 0 ; i < threadCnt ; i++)
		{
			sortArgs[i].S = S ;
			if (i < remainder)
			{
				sortArgs[i].start = i * (n / threadCnt) + i ;
				sortArgs[i].end =  (i + 1) * (n / threadCnt) + i ;
			}
			else
			{
				sortArgs[i].start = i * (n / threadCnt) + remainder ;
				sortArgs[i].end =  (i + 1) * (n / threadCnt) + remainder - 1 ;
			}

			sortArgs[i].comp = comp ;
			sortArgs[i].threadCnt = threadCnt ;
		}

		for (i = 0 ; i < threadCnt ; ++i)
			pthread_create(&threads[i], &attr, Sort_Thread, (void *)(&sortArgs[i])) ;

		for (i = 0 ; i < threadCnt ; ++i)
			pthread_join(threads[i], NULL) ;

		// Merge the sorted segments
		struct _pthread_merge_arg *mergeArgs = new struct _pthread_merge_arg[threadCnt] ;
		size_t stride = 1 ; // the stride for the segment
		for (stride = 1 ; stride < threadCnt ; stride *= 2)
		{
			for (i = 0 ; i + stride < threadCnt ; i += 2 * stride)
			{
				size_t s1 = sortArgs[i].start ;
				size_t e1 = sortArgs[i + stride - 1].end ;
				
				size_t s2 = sortArgs[i + stride].start ;
				size_t e2 = 0 ;
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
					{
						mergeArgs[j].s1 = s1 ; // Notice the s1,e1 in mergeArgs are subblocks in S[s1..e1] and S[s2..e2]
						mergeArgs[j].n1 = mergeCutRank[0] ;
					}
					else
					{
						mergeArgs[j].s1 = s1 + mergeCutRank[j - 1] ;
						mergeArgs[j].n1 = mergeCutRank[j] - mergeCutRank[j - 1] ;
					}

					if (j == 0)
						mergeArgs[j].s2 = s2 ;
					else
						mergeArgs[j].s2 = s2 + j * chunkSize - mergeCutRank[j - 1] ;

					if (j == 0)
						mergeArgs[j].n2 = chunkSize - mergeCutRank[0] ;
					else if (j < threadCnt - 1)
						mergeArgs[j].n2 = chunkSize - mergeCutRank[j] + mergeCutRank[j - 1] ;
					else
						mergeArgs[j].n2 = e2 - mergeArgs[j].s2 + 1 ;

					mergeArgs[j].offset = s1 + j * chunkSize ;
					mergeArgs[j].buffer = buffer + s1 + j * chunkSize ;
				
					mergeArgs[j].comp = comp ;
					mergeArgs[j].threadCnt = threadCnt ;
					//printf("merge: %lu %lu %lu %lu. %lu. %lf %lf\n", mergeArgs[j].s1, mergeArgs[j].n1, mergeArgs[j].s2, mergeArgs[j].n2, s1 + j * chunkSize, S[mergeArgs[j].s1], S[mergeArgs[j].s2]) ;
				}

				for (j = 0 ; j < threadCnt ; ++j)
					pthread_create(&threads[j], &attr, Merge_Thread, (void *)(&mergeArgs[j])) ;

				for (j = 0 ; j < threadCnt ; ++j)
					pthread_join(threads[j], NULL) ;
			}

			if (stride == 1 && i < threadCnt)
			{
				// Copy the last segment to the buffer, so buffer and main output array can be consistent
				// i+1 is beyond threadCnt
				for (j = sortArgs[i].start ; j <= sortArgs[i].end ; ++j)
					buffer[j] = S[j] ;
			}
		
			// Copy back the merged result to S
			for (j = 0 ; j < threadCnt ; ++j)
			{
				mergeArgs[j].s1 = (n / threadCnt) * j ;
				mergeArgs[j].n1 = (j == threadCnt - 1) ? (n - (n/threadCnt)*j) : n/threadCnt ;
				mergeArgs[j].buffer = buffer ;
			}

			for (j = 0 ; j < threadCnt ; ++j)
				pthread_create(&threads[j], &attr, Copy_Thread, (void *)(&mergeArgs[j])) ;

			for (j = 0 ; j < threadCnt ; ++j)
				pthread_join(threads[j], NULL) ;
		}

		pthread_attr_destroy(&attr) ;

		delete[] threads ;
		delete[] sortArgs ;
		delete[] buffer ;
		delete[] mergeCutRank ;
		delete[] mergeArgs ;
  }
} ;

#endif
