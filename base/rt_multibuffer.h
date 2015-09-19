#ifndef RT_MULTIBUFFER_H
#define RT_MULTIBUFFER_H

#define NO_AVAIL_CHECK

#include "rt_common.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*! \brief - reaw/write circular buffer with arbitrary size
 * support only one item IO access (can't read/write array at once)
 * overwrites data if no free space left
 * \note - use template feature to implement item as array
 * support overflow indicator, multi reader, one writer, locks */

template <typename T, int N> class t_multibuffer {

    protected:
        int     size;
        int     wmark;
        int     rmark[N];
        int     overflow[N];
        T       *buf;

        void __init(){

            buf = (T *) new T[size];

            wmark = 0;
            for(int i=0; i<N; i++){

                overflow[i] = -1;
                rmark[i] = 0;
            }
        }

        void __deinit(){

            delete[] buf;
        }


    public:
        virtual const T *write(const T *smp);  /*!< simple append (update write pointer), return 1 with success */
        virtual const T *set(const T *smp);  /*!< re/write item on write pointer without it shift, return 1 if item avaiable */

        const T *read(int n = 0);  /*!< simple read (update read pointer), return non-null with success */
        const T *get(int n = 0);  /*!< reads item on read pointer without it shift */

        int shift(int len, int n = 0);     /*!< shift read pointer / dummy read = reflect overflow flag */

        int readSpace(int n = 0);      /*!< how much items can be reads/avaiable items */
        int writeSpace(int n = 0);     /*!< how much items can be write to overwite read buffer n */
                                       /*! warning - it is not the free space because it can owerwrite another
                                        * read pointer and we can implement minimum fro all writeSpace because
                                        * we dont know which of N read pointers are used */

        bool isEmpty(int n = 0){

            return (((overflow[n % N] < 0) && (wmark == rmark[n % N])));
        }

        bool isOver(int n = 0){

            return (overflow[n % N] > 0) ? 1 : 0;
        }

        /*!< size is number of item, define mutex lock if concuret writing
         * or concuret reading from one index can occure */
        t_multibuffer(int _size)
            :size(_size){

            __init();
        }

        virtual ~t_multibuffer(){

            __deinit();
        }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template <typename T, int N>  const T *t_multibuffer<T, N>::get(int n){

    int nn = n % N;

#ifndef NO_AVAIL_CHECK
    if(readSpace(nn) <= 0)
        return NULL;
#endif //NO_AVAIL_CHECK

    return &buf[rmark[nn]];      //ok
}

//------------------------------------------------------------------------------
template <typename T, int N> const T *t_multibuffer<T, N>::set(const T *smp){

#ifndef NO_AVAIL_CHECK
    if((smp == NULL) /* || (writeSpace(n) < 0)*/)
        return NULL;
#endif //NO_AVAIL_CHECK

    buf[wmark] = *smp;
    return &buf[wmark];
}


//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::shift(int len, int n){

    int nn = n % N;

    if((rmark[nn] + len) >= size) rmark[nn] -= size;
        else rmark[nn] += len;
    overflow[nn] = -1;

    return overflow[nn];
}

//------------------------------------------------------------------------------
template <typename T, int N> const T *t_multibuffer<T, N>::write(const T *smp){

    if(smp) buf[wmark] = *smp;
    const T *ret = &buf[wmark];

    //check than modify - prevent mark to point out form buffer
    //can be problem at multithread, we dont have to use lock (for simo system)
    if(wmark >= (size-1)) wmark = 0;
        else wmark++;

    for(int nn=0; nn < N; nn++)  //testujem pro vsechny cteci pointery!
        if((overflow[nn] > 0) || (wmark == rmark[nn]))  //musime testovat znovu po updatu
            overflow[nn] = (overflow[nn]+1) % size; //vetsi preteceni nez size neindikujem, po prvnim srovnani je overflow 0 == FULL

    return ret;
}

//------------------------------------------------------------------------------
template <typename T, int N> const T *t_multibuffer<T, N>::read(int n){

    int nn = n % N;
    const T *smp = NULL;

    if((wmark != rmark[nn]) || (overflow[nn] >= 0)){

        smp = &buf[rmark[nn]];
        if(rmark[nn] >= (size-1)) rmark[nn] = 0;
            else rmark[nn]++;

        overflow[nn] = -1; //OK
    }

    return smp;
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::readSpace(int n){

    int nn = n % N;
    int tmp = (int)(wmark - rmark[nn]);
    if( ((tmp == 0)&&(overflow[nn] >= 0)) || (tmp < 0) ) tmp += size;   //volny prostor je prelozeny, nebo doslo k preteceni
    return(tmp);   //prostor nebyl prelozeny (rd muze != wr i pri overflow)
}

//------------------------------------------------------------------------------
//vycitani volneho mista z kruh bufferu ktery vyuziva cely prostor
template <typename T, int N> int t_multibuffer<T, N>::writeSpace(int n){

    int nn = n % N;
    int tmp = (int)(rmark[nn] - wmark);
    if(overflow[nn] < 0){

      if(tmp < 0) tmp += size;
    } else
      tmp = 0;

    return tmp; //mam tu preteceni - nelze zapsat nic; az dokud si app overflow neshodi
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#endif // RT_MULTIBUFFER_H
