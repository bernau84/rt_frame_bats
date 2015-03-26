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

    public:
        virtual int write(const T *smp){  //add sample

            buf[wmark + size] = *smp;  //fill higher half
            return t_multibuffer::write(smp);  //and lower + increment wr pointer
        }

        virtual int set(const T *smp){ //rewrite actual possition & mirror

            buf[wmark + size] = *smp;  //fill higher half
            return t_multibuffer::set(smp);  //and lower
        }

        t_doublebuffer(int _size, const t_rt_lock &_lock):
            t_multibuffer(2*_size, _lock){

            size = _size;  //workaround - mask higher half
        }

        virtual ~t_doublebuffer();
};

