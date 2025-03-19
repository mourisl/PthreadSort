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

  int depth ;
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
    printf("region sort: %lu %lu. %lu\n", start, end, arg.S[10]);
    if (start < end)
      StdSort(arg.S + start, end - start + 1, arg.comp) ;
    //printf("region sort: %lu %lu. %lu\n", start, end, arg.S[10]);
    pthread_exit(NULL) ;
  }

  static void Swap(T &a, T &b)
  {
    T tmp ;
    tmp = a ; a = b ; b = tmp ;
  }

  // Merge the [start, mid], (mid, end] to be sorted [start, end]
  static void *QSort_Thread(void *pArg)
  {
    int i ;
    struct _pthread_sort_arg<T> &arg = *((struct _pthread_sort_arg<T> *)pArg) ;
    
    if (arg.start >= arg.end)
      pthread_exit(NULL) ;
    size_t s = arg.start ;
    T *S = arg.S ;
    
    Swap(S[s], S[(arg.start + arg.end)/2]) ;

    size_t pi = arg.start + 1 ; // pi points to the current process element
    size_t pj = arg.end + 1 ; // pj points the first element of the second chunk

    while (pi < pj)
    {
      bool lessThanPivot = (arg.comp ? (*(arg.comp))(S[pi], S[s]) : (S[pi] < S[s])) ;
      if (lessThanPivot)
        ++pi ;
      else 
      {
        Swap(S[pi], S[pj - 1]) ;
        --pj ;
      }
    }
    Swap(S[s], S[pi-1]) ;
    
    pthread_t threads[2] ;
    struct _pthread_sort_arg<T> sortArgs[2] ;
    pthread_attr_t attr ;

    pthread_attr_init( &attr ) ;
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) ;
  
    for (i = 0 ; i < 2 ; ++i)
    {
      sortArgs[i].S = arg.S ;
      sortArgs[i].comp = arg.comp ;
      sortArgs[i].depth = arg.depth + 1 ;
      sortArgs[i].threadCnt = arg.threadCnt ;
      if (i == 0)
      {
        sortArgs[i].start = arg.start ;
        if (pi >= 2)
          sortArgs[i].end = pi - 2 ;
        else
          sortArgs[i].end = arg.start ;
      }
      else
      {
        sortArgs[i].start = pi ;
        sortArgs[i].end = arg.end ;
      }
    }

    if ((1 << arg.depth) * 4 > arg.threadCnt)
    {
      pthread_create(&threads[0], &attr, Sort_Thread, (void *)(sortArgs + 0)) ;
      pthread_create(&threads[1], &attr, Sort_Thread, (void *)(sortArgs + 1)) ;
    }
    else
    {
      pthread_create(&threads[0], &attr, QSort_Thread, (void *)(sortArgs + 0)) ;
      pthread_create(&threads[1], &attr, QSort_Thread, (void *)(sortArgs + 1)) ;
    }

    for (i = 0 ; i < 2 ; ++i)
      pthread_join(threads[i], NULL) ;
    pthread_attr_destroy( &attr ) ;
    pthread_exit(NULL) ;
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
    struct _pthread_sort_arg<T> sortArg ; 
    pthread_attr_t attr ;

    pthread_attr_init( &attr ) ;
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) ;

    sortArg.S = S ;
    sortArg.start = 0 ;
    sortArg.end = n - 1 ;
    sortArg.comp = comp ;
    sortArg.depth = 0 ;
    sortArg.threadCnt = threadCnt ;

    pthread_create(&thread, &attr, QSort_Thread, (void *)(&sortArg)) ;
    pthread_join(thread, NULL) ;

    pthread_attr_destroy(&attr) ;
  }
} ;

#endif
