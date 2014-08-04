#include "rt_graphics.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

#define DEMO 1

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

void rt_graphics::mouseMoveEvent(QMouseEvent *ev){

    if(m_fp.isNull() == true)
        return;

    QPoint p = ev->pos() - m_fp;
    m_matrix.rotate(360 * p.manhattanLength() / width(), 0, p.y()/height(), p.x()/height());
}

void rt_graphics::mousePressEvent(QMouseEvent *ev){

    m_fp = ev->pos();
}

void rt_graphics::mouseReleaseEvent(QMouseEvent *ev){

    m_fp = QPoint();
}

void rt_window::render(GLuint vbo[], int n){

    if(!win->isExposed())
        return;

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

    if(0 == vbo[RT_GR_OBJ_TIMESL]){

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





void rt_graphics::timerEvent(QTimerEvent *event){

    Q_UNUSED(event);
    render(m_bufObject, RT_GR_OBJ_NUMBER);
}


rt_graphics::~rt_graphics()
{
}

void rt_graphics::change()
{
    this->startTimer(set["Refresh"].get().toDouble());
}



void rt_graphics::process()
{
#ifndef DEMO
    int N = set["Multibuffer"].get().toDouble();
#endif //DEMO

#if DEMO
    #define N 32
    GLfloat graph[N][N][3*3];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float x = (i - N / 2) / (N / 2.0);
            float y = (j - N / 2) / (N / 2.0);
            float d = hypotf(x, y) * 4.0;
            float z = (1 - d * d) * expf(d * d / -2.0);

            graph[i][j][0] = (i - N/2)/(N * 1.0);
            graph[i][j][1] = (j - N/2)/(N * 1.0);
            graph[i][j][2] = roundf(z * N/2 + N/2)/(N * 1.0);

            graph[i][j][3] = 0.0;  //for vertical
            graph[i][j][4] = 0.0;
            graph[i][j][5] = graph[i][j][2]-0.3;

            graph[i][j][6] = 0.0;    //for horizontal lines
            graph[i][j][7] = graph[i][j][2]-0.3;
            graph[i][j][8] = 0.0;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_bufObject[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof graph, graph, GL_STATIC_DRAW);

    // Create an array of indices into the vertex array that traces both horizontal and vertical lines
    GLushort ilines_h[(N-1) * N * 2];
    int i = 0;

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N-1; x++) {
            ilines_h[i++] = y * N + x;
            ilines_h[i++] = y * N + x + 1;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufObject[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof ilines_h, ilines_h, GL_STATIC_DRAW);

    GLushort ilines_v[(N-1) * N * 2];
    i = 0;
    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N-1; y++) {
            ilines_v[i++] = y * N + x;
            ilines_v[i++] = (y + 1) * N + x;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufObject[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof ilines_v, ilines_v, GL_STATIC_DRAW);

    GLushort itri[(N-1) * (N-1) * 6];
    i = 0;

    for (int x = 0; x < N-1; x++) {
        for (int y = 0; y < N-1; y++) {

            itri[i++] = y * N + x;
            itri[i++] = (y + 1) * N + x;
            itri[i++] = (y + 1) * N + x + 1;

            itri[i++] = y * N + x;
            itri[i++] = y * N + x + 1;
            itri[i++] = (y + 1) * N + x + 1;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufObject[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof itri, itri, GL_STATIC_DRAW);
#endif //DEMO

    render(m_bufObject, RT_GR_OBJ_NUMBER);
}




