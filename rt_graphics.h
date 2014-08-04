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

    virtual void	mouseMoveEvent(QMouseEvent *ev);
    virtual void	mousePressEvent(QMouseEvent *ev);
    virtual void	mouseReleaseEvent(QMouseEvent *ev);

public:

    void render(GLuint vbo[], int n);

    QMatrix4x4  getMatrix(){ //for usage in render function

        return m_matrix;
    }

    rt_window(QObject *parent);
    ~rt_window(){

        if(m_device)
            delete m_device;
    }
};


class rt_graphics : public t_rt_output
{
    Q_OBJECT

private:

    rt_window *m_winGraph;
    GLuint m_bufObject[RT_GR_OBJ_NUMBER];

private:
    void render();  //redraw scene

public slots:
    void start();
    void pause();
    void process();
    void change();

public:

    rt_graphics(QObject *parent = 0):
        m_winGraph(new rt_window(parent)){

        glGenBuffers(RT_GR_OBJ_NUMBER, m_bufObject);   // Create vertex buffer objects
        change();
    }
};


#endif // RT_GRAPHICS_H
