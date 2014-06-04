#include "rt_graphics.h"

#include "openglwindow.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

static const char *vertexShaderSource =
    "attribute highp vec4 posAttr;\n"
    "attribute lowp vec4 colAttr;\n"
    "varying lowp vec4 col;\n"
    "uniform highp mat4 matrix;\n"
    "void main() {\n"
    "   col = colAttr;\n"
    "   gl_Position = matrix * posAttr;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying lowp vec4 col;\n"
    "void main() {\n"
    "   gl_FragColor = col;\n"
    "}\n";


rt_graphics::rt_graphics(QWindow *parent)
    : win(new QWindows(parent))
    , m_context(0)
    , m_device(0)
{

    m_win->setSurfaceType(QWindow::OpenGLSurface);
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");

    change();
}

rt_graphics::timerEvent(QTimerEvent *event){

    Q_UNUSED(event);
    process();
}


rt_graphics::~rt_graphics()
{
    delete m_device;
}

void rt_graphics::change()
{
    if (!m_context) {

        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();
    }

    if (!m_device){

        m_device = new QOpenGLPaintDevice;
    }
}

void rt_graphics::process()
{
    if(!win || !win->isExposed())
        return;

    this->startTimer(set["Refresh"].get().toDouble());

    m_context->makeCurrent(this);
    m_device->setSize(size());

    {

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            const qreal retinaScale = devicePixelRatio();
            glViewport(0, 0, width() * retinaScale, height() * retinaScale);

            glClear(GL_COLOR_BUFFER_BIT);

            m_program->bind();

            QMatrix4x4 matrix;
            matrix.perspective(60, 4.0/3.0, 0.1, 100.0);
            matrix.translate(0, 0, -2);
            matrix.rotate(100.0f * m_frame / screen()->refreshRate(), 0, 1, 0);

            m_program->setUniformValue(m_matrixUniform, matrix);

            GLfloat vertices[] = {
                0.0f, 0.707f,
                -0.5f, -0.5f,
                0.5f, -0.5f
            };

            GLfloat colors[] = {
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f
            };

            glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
            glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);

            m_program->release();

            ++m_frame;
    }

    m_context->swapBuffers(this);
}




