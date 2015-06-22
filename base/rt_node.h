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
    //friend class rt_node; /*! nodes can share base access to provide data interface */
    //not need because rt_node is implicitly friend with itself, hmm...

    i_rt_base *base; /*! child not have to initilize base with real work base class */

public:
    enum t_rt_a_sta {
        Stopped = 0,
        Active = 1,
        Suspended = 0x80  //flag
    } state;

    int        m_id; //zaloha reader_i z base connection() ktery je jinak private
    i_rt_base *m_src;

signals:
    void on_update(const rt_node *);   //zmena zvnejsi - signal vede na slot change
    void on_change(const rt_node *);  //propagace zmeny zevnitr k navazanym prvkum

protected slots:

    virtual void update(const rt_node *from){

        //assert(base); - asserty vadi mingw debugeru - viz.https://forum.qt.io/topic/7108/solved-qtcreator-2-2-1-crashes-when-debugging/12

        if((state == Active) && (m_id >= 0) && from){

            while(from->base->readSpace(m_id))
                base->update(from->base->read(m_id));

            if(base->readSpace())
                emit on_update(this);
        }
    }

    virtual void change(const rt_node *from){

        //assert(base);

        Q_UNUSED(from);
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

        m_src = to->base;
        m_id = base->connection(m_src); //init reader index

        //bound together
        connect(to, SIGNAL(on_update(const rt_node*)), this, SLOT(update(const rt_node*)));
        connect(to, SIGNAL(on_change(const rt_node*)), this, SLOT(change(const rt_node*)));
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


