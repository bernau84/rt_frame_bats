#ifndef RT_GRAPH_CONTEXT_H
#define RT_GRAPH_CONTEXT_H

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QScreen>


/*!
 * \brief The rt_graph_context class
 * jednotnyu kontext pro vicero framebufferu
 * sdruzuje ogl kontex a fyzicke okno pro vykreslovani
 */
class rt_graph_context : public QWindow, protected QOpenGLFunctions
{
private:
    QOpenGLShaderProgram *m_program;
    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;

    QPoint  m_fp;  //coord mouse press down
    QWindow *m_win; //system window for output

    virtual void mouseMoveEvent(QMouseEvent *ev);
    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void mouseReleaseEvent(QMouseEvent *ev);
    virtual bool event(QEvent *event);

public:
    rt_graph_context();
};

#endif // RT_GRAPH_CONTEXT_H
