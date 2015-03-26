#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QSignalMapper>

#include "rt_setup.h"
#include "rt_dataflow.h"

/*! \brief - interface for object with io setup encapsulation
 * and working interface
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

    virtual void update(QMap<QString, QVariant> &npar){      /*! \brief merge of recent and new setup */

        foreach(QMap<QString, QVariant>::iterator nval, npar){

            t_setup_entry val;
            if(par.ask(nval.first, &val)){  //get the config item first

                val.set(nval.second);  //update actual value (with all restriction applied)
                par.replace(nval.first, val); //writeback
            }
        }

        change();
    }

    virtual ~i_rt_worker();
    i_rt_worker(const QDir &resource = QDir()):
        par(__set_from_file(resource.absolutePath())){

        change();  //to apply inited parameter
    }
};

/*! \brief - interface for object with io setup encapsulation
 * and workoing interface
*/
class i_rt_base : virtual public i_rt_worker
{
private:
    std::vector<const i_rt_base *> subscribers;
    i_rt_dataflow_output *pending_src;
    int reader_i;

    t_lock lock;
public:
    int subscribe(const i_rt_base *reader){

        if(subscribers.size() >= RT_MAX_READERS)
            return -1;

        subscribers.push_back(reader);
        return subscribers.size()-1;
    }

    int connect_to(const i_rt_base *source){

        reader_i = source->subscribe(this);
    }

    //zmena interfacu za behu neni pripustna (nepodporujem dynamickou zmenu usporadani
    // takze todle je bezpecne)
    virtual void sig_update(i_rt_dataflow_output *src){ //store or process instantly

        pending_src = src;
        if(lock.isLocked())
            return;

        if(reader_i >= 0)
            while(NULL != (sample = pending_src->read(reader_i)))
                update(sample);
    }

    virtual void sig_update(const void *sample){ //process instantly (sample has not quarantine validity)

        update(sample);
    }

    virtual void update(const void *sample){

        lock.lock();
        i_rt_worker::update(sample); //process sample
        lock.unlock();

        while(readSpace()){

            const void *sample = read();
            for(int i=0; i<subscribers.size(); i++){

                subscribers[i]->sig_update(sample);
                subscribers[i]->sig_update(this);
            }
        }
    }

    i_rt_base(const QDir &resource):
        i_rt_worker(resource),
        reader_i(-1)
    {

    }

    virtual void ~i_rt_base(){

    }
}




#endif // T_RT_BASE_H

