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

class rt_graphics : public t_rt_output, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    QWindow *m_win;
    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;

    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;

    QOpenGLShaderProgram *m_program;
    int m_frame;

private slots:

public slots:
    void start();
    void pause();
    void process();
    void change();

public:
    rt_graphics(QObject *parent = 0);
};


#endif // RT_GRAPHICS_H
