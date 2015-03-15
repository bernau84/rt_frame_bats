#ifndef RT_CIRCULARBUFFER
#define RT_CIRCULARBUFFER

#ifndef RT_MULTIBUFFER_H
#define RT_MULTIBUFFER_H

#define NO_AVAIL_CHECK

#include "rt_multibuffer.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*! \brief - reaw/write circular buffer, support also simo acces as rt_multibuffer
 *  with arbitrary acces via from additional parameter overrides the current rd mark
 *  rmark lets been used for internal reader , from for external
*/

template <typename T, int N> class t_circularbuffer {

    private:
        t_rt_lock lock;

    protected:
        int     size;
        int     wmark;
        int     rmark;
        int     overflow;
        T       *buf;

    public:
        int write(T &smp);  /*!< simple append (update write pointer), return 1 with success */
        int read(T &smp, int &from);  /*!< simple read (update read pointer), return 1 with success */
        int read(T &smp){ return read(smp, rmark); }

        T get(int &from);  /*!< reads item on read pointer without it shift */
        T get(){ return get(rmark); }

        int set(T &smp);  /*!< re/write item on write pointer without it shift, return 1 if item avaiable */

        int shift(int len, int &from);     /*!< shift read pointer / dummy read = reflect overflow flag */
        int shift(int len){ return shift(len, rmark); }

        int readSpace(int &from);      /*!< how much items can be reads/avaiable items */
        int readSpace(){ return readSpace(rmark); }

        int writeSpace(int &from);     /*!< how much items can be write to overwite read buffer n */
        int writeSpace(){ return writeSpace(rmark); }

        bool isEmpty(){

            return (((overflow < 0) && (wmark == rmark)));
        }

        bool isOver(){

            return (overflow > 0) ? 1 : 0;
        }

        t_multibuffer(int _size, t_rt_lock &_lock)  /*!< size is number of item, can work directly on p if int NULL */
            :size(_size), lock(_lock){

            buf = (T *) new T[size];
            wmark = 0;
            overflow = -1;
            rmark = 0;
        }

        virtual ~t_circularbuffer(){

            if(buf) delete[] buf;
        }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template <typename T> T t_circularbuffer<T>::get(int &from){

    from %= size;

#ifndef NO_AVAIL_CHECK
    if(readSpace(from) <= 0)
        return T();
#endif //NO_AVAIL_CHECK

    lock.lockRead();
    T ret = buf[from];
    lock.unlock();
    return ret;      //ok
}

//------------------------------------------------------------------------------
template <typename T> int t_circularbuffer<T>::set(T &smp){

#ifndef NO_AVAIL_CHECK
    if((smp == NULL)  || (writeSpace() < 0))
        return 0;
#endif //NO_AVAIL_CHECK

    lock.lockWrite();
    buf[wmark] = smp;
    lock.unlock();
    return 1;
}


//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::shift(int len, int n){

    int nn = n % N;

    lock.lockRead();
    if((rmark[nn] += len) >= size) rmark[nn] -= size;
    overflow[nn] = -1;
    lock.unlock();

    return overflow[nn];
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::write(T &smp){

    lock.lockWrite();

    buf[wmark++] = smp;
    if(wmark >= size) wmark = 0;

    for(int nn=0; nn < N; nn++)  //testujem pro vsechny cteci pointery!
        if((overflow[nn] > 0) || (wmark == rmark[nn]))  //musime testovat znovu po updatu
            overflow[nn] = (overflow[nn]+1) % size; //vetsi preteceni nez size neindikujem, po prvnim srovnani je overflow 0 == FULL

    lock.unlock();

    return 0;
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::read(T &smp, int n){

    int nn = n % N;
    int ret = 0;

    lock.lockRead();
    if((wmark != rmark[nn]) || (overflow[nn] >= 0)){

        smp = buf[rmark[nn]++];
        if(rmark[nn] >= size) rmark[nn] = 0;

        overflow[nn] = -1; //OK
        ret = 1;
    }
    lock.unlock();

    return ret;
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::readSpace(int n){

    lock.lockWrite();

    int nn = n % N;
    int tmp = (int)(wmark - rmark[nn]);
    if( ((tmp == 0)&&(overflow[nn] >= 0)) || (tmp < 0) ) tmp += size;   //volny prostor je prelozeny, nebo doslo k preteceni
    lock.unlock();

    return(tmp);   //prostor nebyl prelozeny (rd muze != wr i pri overflow)
}

//------------------------------------------------------------------------------
//vycitani volneho mista z kruh bufferu ktery vyuziva cely prostor
template <typename T, int N> int t_multibuffer<T, N>::writeSpace(int n){

    lock.lockWrite();

    int nn = n % N;
    int tmp = (int)(rmark[nn] - wmark);
    if(overflow[nn] < 0){

      if(tmp < 0) tmp += size;
    } else
      tmp = 0;

    lock.unlock();
    return tmp; //mam tu preteceni - nelze zapsat nic; az dokud si app overflow neshodi
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#endif // RT_MULTIBUFFER_H

int read_from(int &rmark, T &smp);  /*!< \todo - simple read from given possition with rmark incrementation */

#endif // RT_CIRCULARBUFFER

