#ifndef RT_COMMON_H
#define RT_COMMON_H


/*! \brief - interface definition for lock - depend on concrete implementation
 * \note - QReadWriteLock can be use for Qt, Mutex in C03 standard */

#include <QReadWriteLock>
class t_rt_lock {

public:
    virtual void lockRead(){;}
    virtual void lockWrite(){;}
    virtual void unlock(){;}

    t_rt_lock(){;}
    virtual ~t_rt_lock(){;}
};


///*! \brief - std mutex lock wrapper
// *  work only under c++11
// */
////#include <mutex> for c++11
//using namespace std;
//class rt_std_lock : public t_rt_lock {

//private:
//    std::mutex lock;
//    bool locked_rd;
//    bool locked_wr;

//public:
//    //read lock only when writing in procees, on other hand it is not need
//    virtual void lockRead(){ if(locked_wr){ locked_rd = true; lock.lock(); } }
//    virtual void lockWrite(){ locked_wr = true; lock.lock(); }
//    virtual void unlock(){ lock.unlock(); locked_rd = locked_wr = false; }
//    bool locked(){ return (locked_rd || locked_wr) ? true : false; }

//    rt_std_lock(){ ;}
//    virtual ~rt_std_lock(){;}
//};


/*! \brief - alternative for non os system
 * it is not full version of lock
 */
class rt_nos_lock : public t_rt_lock {

private:
    volatile bool lock;
    volatile bool locked_rd;
    volatile bool locked_wr;

    void _lock(){

        bool tlock;
        while(true == (tlock = lock));
        lock = true;
    }

public:
    //read lock only when writing in procees, on other hand it is not need
    virtual void lockRead(){

        if(!locked_wr) return;
        locked_rd = true;
        _lock();
    }
    virtual void lockWrite(){

        locked_wr = true;
        _lock();
    }

    virtual void unlock(){

        lock = false;
        locked_rd = locked_wr = false;
    }

    bool locked(){

        return (locked_rd || locked_wr) ? true : false;
    }

    rt_nos_lock(){

        unlock();
    }

    virtual ~rt_nos_lock(){;}
};


#endif // RT_COMMON_H
