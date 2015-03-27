#ifndef RT_COMMON_H
#define RT_COMMON_H


/*! \brief - interface definition for lock - depend on concrete implementation
 * \note - QReadWriteLock can be use for Qt, Mutex in C03 standard */

class t_rt_lock {

public:
    virtual void lockRead(){;}
    virtual void lockWrite(){;}
    virtual void unlock(){;}

    t_rt_lock(){;}
    virtual ~t_rt_lock(){;}
};


/*! \brief - std mutex lock wrapper
 */
class rt_std_lock : public t_rt_lock {

private:
    mutex lock;
    bool locked_rd;
    bool locked_wr;

public:

    virtual void lockRead(){ locked_reading = true; lock.lock(); }
    virtual void lockWrite(){ locked_writing = true; lock.lock(); }
    virtual void unlock(){ lock.unlock(); locked_rd = locked_wr = false; }
    bool locked(){ return (locked_rd || locked_wr) ? true : false; }

    rt_std_lock(){ ;}
    virtual ~rt_std_lock(){;}
};



#endif // RT_COMMON_H
