#ifndef _MOURISL_PTHREAD_SORT
#define _MOURISL_PTHREAD_SORT

#include <algorithm>

#include <stdio.h>
#include <pthread.h>

template <class T>
struct _pthread_sort_arg
{
  T *S ;
  size_t start, size_t end ;
  bool (*comp)(const T&, const T&) ;
} ;

template <class T>
struct _pthread_merge_arg
{
  T *S ;
  size_t start, size_t mid, size_t end ;
  bool (*comp)(const T&, const T&) ;
} ;


template <class T>
class PthreadSort
{
private:
  static void *Sort_Thread(void *pArg)
  {
    struct _pthread_sort_arg<T> &arg = *((struct _pthread_sort_arg *)pArg) ;
    std::sort(arg.S + start, args.S + end + 1, comp) ;
    pthread_exit(NULL) ;
  }

  static void StdSort(T *S, size_t n, bool (*comp)(const T&, const T&))
  {
    if (comp == NULL)
      std::sort(S, S + n) ;
    else
      std::sort(S, S + n, *comp) ;
  }

  // Merge the [start, mid], (mid, end] to be sorted [start, end]
  static void *Merge_Thread(void *pArg)
  {
    struct _pthread_merge_arg<T> &arg = *((struct _pthread_merge_arg *)pArg) ;
    size_t i, j, k ;
    
    i = arg.start ;
    j = arg.mid + 1 ;

    while (i <= mid && j <= arg.end)
    {
      if ((arg.comp ? (*(arg.comp))(S[i], S[j]) : (S[i] < S[j])))
      {

      }
    }

  }
public:
  static void Sort(T *S, size_t n, int threadCnt, bool (*comp)(const T&, const T&))
  {
    size_t i, j ;
    size_t step = n ;
    if (n < threadCnt)
    {
      StdSort(S, n, comp) ;
      return ;
    }
    
    std::vector<std::pair<size_t, size_t> > segs ;
    int segCnt ;
    size_t block = n / threadCnt ;
    for (i = 0 ; i < threadCnt ; ++i)
    {
      size_t start = i * block ;
      size_t end = i * block + block - 1 ;
      if (end == threadCnt - 1)
        end = n - 1 ;

      std::pair<size_t, size_t> ns ;
      ns.first = start ;
      ns.second = end ;
      segs.push_back(ns) ;
    }

    segCnt = segs.size() ;

    // Sort the leaf segments
    pthread_t *threads = new pthread_t[ threadCnt ] ;
    struct _pthread_sort_arg<T> *sortArgs  = new struct _pthread_sort_arg<T>[threadCnt] ;
    pthread_attr_t attr ;

    pthread_attr_init( &attr ) ;
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) ;

    for (i = 0 ; i < threadCnt ; ++i)
    {
      sortArgs[i].S = S ;
      sortArgs[i].start = segs[i].first ;
      sortArgs[i].end = segs[i].end ;
      sortArgs[i].comp = comp ;

      pthread_create(&threads[i], &attr, Sort_Thread, (void *)(sortArgs + i)) ;
    }

    for (i = 0 ; i < threadCnt ; ++i)
      pthread_join(threads[i], NULL) ;
    delete[] sortArgs ;

    // Merge the segments
    while (segCnt > 1)
    {
      int usedThread = 0 ;
    }

    delete[] threads ;
    pthread_attr_destroy(&attr) ;
  }
} ;

#endif
