#ifndef RT_GRAPHICS_H
#define RT_GRAPHICS_H

#include "rt_output.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>

class QPainter;
class QOpenGLContext;
class QOpenGLPaintDevice;


enum t_rt_graph_object_item {

    RT_GR_OBJ_VERTEXBUF = 0,    /*!< 0 - vertex buffer */
    RT_GR_OBJ_TIMESL,       /*!< 1 - vertical slices indexes */
    RT_GR_OBJ_FREQSL,       /*!< 2 - horizontal slices */
    RT_GR_OBJ_TRISURF,      /*!< 3 - surface triangulation */
    RT_GR_OBJ_AXES,       /*!< 4 - axis and fractions */
    RT_GR_OBJ_LABELS,       /*!< 5 - axis and fractions */
    RT_GR_OBJ_GRAD,       /*!< 6 - color gradient sub */
    RT_GR_OBJ_NUMBER
};


class rt_window : public QWindow, protected QOpenGLFunctions {

private:
    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;
    QOpenGLShaderProgram *m_program;

    QMatrix4x4 m_matrix;  //perspective matrix
    QPoint  m_fp;  //coord mouse press down

    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;
    GLuint m_vbo[RT_GR_OBJ_NUMBER];

    virtual void mouseMoveEvent(QMouseEvent *ev);
    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void mouseReleaseEvent(QMouseEvent *ev);
    virtual bool event(QEvent *event);

private:
    void render();

public:

    void render(GLuint vbo[], int n);

    rt_window(QObject *parent);
    virtual ~rt_window(){

        if(m_device)
            delete m_device;
    }
};


class rt_graphics : public t_rt_base
{
    Q_OBJECT

private:
    int M;
    int N;
    QJsonArray m_selected;

    rt_window *m_winGraph;
    void *m_bufRaw;
    GLuint m_bufObject[RT_GR_OBJ_NUMBER];

public slots:
    void start();
    void pause();
    void process();
    void change();

public:

    explicit rt_graphics(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_base(parent, resource),
        m_winGraph(new rt_window(parent)),
        m_bufRaw(NULL)
    {

        glGenBuffers(RT_GR_OBJ_NUMBER, m_bufObject);   // Create vertex buffer objects
        change();
    }

    virtual ~rt_graphics(){

        glDeleteBuffers(RT_GR_OBJ_NUMBER, m_bufObject);
        if(m_bufRaw) free(m_bufRaw);
    }
};


#endif // RT_GRAPHICS_H
