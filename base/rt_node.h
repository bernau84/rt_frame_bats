#ifndef RT_NODE
#define RT_NODE

//#include <assert.h>
#include <QObject>
#include <QtGlobal>
#include "rt_base.h"


/*! \brief - implement qt style interconnection instead of simple callback in i_rt_base
 *  introduce simple state machine
 *
 * qt signal ensures queueing, we dont have to bother with lock
 * (process slot are private to prevent dorect calling)
 *
 * omits totaly the callbacks in t_rt_base
 */
class rt_node : public QObject
{
    Q_OBJECT

protected:
    //not need tobe declared friend because rt_node is implicitly friend with itself, hmm...
    i_rt_base *base; /*! child not have to initilize base with real work base class */

public:
    enum t_rt_a_sta {
        Stopped = 0,
        Active = 1,
        Suspended = 0x80  //flag
    } state;

signals:
    void on_update(const void *sample);   //zmena z vnejsi - signal vede na slot change
    void on_change();  //propagace zmeny zevnitr k navazanym prvkum

protected slots:

    virtual void update(const void *sample){

        //assert(base); - asserty asi vadi mingw debugeru - viz.https://forum.qt.io/topic/7108/solved-qtcreator-2-2-1-crashes-when-debugging/12

        if(state == Active){

            base->on_update(sample); //process

            //test if signals for sucessor occured
            for(std::pair<e_rt_signals, const void *> sig = base->pop_signal();
                sig.first != RT_SIG_EMTY;
                sig = base->pop_signal()){

                //base->notify_all(sig.first, sig.second); //direct call
                emit on_update(this); //qt signal/slot
            }
        }
    }

    virtual void change(){

        //assert(base);

        base->change();
        emit on_update(this);
    }

public:

    void init(i_rt_base *_base){

        //assert(_base);
        base = _base;
    }

    void connection(const rt_node *to){

        //assert(to);
        if(!to) return;

        base->connection(to->base); //init reader index

        //bound together
        connect(to, SIGNAL(on_update(const void *)), this, SLOT(update(const void *)));
        connect(to, SIGNAL(on_change()), this, SLOT(change()));
    }

    virtual void start(){

        state = Active;
    }

    virtual void stop(){

        state = Stopped;
    }

    virtual void pause(){

        state = (t_rt_a_sta)(state + Suspended);
    }

    virtual void resume(){

        state = (t_rt_a_sta)(state - Suspended);
    }

    rt_node(QObject *parent = NULL):
        QObject(parent)
    {
        state = Stopped;
        base = NULL;
    }

    virtual ~rt_node(){
        //empty
    }
};

#endif // RT_NODE


