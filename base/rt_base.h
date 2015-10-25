#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QSignalMapper>

#include "rt_common.h"
#include "rt_setup.h"
#include "rt_dataflow.h"
#include "rt_tracing.h"

/*! \brief common interface - dataflow_outpu is an interface for access internal buffer
 * update and chaneg are hidden implementation of action - acessible throught on_update and on_change in
 * rt_base child class
*/
class i_rt_worker_io : public virtual i_rt_dataflow_output {

protected:
    virtual void update(const void *sample) = 0;  /*! \brief data to analyse/process */
    virtual void change() = 0;  /*! \brief someone changed setup or input signal property (sampling frequency for example) */

    i_rt_worker_io(){;}
    virtual ~i_rt_worker_io(){;}
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

    RT_BLOCKING = 0,  //direct call from one update() to another
    RT_QUEUED,  //signal is always queueq, signalization is maitained outside
    RT_TERMINAL  //no signal is spread from this node
};

enum e_rt_signals {

    RT_SIG_EMTY,
    RT_SIG_SOURCE_UPDATED,
    RT_SIG_CONFIG_CHANGED
};

class i_rt_base : virtual public i_rt_worker_io
{
protected:
    e_rt_regime m_mode;

    //list of sucessor
    std::vector<i_rt_base *> subscribers;

    //precesor prop
    int reader_i;  //data acces reader index for this
    i_rt_base *source;  //interface for data

    //queue mode props
    std::list<std::pair<e_rt_signals, const void *> > pending_sig;

    //lock for exclusive run of update
    rt_nos_lock m_lock;

    //setup io & storage
    t_collection par;

    //tracing op
    t_rt_tracer trc;

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

public:

    /*! \brief - direct call of listeners handlers */
    void notify_all(e_rt_signals sig, const void *data){

        for(unsigned i=0; i<subscribers.size(); i++)
            if(subscribers[i] != this){ //prevent recursion when using buffer for itself

                if(sig == RT_SIG_SOURCE_UPDATED) subscribers[i]->on_update(data);
                else if(sig == RT_SIG_SOURCE_UPDATED) subscribers[i]->on_change();
            }
    }

    /*! \brief notification signal handlihg - direct or queued
     */
    void signal(e_rt_signals sig, const void *data){

        if(m_mode == RT_QUEUED){

            if(subscribers.size()){  //not for terminal nodes

                std::pair<e_rt_signals, const void *> prop(sig, data);
                pending_sig.push_back(prop);
                    return;
            }
        }

        if(m_mode == RT_BLOCKING)
            notify_all(sig, data);
    }

    /*! \brief read pending signal and process them externaly
     */
    std::pair<e_rt_signals, const void *> pop_signal(void){

        std::pair<e_rt_signals, const void *> prop(RT_SIG_EMTY, NULL);
        if(pending_sig.size()){

            prop = pending_sig.front();
            pending_sig.pop_front();
        }

        return prop;
    }

    /*! \brief allow follower to register as independant reader
     * if they want to work in blocking / buffered mode using i_rt_dataflow_output
     * interface
     * \return 0..RT_MAX_READERS-1*/
    int subscribe(i_rt_base *reader){

        if(subscribers.size() >= RT_MAX_READERS)
            return -1;

        subscribers.push_back(reader);
        return subscribers.size()-1;
    }

    /*! \brief calling subscibe of remote source */
    int connection(i_rt_base *to){

        source = to;
        return ((reader_i = to->subscribe(this)));
    }

    /*! \brief received notification of new data to process;
     * in dependance of mode sample is immediately procesed or
     * pointer is cached in internal queue
     */
    void on_update(const void *sample){ //process instantly (sample has not quarantine validity)

        m_lock.lockWrite();
        update(sample); //process sample
        m_lock.unlock();
    }

    /*! \brief safe calling of change according to setup parameters
     * reset proccessing also
     */
    void on_change(void){

        m_lock.lockWrite();
        change(); //process change of config
        m_lock.unlock();
    }


    /*! \brief setup collection io, read for empty/default v
     * \todo - template instead of QVariant and std::string for QString
     */
    QVariant setup(const QString &name, QVariant v = QVariant()){

        t_setup_entry val;
        if(0 == par.ask(name, &val))  //get the config item first
            return QVariant(); //parametr of name do not exists

        if(v.isValid()){

            val.set(v.toJsonValue());  //update actual value (with all restriction applied)
            par.replace(name, val); //writeback

            on_change();

            /* we may fire signal instantly or let change() to to that - prefered because
             *      update works at the same way
             *
             * signal(RT_SIG_CONFIG_CHANGED, NULL);
             * signal(RT_SIG_CONFIG_CHANGED, name) lespi ale todle nelze
             *          jedine ze by to slo nejak pretypovat na pointer s trvalou platnosti, ukazatel do par
             *
             * !!!strategii pro change signal vubec nemam; vim jen ze se ma sirit pokud na tom zavisi
             * navazujici nody ale proto je to delane pres jfta buffer by se tomu predeslo
             * zatim tedy neni sireni signalu pro update nutne
             */
        }

        return val.get();
    }

    /*! \brief batch collection update from json string
     */
    void import(const QString &json_config){

        Q_UNUSED(json_config);
        /*! \todo
         */
    }

    /*! \todo cancel dependacy to QDir - use std::string */
    i_rt_base(const QDir &resource, e_rt_regime mode = RT_BLOCKING):
        m_lock(),
        par(__set_from_file(resource.absolutePath())),
        trc()
    {
        reader_i = -1;
        m_mode = mode;
        source = NULL;
    }

    virtual ~i_rt_base(){
        //empty
    }
};




#endif // T_RT_BASE_H

