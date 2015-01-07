#ifndef RT_GRAPHICS_H
#define RT_GRAPHICS_H

#include "outputs/rt_output.h"
#include "base/rt_doublebuffer.h"
#include "base/rt_base.h"
#include "graphs/rt_ogl_object.h"
#include "graphs/rt_ogl_frame.h"
#include "graphs/rt_ogl_memory.h"
#include "graphs/rt_ogl_context.h"
#include "graphs/rt_graph_scaler.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>
#include <QSignalMapper>
#include <QVector3d>

enum e_rt_graph_object {

    RT_OBJ_0_PNTS,         /*!<points */
    RT_OBJ_0_TIMESL,       /*!<vertical slices */
    RT_OBJ_0_FREQSL,       /*!<horizontal slices */
    RT_OBJ_0_TRISURF,      /*!<surface triangulation */
    RT_OBJ_0_AXIS_X,       /*!<axis and fractions */
    RT_OBJ_0_AXIS_Y,       /*!<axis and fractions */
    RT_OBJ_0_AXIS_Z,       /*!<axis and fractions */
    RT_OBJ_0_LABELS,       /*!<axis labels */

    RT_OBJ_1_PNTS,
    RT_OBJ_1_TIMESL,
    RT_OBJ_1_FREQSL,
    RT_OBJ_1_TRISURF,
    RT_OBJ_1_AXIS_X,
    RT_OBJ_1_AXIS_Y,
    RT_OBJ_1_AXIS_Z,
    RT_OBJ_1_LABELS,

    RT_OBJ_2_PNTS,
    RT_OBJ_2_TIMESL,
    RT_OBJ_2_FREQSL,
    RT_OBJ_2_TRISURF,
    RT_OBJ_2_AXIS_X,
    RT_OBJ_2_AXIS_Y,
    RT_OBJ_2_AXIS_Z,
    RT_OBJ_2_LABELS,


    /* and so */
    RT_OBJ_NUMBER = 1000
};

enum e_axis_jtfa {

    EAXIS_U_TIME = EAXIS_X,
    EAXIS_U_AMP = EAXIS_Z,
    EAXIS_U_FREQ = EAXIS_Y
};

//typedef struct {
//    double min;
//    double max;
//} t_rt_scale;

/*! \brief - encapsulation for multibuffer fixed number of readers
typedef is unusable in this case
*/
class t_3dcircbuf : public t_doublebuffer<t_rt_graph_v, RT_MAX_READERS>{

private:
    rt_qt_lock lock;

public:

    /*! \brief resize & reset */
    virtual void resize(int _reserved_size, t_rt_graph_v *_buf = NULL){

        if(_buf) buf  = _buf;
        size = _reserved_size / 2;

        wmark = 0;
        for(int i=0; i<N; i++){

            overflow[i] = -1;
            rmark[i] = 0;
        }
    }

    /*! \brief - fist is reserved not buffer size - it will be just half */
    t_3dcircbuf(int _reserved_size, t_rt_graph_v *_buf):
        lock(),
        t_doublebuffer(0, &lock){  //no dynamic allocation!

        buf  = _buf;
        size = _reserved_size / 2;
    }

    virtual ~t_3dcircbuf(){

        buf = NULL; //prevent delete at t_multibuffer
    }
};

/*! \brief - joint of rt and data object, we do not expect there will be any follower, graph is
 * going to be terminal object
 */
class t_rt_graph_base : public t_rt_empty, t_3dcircbuf {

    Q_OBJECT

public:
    explicit t_rt_graph_base(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_empty(parent, resource),
        t_3dcircbuf(0, NULL)
    {

    }

    virtual ~t_rt_graph_base(){;}

public slots:
    virtual void start(){
        t_rt_base::start();
    }

    virtual void stop(){
        t_rt_base::stop();
    }

};

class rt_graph : public t_rt_graph_base
{
    Q_OBJECT

private:
    int histsz; //how many time(x) samples we would keep
    int frefresh; //refresh frequency
    double last_refresh;  //timestamp

    struct {
        double scale_lo;
        double scale_hi;
        double angle;
    } trans[EAXIS_NUMB];

    rt_graph_context *m_canvas;  //inherits QWindow
    QList<rt_graph_frame *> m_frame; //object maintaner
    QMap<e_rt_graph_object, rt_graph_object *> m_objects; //graph items collection (graph1, axis1, color-scale1, graph2, ...)
    rt_autoscale m_rescaler[EAXIS_NUMB];
    QSignalMapper m_mapper;

public slots:
    void process(void){

        t_slcircbuf *source = dynamic_cast<t_slcircbuf *>pre;
        if(!source) return; //navazujem na zdroj dat?

        t_rt_slice<double> w;  //radek casovych dat
        while(source->read(&w, rd_i)){ //vyctem radek

            if(sta.state != t_rt_status::ActiveState){

                int n_dummy = source->readSpace(rd_i); //zahodime data
                source->shift(n_dummy, rd_i);
            } else {

                QVector<t_rt_graph_v> v3f;
                //convert to t_rt_graph_v and write
                for(int i=0; i<w.A.size(); i++){

                    t_rt_graph_v p3f = { w.t, w.A[i], w.I[i] };  //time, amplitude, freq.
                    t_3dcircbuf::write(&p3f);
                    v3f << p3f;
                }

                emit on_update();  //to rescallers
            }
        }
    }

    void change(void){

        histsz = set["Multibuffer"].get().toInt();
        frefresh = set["Rate"].get().toInt();

        pause();

        uint32_t global_sz = RT_GRAPH_ALLOC_DEFSZ / 2; //or exactly - 2*sizeof(t_rt_graph_v)*MAX_FREX_POINTS*histsz; //vertex
        float *global =  m_frame.first()->alloc(global_sz);
        t_3dcircbuf::resize(global_sz, global);

        sta.fs_out = frefresh;
        last_refresh = 0;

        resume();

        emit on_change(); //poslem dal
    }

    void rescale(int id){

        QVector<float> divs;  //divides of axis
        m_rescaler[id].read(divs, 1);  //update selected

        trans[id].scale_lo = divs.first(); //min
        trans[id].scale_hi = divs.last();  //max

        QVector3D lo(trans[EAXIS_X].scale_lo, trans[EAXIS_Y].scale_lo, trans[EAXIS_Z].scale_lo);
        QVector3D hi(trans[EAXIS_X].scale_hi, trans[EAXIS_Y].scale_hi, trans[EAXIS_Z].scale_hi);

        //update all objects scale
        //every object based on graph data
        foreach(QMap<e_rt_graph_object, rt_graph_object *>::Iterator o, m_objects){

            if(o.value().m_ver == /*nejaky vbo zarucene pochazejici z dat */){

                o.value().translate(lo);  //recount matrix
                o.value().scale(1/(hi.x - lo.x), 1/(hi.y - lo.y), 1/(hi.z - lo.z));
            }
        }
    }

public:

    explicit rt_graph(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_graph_base(parent, resource),
        m_canvas(new rt_graph_context(parent)),
        m_mapper(this)
    {
        m_frame << new rt_graph_frame(&m_canvas);

        //connect update scale signal of particullar axis to graph
        //and uprade signal from graph to rescallers
        QObject::connect(&m_mapper, SIGNAL(mapped(int)), this, SLOT(rescale(int));
        for(int a=0; a<(int)EAXIS_NUMB; a++){

            m_rescaler[a].attach(this);  //set source of data for each rescaler
            QObject::connect(&m_rescaler[a], SIGNAL(on_update()), &m_mapper, SLOT(map()));
            m_mapper.setMapping(&m_rescaler[a], a);
        }

        change();



        //resevr some space for doublebuffer
    }

    virtual ~rt_graphics(){

        while(m_frame.count())
            delete m_frame.last();
    }
};


#endif // RT_GRAPHICS_H
