#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QSignalMapper>

#include "rt_common.h"
#include "rt_setup.h"
#include "rt_dataflow.h"

/*! \brief - interface for object with io setup encapsulation and working interface
 * child of this object are usable stand alone - need no other interface
 */
class i_rt_worker : public virtual i_rt_dataflow_output
{

protected:
    t_collection par;  //setup io & storage

    //constructor helpers
    QJsonObject __set_from_file(const QString path){

        /*! default config */
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::ReadOnly | QIODevice::Text)){

            QByteArray f_data = f_def.read(64000);
            QJsonDocument js_doc = QJsonDocument::fromJson(f_data);
            if(!js_doc.isEmpty())
                return js_doc.object();
        }

        return QJsonObject();
    }

    QJsonObject __set_from_binary(const QByteArray _path){
        /*! \todo - from JsonDocument binary of from remote data */
        path = _path;
        return QJsonObject();
    }

public:
    virtual void update(const void *sample) = 0;  /*! \brief data to analyse/process */
    virtual void change() = 0;  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    virtual ~i_rt_worker();
    i_rt_worker(const QDir &resource = QDir()):
        par(__set_from_file(resource.absolutePath())){

        change();  //to apply inited parameter
    }
};

/*! \brief - worker warapper and interface boundary between object
 * do not depend on qt signal-slot instead use queue and calabck (rt_regime option)
 *
 * child obejct can overload sig_update/change and proc_update/change
 * to modify the signal <-> process mechanism
*/

enum e_rt_regime {

    RT_BLOCKING = 0,
    RT_QUEUED
};

class t_rt_base : virtual public i_rt_worker
{
protected:
    e_rt_regime m_mode;

    //props for acces trough interface
    std::vector<const i_rt_base *> subscribers;
    int reader_i;

    //queue mode props
    std::list<const void *> pending_smp;
    i_rt_dataflow_output *pending_src;

    //lock for exclusive run of update
    rt_std_lock m_lock;

    /*! \brief sample process & notification to follower
     */
    void __update(const void *sample){

        lock.lock();
        i_rt_worker::update(sample); //process sample
        lock.unlock();

        while(readSpace()){

            const void *sample = read();
            for(int i=0; i<subscribers.size(); i++){

                subscribers[i]->sig_update(sample); //report each sample
                if(!i) subscribers[i]->sig_update(this);  //only once
            }
        }
    }


    /*! \brief safe calling of change according to setup parameters
     * reset proccessing also
     */
    void __change(){

        //wait for empty queue - samples may become invalid ater updade
        //for resizing internal buffer
        while(pending_samples());

        lock.lock();
        i_rt_worker::change(); //process sample
        lock.unlock();
    }

public:
    /*! \brief allow follower to register as independant reader
     * if they wan to work in buffered mode using i_rt_dataflow_output
     * interface
     * \return 1..RT_MAX_READERS-1, 0 is reserved for internal use */
    int subscribe(const i_rt_base *reader){

        if(subscribers.size() >= RT_MAX_READERS)
            return -1;

        subscribers.push_back(reader);
        return subscribers.size();
    }

    /*! \brief calling subscibe of remote source */
    int connection(const i_rt_base *to){

        reader_i = to->subscribe(this);
    }

    /*! \brief query of remaining samples to procceed
     * for ballancing load and for waiting for idle state (== 0)
     */
    int pending_samples(){

        if(pending_src && (reader_i >= 0))  //reader is valid only in cached mode
            return pending_src->readSpace(reader_i);

        return pending_smp.size();
    }

    /*! \brief received notification of new data to process;
     * in dependance of mode data are immediately procesed or
     * information is saved for next time;
     *
     * for data caching is responsible source
     */
    void sig_update(i_rt_dataflow_output *src){ //store or process instantly

        pending_src = src;
        if(m_mode == RT_QUEUED)
            if(lock.locked())
                return;

        if(reader_i >= 0)
            while(NULL != (sample = pending_src->read(reader_i)))
                __update(sample);
    }

    /*! \brief received notification of new data to process;
     * in dependance of mode sample is immediately procesed or
     * pointer is cached in internal queue
     */
    void sig_update(const void *sample){ //process instantly (sample has not quarantine validity)

        pending_smp.push_back(sample);
        if(m_mode == RT_QUEUED)
            if(lock.locked())
                return;

        while(pending_smp.size())
            __update(pending_smp.pop_front());
    }

    /*! \brief safe calling of change according to setup parameters
     * reset proccessing also
     */
    virtual void sig_change(){

        proc_change();
    }


    /*! \brief setup collection io, read for empty/default v
     */
    QVariant setup(const QString &name, QVariant v = QVariant()){

        t_setup_entry val;
        if(0 == par.ask(name, &val))  //get the config item first
            return QVariant(); //parametr of name do not exists

        if(v.isValid()){

            val.set(v);  //update actual value (with all restriction applied)
            par.replace(nval.first, val); //writeback

            __change();
        }

        return val.get();
    }

    t_rt_base(const QDir &resource, e_rt_regime mode = RT_BLOCKING):
        i_rt_worker(resource),
        reader_i(-1),
        pending_smp(),
        pending_src(NULL),
        m_mode(mode)
    {
    }

    virtual void ~t_rt_base(){

    }
}




#endif // T_RT_BASE_H

