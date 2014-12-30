#include "t_rt_base.h"

t_rt_empty::t_rt_empty(QObject *parent, const QDir &resource) :
    QObject(parent),
    set(__set_from_file(resource.absolutePath()))
{
    sta.fs_in = (double *)0; //vstupni fs
    sta.fs_out = 0; //vystuni fs
    sta.state = t_rt_status::StoppedState;
    sta.nn_tot = sta.nn_run = 0;

    rd_n = 1;   //kolik prvku uz mame registrovano - index 0. vyhrazen pro tento prvek (vyzaduje to management - musime cist co jsem zapsali)
    rd_i = -1;   //index pod kterym muze vycitat data z predchoziho prvku
}

QJsonObject t_rt_base::__set_from_file(const QString path){

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


int t_rt_base::attach(t_rt_base *next){

    if(rd_n >= RT_MAX_READERS) return -1; //zadnej dalsi prvek uz pripojit nemuzeme

    connect(this, SIGNAL(on_change()), next, SLOT(change()));  //reakce na zmeny nastaveni, chyby, zmeny rezimu
    connect(this, SIGNAL(on_update()), next, SLOT(process())); //reakce na nova data

    t_slcircbuf::shift(t_slcircbuf::readSpace(rd_n), rd_n); //reset rd_cntr tak aby byl pripraven
    next->sta.fs_in = &sta.fs_out; //napojim info o vzorkovacich frekv.
    return rd_n++;  //dalsi pijavice
}

