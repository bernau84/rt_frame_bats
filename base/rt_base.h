#ifndef T_RT_BASE_H
#define T_RT_BASE_H

#include <QObject>
#include <QTime>
#include <QDir>
#include <QSignalMapper>

#include "rt_setup.h"
#include "rt_dataflow.h"


/*! symnames for floating point dataflow buffers */
typedef rt_dataflow_circ_simo_tfslices<double> t_sl_fpcbuf;
typedef rt_dataflow_circ_simo_double<double> t_fpdbuf;


/*! \brief - interface for execution content of rt node
 * adds setup support
*/
class i_rt_base
{

protected:
    t_collection par;

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

    /*! \brief data to analyse/process */
    virtual void update(const void *dt, int size) = 0;
    /*! \brief someone changed setup or input signal property (sampling frequency for example) */
    virtual void change() = 0;
    /*! \bries merge of recent and new setup */
    virtual void setup(QMap<QString, QVariant> &npar){

        foreach(QMap<QString, QVariant>::iterator nval, npar){

            t_setup_entry val;
            if(par.ask(nval.first, &val)){  //get the config item first

                val.set(nval.second);  //update actual value (with all restriction applied)
                par.replace(nval.first, val); //writeback
            }
        }

        change();
    }
    virtual ~i_rt_base();
    i_rt_base(const QDir &resource = QDir()):
        par(__set_from_file(resource.absolutePath())){

        change();  //to apply inited parameter
    }
};


/*! \brief - interface for other sliced multibuffer items in rt network */
template <typename T> class i_rt_sl_csimo_te : public i_rt_base,
        public rt_dataflow_circ_simo_tfslices<T>
{
private:
    int rd_n;   //pocet registrovanych prvku pro cteni
    int rd_i;   //cteci index v multibufferu predchoziho prvku

public:
    /*! \brief - set new reader ( = forward connection ) */
    virtual int subscriber(i_rt_dataflow *nod){  //vraci index pod kterym muze pristupovat do multibufferu

        if(rd_n >= RT_MAX_READERS) return -1; //zadnej dalsi prvek uz pripojit nemuzeme

        //read out buffered data - prevent overload new item
        t_rt_slice<T> dm;
        for(int n=0; n<nod->readSpace(rd_n); n++)
            nod->read<t_rt_slice<T> >(&dm, rd_n);

        return rd_n++;  //dalsi pijavice
    }

    /*! \brief callbacks annonced new data from source
     * we can implemented it for all sliced multibuffer centraly */
    virtual void update_notif(i_rt_dataflow &src){

        /* expect sourced object is the same type as we are */
        t_rt_slice<T> w;
        while(read<t_rt_slice<T> >(w, rd_i))
            update(w.A.data(), w.A.size());
    }

public:
    i_rt_sl_csimo_te(QObject *parent){

        rd_n = 1;   //kolik prvku uz mame registrovano - index 0. vyhrazen pro tento prvek (vyzaduje to management - musime cist co jsem zapsali)
        rd_i = -1;   //index pod kterym muze vycitat data z predchoziho prvku
    }

    virtual ~i_rt_sl_csimo_te(){;}
};

#endif // T_RT_BASE_H

