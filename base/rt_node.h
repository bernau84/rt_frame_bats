#ifndef RT_NODE
#define RT_NODE

#include <QObject>
#include "rt_base.h"


/*! \brief - implement qt style interconnection instead of simple callback in i_rt_base
 *  introduce simple state machine
 *
 * qt signal ensures queueing, we dont have to bother with lock
 * (process slot are private to prevent dorect calling)
 *
 * omits totaly the callbacks in t_rt_base
 */
class rt_node : public QObject, private virtual i_rt_base
{
    Q_OBJECT

public:
    enum t_rt_a_sta {
        Stopped = 0,
        Active = 1,
        Suspended = 0x80  //flag
    } state;

signals:
    void on_update(rt_node *);   //zmena zvnejsi - signal vede na slot change
    void on_change(rt_node *);  //propagace zmeny zevnitr k navazanym prvkum

protected slots:

    virtual void update(rt_node *from){

        if((state == Active) && (reader_i >= 0) && from){

            while(from->readSpace(reader_i));
                i_rt_base::update(from->read(reader_i));

            if(i_rt_base::readSpace())
                emit on_update(this);
        }
    }

    virtual void change(rt_node *from){

        Q_UNUSED(from);
        i_rt_base::change();
        emit on_update(this);
    }

public:
    void connection(rt_node *to){

        i_rt_base::connection(to); //init reader index

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

    rt_node(QDir resource, QObject *parent = NULL):
        i_rt_base(resource, RT_BLOCKING),
        QObject(parent)
    {
        state = Stopped;
    }

    virtual ~rt_node(){;}
};

#endif // RT_NODE


