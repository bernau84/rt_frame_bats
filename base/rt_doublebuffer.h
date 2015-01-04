#ifndef RT_DOUBLEBUFFER_H
#define RT_DOUBLEBUFFER_H

#include "rt_multibuffer.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/*! \brief base on rt_multibuffer
 * BUT reading functions return pointer (instead of value) to linear space of length N
 * real allocated size is 2*size
 */

template <typename T, int N> class t_doublebuffer : private t_multibuffer<N, T> {

    public:
        int write(T smp){  //add sample

            buf[wmark + size] = smp;  //fill higher half
            return t_multibuffer::write(smp);  //and lower + increment wr pointer
        }

        const T *read(int n = 0){  //access to samples for reading

            T dummy;
            const T *ret = &buf[rmark];  //prepare return pointer
            return (t_multibuffer::read(dummy)) ? ret : NULL;  //if data avaiable return pointer
        }

        int set(T smp){ //rewrite actual possition & mirror

            buf[wmark + size] = smp;  //fill higher half
            return t_multibuffer::set(smp);  //and lower
        }

        const T *get(int n = 0){

            return &buf[rmark];
        }

        int shift(int len, int n = 0){ return t_multibuffer::shift(len, n); }

        int writeSpace(int n = 0){ return t_multibuffer::writeSpace(n); }
        int readSpace(int n = 0){ return t_multibuffer::readSpace(n); }

        bool    isEmpty(int n = 0){ return t_multibuffer::isEmpty(n); }
        bool    isOver(int n = 0){ return t_multibuffer::isOver(n); }

        t_doublebuffer(int _size, const t_rt_lock &_lock):
            t_multibuffer(2*_size, _lock){

            size = _size;  //workaround - mask higher half
        }

        virtual ~t_doublebuffer();
};

