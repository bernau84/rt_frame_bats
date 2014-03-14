#include "t_rt_base.h"

t_rt_base::t_rt_base(QObject *parent, QDir &config) :
    QObject(parent), set(config), t_slcircbuf(0)
{
    sta.fs_in = (double *)0; //vstupni fs
    sta.fs_out = 0; //vystuni fs
    sta.state = t_rt_status::StoppedState;

    rd_n = 0;   //kolik prvku uz mame registrovano
    rd_i = 0;   //index pod kterym muze vycitat data z predchoziho prvku

    nn_tot = 0; //pocet zpracovanych vzorku
    tm_tot = 0; //celkova dobe behu funkce process (v sec)

    if(parent){

        t_rt_base *pre = dynamic_cast<t_rt_base *>(parent);
        if(!pre) return; //navazujem na kompatibilni prvek? to do test!
        rd_i = pre->attach(this);    //nastavi sta.fs_in a rd_I
    }

    QFile f_res(config);
    if(f_res.open(QIODevice::ReadOnly | QIODevice::Text)){

        QByteArray f_data = f_res.read(64000);
        QJsonDocument js_doc = QJsonDocument::fromJson(f_data);
        if(!js_doc.isEmpty()){

            set = js_doc.object();
        }
    }
}

int t_rt_base::attach(t_rt_base *next){

    if(rd_n >= RT_MAX_SUCCESSORS) return 0; //zadnej dalsi prvek uz pripojit nemuzeme

    connect(this, SIGNAL(on_change()), next, SLOT(change()));  //reakce na zmeny nastaveni, chyby, zmeny rezimu
    connect(this, SIGNAL(on_update()), next, SLOT(process())); //reakce na nova data

    rd_n += 1;  //dalsi pijavice
    t_slcircbuf::readShift(t_slcircbuf::readSpace(rd_n), rd_n); //reset rd_cntr tak aby byl pripraven

    next->sta.fs_in = &sta.fs_out; //napojim info o vzorkovacich frekv.
    return rd_n;
}

void t_rt_base::start(){

    sta.state = t_rt_status::ActiveState;
}

void t_rt_base::pause(){

    sta.state = t_rt_status::SuspendedState;
}

