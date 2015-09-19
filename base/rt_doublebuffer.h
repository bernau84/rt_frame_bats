#ifndef RT_DOUBLEBUFFER_H
#define RT_DOUBLEBUFFER_H

#include "rt_multibuffer.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*! \brief base on rt_multibuffer
 * BUT reading functions return pointer (instead of value) to linear space of length N
 * real allocated size is 2*size
 */

template <typename T, int N> class t_doublebuffer : public virtual t_multibuffer<T, N> {

protected:
    /* typedef arrayListType<elemType> Parent; or this for non C++11
      * otherway use 'using' keyword */
     using t_multibuffer<T, N>::buf;
     using t_multibuffer<T, N>::size;
     using t_multibuffer<T, N>::wmark;
     using t_multibuffer<T, N>::overflow;
     using t_multibuffer<T, N>::rmark;

    public:
        virtual const T *write(const T *smp){  //add sample

            buf[wmark + size] = *smp;  //fill higher half
            return t_multibuffer<T, N>::write(smp);  //and lower + increment wr pointer
        }

        virtual const T *set(const T *smp){ //rewrite actual possition & mirror

            buf[wmark + size] = *smp;  //fill higher half
            return t_multibuffer<T, N>::set(smp);  //and lower
        }

        t_doublebuffer(int _size, const t_rt_lock &_lock):
            t_multibuffer<T, N>(2*_size, _lock){

            size = _size;  //workaround - mask higher half
        }

        virtual ~t_doublebuffer();
};

#endif //RT_DOUBLEBUFFER_H
