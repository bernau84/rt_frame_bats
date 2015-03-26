#ifndef RT_DATAFLOW_H
#define RT_DATAFLOW_H

#include <QVector>
#include <QReadWriteLock>
#include "rt_multibuffer.h"
#include "rt_doublebuffer.h"

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

class i_rt_dataflow : public virtual i_rt_dataflow_input, i_rt_dataflow_output
{
    rt_dataflow(){;}
    virtual ~rt_dataflow(){;}
};

/*! \brief amplitude-frequency row/slice for rt buffer
 * 2D extension for simple rt_multibuffer
 */
template <class T> class t_rt_slice {

private:
    int irecent;  //last write index - -1 unitialized, 0 - first valid

public:

    T          t;  //time mark of this slice
    std::vector<T> A;  //amplitude
    std::vector<T> I;  //index / frequency

    //some useful operators on <A, I> pair
    typedef struct {

        T A;
        T I;
    } t_rt_ai;

    /*! \brief - test */
    bool isempty(){

       return (irecent < A.size()) ? true : false;
    }
    /*! \brief - reading from index */
    const t_rt_ai read(int i){

       i %= A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - read last written */
    const t_rt_ai last(){

       if(irecent < 0) irecent = 0;  //special case - not inited
       int i = irecent % A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - set last written */
    void set(const t_rt_ai &v){

        if(irecent < 0) irecent = 0; //special case - not inited
        A[irecent] = v.A;
        I[irecent] = v.I;
    }
    /*! \brief - writing, returns number of remaining positions */
    int append(const t_rt_ai &v){

       if(++irecent < A.size()){

           A[irecent] = v.A;
           I[irecent] = v.I;
       }

       return (A.size() - irecent);
    }

    t_rt_slice &operator= (const t_rt_slice &d){

        A = std::vector<T>(d.A);
        I = std::vector<T>(d.I);
        t = d.t;
        irecent = -1;
    }

    t_rt_slice(const t_rt_slice &d):
        t(d.t), A(d.A), I(d.I), irecent(-1){;}

    t_rt_slice(T time = T(), int N = 0, T def = T()):
        t(time), A(N, def), I(N), irecent(-1){;}
};

/*! \brief - wrapper for QReadWrite lock
 */
class rt_qt_lock : public t_rt_lock{

private:
    QReadWriteLock lock;
    virtual void lockRead(){ lock.lockForRead(); }
    virtual void lockWrite(){ lock.lockForWrite(); }
    virtual void unlock(){ lock.unlock(); }

public:
    rt_qt_lock():
        lock(QReadWriteLock::NonRecursive)
    {
    }

    virtual ~rt_qt_lock(){;}
};

/*! \brief - encapsulation for multibuffer fixed number of readers
 * typedef is unusable in this case
 */
template <class T> class rt_idf_circ_simo<T> : public virtual i_rt_dataflow
{
private:
    rt_qt_lock lock;
    t_multibuffer<T, RT_MAX_READERS> data;

protected:
    //for internal working object usage
    virtual int write(const void *smp){ return data::write((T *)smp); }
    virtual int set(const void *smp){ return data::set((T *)smp); }
    virtual int writeSpace(){ return data::writeSpace(0); }

    /* typedef arrayListType<elemType> Parent; or this for non C++11
      * otherway use 'using' keyword */
     using t_multibuffer<T, RT_MAX_READERS>::buf;
     using t_multibuffer<T, RT_MAX_READERS>::size;
     using t_multibuffer<T, RT_MAX_READERS>::wmark;
     using t_multibuffer<T, RT_MAX_READERS>::overflow;
     using t_multibuffer<T, RT_MAX_READERS>::rmark;

public:
    //extern object can read too (but cannot change)
    virtual const void *read(int n = 0){return data::read(n); }
    virtual const void *get(int n = 0){return data::get(n); }
    virtual int readSpace(int n = 0){return data::readSpace(n); }

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
        lock(),
        data(_size, lock)
    {
    }

    virtual ~rt_idf_circ_simo(){;}
};


/*! \brief tempate class of data storage suitable for rt_base
 * circular, single input multi output (multireader), double buffer wit no need for warapped copy on read acceess
 */

template <class T> class rt_idf_circ2buf_simo<T> : public virtual i_rt_dataflow
{
private:
    rt_qt_lock lock;
    t_doublebuffer<T, RT_MAX_READERS> data;

    /* typedef arrayListType<elemType> Parent; or this for non C++11
      * otherway use 'using' keyword */
     using t_multibuffer<T, RT_MAX_READERS>::buf;
     using t_multibuffer<T, RT_MAX_READERS>::size;
     using t_multibuffer<T, RT_MAX_READERS>::wmark;
     using t_multibuffer<T, RT_MAX_READERS>::overflow;
     using t_multibuffer<T, RT_MAX_READERS>::rmark;

protected:
    //for internal working object usage
    virtual int write(const void *smp){ return data::write((T *)smp); }
    virtual int set(const void *smp){ return data::set((T *)smp); }
    virtual int writeSpace(){ return data::writeSpace(0); }

public:
    //extern obejct can read too (but cannot change)
    virtual const void *read(int n = 0){return data::read(n); }
    virtual const void *get(int n = 0){return data::get(n); }
    virtual int readSpace(int n = 0){return data::readSpace(n); }

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
        lock(),
        data(_size, lock)
    {
    }

    virtual ~rt_idf_circ2buf_simo(){;}
};



#endif // RT_DATAFLOW_H
