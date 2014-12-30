#ifndef RT_MULTIBUFFER_H
#define RT_MULTIBUFFER_H

#define NO_AVAIL_CHECK

/*! \brief - interface definition for lock - depend on concrete implementation
 * \note - QReadWriteLock can be use for Qt, Mutex in C03 standard */

class rt_lock {

private:
    virtual lockRead(){;}
    virtual lockWrite(){;}
    virtual unlock(){;}

    rt_lock(){;}
    ~rt_lock(){;}
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*! \brief - reaw/write circular buffer with arbitrary size
 * support only one item IO access (can't read/write array at once)
 * overwrites data if no free space left
 * \note - use template feature to implement item as array
 * support overflow indicator, multi reader, one writer, locks */

template <typename T, int N> class t_multibuffer {

    private:
        rt_lock lock;

    protected:
        int     size;
        int     wmark;
        int     rmark[N];
        int     overflow[N];
        T       *buf;

    public:
        int write(T &smp);  /*!< simple append (update write pointer), return 1 with success */
        int read(T &smp, int n = 0);  /*!< simple read (update read pointer), return 1 with success */

        T get(int n = 0);  /*!< reads item on read pointer without it shift, return 1 if item avaiable */
        int set(T &smp);  /*!< re/write item on write pointer without it shift, return 1 if item avaiable */

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

        t_multibuffer(int _size, const rt_lock &_lock)  /*!< size is number of item, can work directly on p if int NULL */
            :size(_size), lock(_lock){

            buf = (T *) new T[size];

            wmark = 0;
            for(int i=0; i<N; i++){

                overflow[i] = -1;
                rmark[i] = 0;
            }
        }

        virtual ~t_multibuffer(){

            if(buf)
                delete[] buf;
        }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template <typename T, int N> T t_multibuffer<T, N>::get(int n){

    int nn = n % N;

#ifndef NO_AVAIL_CHECK
    if(readSpace(nn) <= 0)
        return T();
#endif //NO_AVAIL_CHECK

    lock.lockRead();
    T ret = buf[rmark[nn]];
    lock.unlock();
    return ret;      //ok
}

//------------------------------------------------------------------------------
template <typename T, int N> int t_multibuffer<T, N>::set(T &smp){

#ifndef NO_AVAIL_CHECK
    if((smp == NULL) /* || (writeSpace(n) < 0)*/)
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

    lock.lockForWrite();

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
