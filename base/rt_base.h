#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QSignalMapper>

#include "rt_common.h"
#include "rt_setup.h"
#include "rt_dataflow.h"

class i_rt_worker_io : public virtual i_rt_dataflow_output {

public:
    virtual void update(const void *sample) = 0;  /*! \brief data to analyse/process */
    virtual void change() = 0;  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    i_rt_worker_io(){

    }

    virtual ~i_rt_worker_io(){

    }
};

/*! \brief - worker warapper and interface boundary between object
 * do not depend on qt signal-slot instead use queue and calabck (rt_regime option)
 *
 * child obejct can overload sig_update/change and proc_update/change
 * to modify the signal <-> process mechanism
 *
 * io setup encapsulation
*/

enum e_rt_regime {

    RT_BLOCKING = 0,
    RT_QUEUED
};

class i_rt_base : virtual public i_rt_worker_io
{
protected:
    e_rt_regime m_mode;

    //props for acces trough interface
    std::vector<i_rt_base *> subscribers;
    int reader_i;

    //queue mode props
    std::list<const void *> pending_smp;
    i_rt_dataflow_output *pending_src;

    //lock for exclusive run of update
    rt_nos_lock m_lock;

    t_collection par;  //setup io & storage

private:
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

    /*! \brief sample process & notification to follower
     */
    void __update(const void *sample){

        m_lock.lockWrite();
        update(sample); //process sample
        m_lock.unlock();

        bool lfirst = true;
        while(readSpace()){

            const void *lsample = read();
            for(unsigned i=0; i<subscribers.size(); i++){

                subscribers[i]->sig_update(lsample); //report each sample
                if(lfirst) subscribers[i]->sig_update((i_rt_dataflow_output *)this);  //only 1x
            }

            lfirst = false;
        }
    }


    /*! \brief safe calling of change according to setup parameters
     * reset proccessing also
     */
    void __change(){

        //wait for empty queue - samples may become invalid ater updade
        //for resizing internal buffer
        while(pending_samples());

        m_lock.lockWrite();
        change(); //process sample
        m_lock.unlock();
    }

public:
    /*! \brief allow follower to register as independant reader
     * if they wan to work in buffered mode using i_rt_dataflow_output
     * interface
     * \return 1..RT_MAX_READERS-1, 0 is reserved for internal use */
    int subscribe(i_rt_base *reader){

        if(subscribers.size() >= RT_MAX_READERS)
            return -1;

        subscribers.push_back(reader);
        return subscribers.size();
    }

    /*! \brief calling subscibe of remote source */
    int connection(i_rt_base *to){

        return ((reader_i = to->subscribe(this)));
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
            if(m_lock.locked())
                return;

        const void *sample;
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
            if(m_lock.locked())
                return;

        while(pending_smp.size()){

            __update(pending_smp.front());
            pending_smp.pop_front();
        }
    }

    /*! \brief safe calling of change according to setup parameters
     * reset proccessing also
     */
    void sig_change(){

        __change();
    }


    /*! \brief setup collection io, read for empty/default v
     */
    QVariant setup(const QString &name, QVariant v = QVariant()){

        t_setup_entry val;
        if(0 == par.ask(name, &val))  //get the config item first
            return QVariant(); //parametr of name do not exists

        if(v.isValid()){

            val.set(v.toJsonValue());  //update actual value (with all restriction applied)
            par.replace(name, val); //writeback

            __change();
        }

        return val.get();
    }

    i_rt_base(const QDir &resource, e_rt_regime mode = RT_BLOCKING):
        par(__set_from_file(resource.absolutePath()))
    {
        reader_i = -1;
        m_mode = mode;
        pending_src = NULL;
    }

    virtual ~i_rt_base(){
        //empty
    }
};




#endif // T_RT_BASE_H

