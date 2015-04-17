#ifndef RT_DATAFLOW_H
#define RT_DATAFLOW_H

#include "rt_common.h"
#include "rt_multibuffer.h"
#include "rt_doublebuffer.h"
#include "rt_datatypes.h"


/*! \def max number of subsribers for simo datatflow buffer
 */
#define RT_MAX_READERS   6

/*! \class i_rt_dataflow
 * \brief interface for all data storages for use in rt_base
 */

class i_rt_dataflow_input
{
protected:
    //for internal working object usage
    virtual int write(const void *smp) = 0;
    virtual int set(const void *smp) = 0;
    virtual int writeSpace() = 0;
};

class i_rt_dataflow_output
{
public:
    //extern obejct can read too (but cannot change)
    virtual const void *read(int n = 0) = 0;
    virtual const void *get(int n = 0) = 0;
    virtual int readSpace(int n = 0) = 0;
};

class i_rt_dataflow : public virtual i_rt_dataflow_input, public virtual i_rt_dataflow_output
{
public:
    i_rt_dataflow(){;}
    virtual ~i_rt_dataflow(){;}
};


/*! \brief - encapsulation for multibuffer fixed number of readers
 * typedef is unusable in this case
 */
template <class T> class rt_idf_circ_simo : public virtual i_rt_dataflow,
             private t_multibuffer<T, RT_MAX_READERS>
{
protected:
    /* typedef arrayListType<elemType> Parent; or this for non C++11
      * otherway use 'using' keyword */
     using t_multibuffer<T, RT_MAX_READERS>::buf;
     using t_multibuffer<T, RT_MAX_READERS>::size;
     using t_multibuffer<T, RT_MAX_READERS>::wmark;
     using t_multibuffer<T, RT_MAX_READERS>::overflow;
     using t_multibuffer<T, RT_MAX_READERS>::rmark;

public:
    //for internal working object usage
    virtual int write(const void *smp){ return write((T *)smp); }
    virtual int set(const void *smp){ return set((T *)smp); }
    virtual int writeSpace(){ return writeSpace(); }

    //extern object can read too (but cannot change)
    virtual const void *read(int n = 0){ return((const void *)read(n)); }
    virtual const void *get(int n = 0){ return ((const void *)get(n)); }
    virtual int readSpace(int n = 0){ return readSpace(n); }

    /*! \brief resize & reset */
    virtual void resize(int _size){

        //keeping data is not possible cause possition of rd/wr
        //pointers is unpredictible
        if(buf) delete[] buf;

        buf = new T[size = _size];

        wmark = 0;
        for(int i=0; i<RT_MAX_READERS; i++){

            overflow[i] = -1;
            rmark[i] = 0;
        }
    }

    rt_idf_circ_simo(int _size):
        i_rt_dataflow(),
        t_multibuffer<T, RT_MAX_READERS>(_size)
    {
    }

    virtual ~rt_idf_circ_simo(){;}
};


/*! \brief tempate class of data storage suitable for rt_base
 * circular, single input multi output (multireader), double buffer wit no need for warapped copy on read acceess
 */

template <class T> class rt_idf_circ2buf_simo : public virtual i_rt_dataflow,
                                        t_doublebuffer<T, RT_MAX_READERS>
{
private:
    /* typedef arrayListType<elemType> Parent; or this for non C++11
      * otherway use 'using' keyword */
     using t_multibuffer<T, RT_MAX_READERS>::buf;
     using t_multibuffer<T, RT_MAX_READERS>::size;
     using t_multibuffer<T, RT_MAX_READERS>::wmark;
     using t_multibuffer<T, RT_MAX_READERS>::overflow;
     using t_multibuffer<T, RT_MAX_READERS>::rmark;

protected:
    //for internal working object usage
    virtual int write(const void *smp){ return write((T *)smp); }
    virtual int set(const void *smp){ return set((T *)smp); }
    virtual int writeSpace(){ return writeSpace(0); }

public:
    //extern obejct can read too (but cannot change)
    virtual const void *read(int n = 0){return read(n); }
    virtual const void *get(int n = 0){return get(n); }
    virtual int readSpace(int n = 0){return readSpace(n); }

    /*! \brief resize & reset */
    virtual void resize(int _size){

        //keeping data is not possible cause possition of rd/wr
        //pointers is unpredictible
        if(buf) delete[] buf;

        buf = new T[size = 2*_size];

        wmark = 0;
        for(int i=0; i<RT_MAX_READERS; i++){

            overflow[i] = -1;
            rmark[i] = 0;
        }
    }

    rt_idf_circ2buf_simo(int _size):
        i_rt_dataflow(),
        t_doublebuffer<T, RT_MAX_READERS>(_size)
    {
    }

    virtual ~rt_idf_circ2buf_simo(){;}
};



#endif // RT_DATAFLOW_H
