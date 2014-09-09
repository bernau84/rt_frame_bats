#include "rt_window.h"

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


#define BUFFER_OFFSET(i) ((char *)NULL + (i))

rt_window::rt_window(QObject *parent):
    QWindow(parent),
    m_context(0),
    m_device(0)
{
    initializeOpenGLFunctions();

    setSurfaceType(QWindow::OpenGLSurface);

    m_device = new QOpenGLPaintDevice;  //nejsem si jist jestli je vubec potreba

    m_context = new QOpenGLContext(this);
    m_context->create();
    m_context->makeCurrent(this);

    QSurfaceFormat format;
    format.setSamples(16);
    m_context->setFormat(format);

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    m_posAttr = m_program->attributeLocation("posAttr");
    m_colAttr = m_program->attributeLocation("colAttr");
    m_matrixUniform = m_program->uniformLocation("matrix");

    m_matrix.perspective(60, 4.0/3.0, 0.1, 100.0);
    m_matrix.translate(0, 0, -2);

    m_fp = QPoint();
}

/*! brief Rotate scene in direction of mouse movement
 * in angle corresponding to speed of move */
void rt_window::mouseMoveEvent(QMouseEvent *ev){

    if(m_fp.isNull() == true)
        return;

    QPoint p = ev->pos() - m_fp;
    m_matrix.rotate(360 * p.manhattanLength() / width(), 0, p.y()/height(), p.x()/height());

    m_fp = ev->pos();
}

void rt_window::mousePressEvent(QMouseEvent *ev){

    m_fp = ev->pos();
}

void rt_window::mouseReleaseEvent(QMouseEvent *ev){

    m_fp = QPoint();
}

bool rt_window::event(QEvent *event){

    switch (event->type()) {
    case QEvent::UpdateRequest:
        render();
        return true;
    default:
        return QWindow::event(event);
    }
}

void rt_window::render(GLuint vbo[], int n){

    if(!win->isExposed())
        return;

    memcpy(m_vbo, vbo, n);
    render();
}

void rt_window::render(){

    m_context->makeCurrent(this);
    m_device->setSize(size());

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_program->bind();
    m_program->setUniformValue(m_matrixUniform, matrix);

    if(0 == vbo[RT_GR_OBJ_VERTEXBUF])
        return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo[RT_GR_OBJ_VERTEXBUF]);
    glVertexAttribPointer(m_posAttr, 3, GL_FLOAT, GL_FALSE, 3*3*4, 0/*graph*/);
    glEnableVertexAttribArray(m_posAttr);

    if(0 == vbo[RT_GR_OBJ_TIMESL]){

        glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 3*3*4, BUFFER_OFFSET(12));
        glEnableVertexAttribArray(m_colAttr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[RT_GR_OBJ_TIMESL]);
        glDrawElements(GL_LINES, (N-1) * N * 2, GL_UNSIGNED_SHORT, 0);

        glDisableVertexAttribArray(m_colAttr);
    }

    if(0 == vbo[RT_GR_OBJ_FREQSL]){

        glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 3*3*4, BUFFER_OFFSET(24));
        glEnableVertexAttribArray(m_colAttr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[RT_GR_OBJ_FREQSL]);
        glDrawElements(GL_LINES, (N-1) * N * 2, GL_UNSIGNED_SHORT, 0);

        glDisableVertexAttribArray(m_colAttr);
    }

    if(0 == vbo[RT_GR_OBJ_TRISURF]){

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[RT_GR_OBJ_TRISURF]);
        glDrawElements(GL_TRIANGLES, (N-1) * (N-1) * 6, GL_UNSIGNED_SHORT, 0);
    }

    glDisableVertexAttribArray(m_posAttr);
    glDisableVertexAttribArray(m_colAttr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_context->swapBuffers(m_win);
}
