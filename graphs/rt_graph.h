#ifndef RT_GRAPHICS_H
#define RT_GRAPHICS_H

#include "outputs/rt_output.h"
#include "rt_graph_axis.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

//    RT_GR_OBJ_TIMESL,       /*!< 1 - vertical slices indexes */
//    RT_GR_OBJ_FREQSL,       /*!< 2 - horizontal slices */
//    RT_GR_OBJ_TRISURF,      /*!< 3 - surface triangulation */
//    RT_GR_OBJ_AXES,       /*!< 4 - axis and fractions */
//    RT_GR_OBJ_LABELS,       /*!< 5 - axis and fractions */
//    RT_GR_OBJ_GRAD       /*!< 6 - color gradient sub */


enum e_axis_jtfa {

    EAXIS_U_TIME = EAXIS_X,
    EAXIS_U_AMP = EAXIS_Z,
    EAXIS_U_FREQ = EAXIS_Y
};


class rt_color_scale : public t_rt_base, rt_graph_frame {

public:
    rt_graph_context *m_canvas;

public slots:
    on_change();

    /*! \todo
     */

    explicit rt_color_scale(QObject *parent = 0, const QDir &resource = QDir(), rt_graph_context *canvas):
        t_rt_base(parent, resource),
        m_canvas(canvas)
    {
        change();
    }

    virtual ~rt_color_scale(){

    }
};

class rt_graph : public t_rt_base
{
    Q_OBJECT

private:
    rt_graph_context *m_canvas;  //inherits QWindow
    QList<rt_graph_frame *> m_frames; //graph items collection (graph1, axis1, color-scale1, graph2, ...)
    QVector<rt_autoscale> m_rescalers;

public slots:
    void start();
    void pause();
    void process(){

        //feed axis autoscale

        //scale & save into double multibuffer (prevent folding at circular buffer)
    }

    void change();

public:

    explicit rt_graphics(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_base(parent, resource),
        m_canvas(new rt_graph_context(parent)),
        m_rescalers(EAXIS_NUMB)
    {
        change();
    }

    virtual ~rt_graphics(){

    }
};


#endif // RT_GRAPHICS_H
