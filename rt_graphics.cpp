#include "rt_graphics.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

//#define DEMO 1

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





void rt_graphics::change()
{
    QJsonValue ts = set["Type"].get();
    if(ts.isArray()) m_selected = ts.toArray();  //vyber multigraph
    else if(ts.isDouble()) m_selected.append(ts.toDouble());  //one selection

    M = N = 0;
    t_slcircbuf *l_mb = dynamic_cast<t_slcircbuf *>(parent());  //local ptr
    for(t_rt_slice l1_amp, l2_amp; l1_amp.t <= l2_amp.t; l_mb->get(&l1_amp, rd_i)){

        l2_amp.t = l1_amp.t;
        l_mb->readShift(rd_i);
        if(M < l2_amp.v.size()) M = l2_amp.v.size();
        N += 1;
    }

    int s_size = 0, l_size = M*N*sizeof(GLfloat);  //number of points
    RT_GR_OBJ_VERTEXBUF = 0,    /*!< 0 - vertex buffer */
    RT_GR_OBJ_TIMESL,       /*!< 1 - vertical slices indexes */
    RT_GR_OBJ_FREQSL,       /*!< 2 - horizontal slices */
    RT_GR_OBJ_TRISURF,      /*!< 3 - surface triangulation */
    RT_GR_OBJ_AXES,       /*!< 4 - axis and fractions */
    RT_GR_OBJ_LABELS,       /*!< 5 - axis and fractions */
    RT_GR_OBJ_GRAD,       /*!< 6 - color gradient sub */
    RT_GR_OBJ_NUMBER
    if(m_selected.contains(RT_GR_OBJ_VERTEXBUF)){

       s_size += l_size * 3; //3 axes
       s_size += l_size * 3;    //2 colors per pixel
    }
    if(m_selected.contains(RT_GR_OBJ_TIMESL)){

       s_size += l_size * 3; //for line color
    }
    m_bufRaw = realloc(m_bufRaw, M*N*sizeof(GLfloat));
}


void rt_graphics::process()
{
#ifndef DEMO

    /*! \todo - if and only if the time from last refresh exceed the
     * selected refresh rate
     * this->startTimer(set["Refresh"].get().toDouble());
     */

    t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
    if(!pre) return; //navazujem na zdroj dat?

    if(sta.state != t_rt_status::ActiveState){

        int n_dummy = pre->t_slcircbuf::readSpace(rd_i); //zahodime data
        pre->t_slcircbuf::readShift(n_dummy, rd_i);
        return;  //bezime?
    }



    //zjistiti pocet prvku a rezervovat odpovidajici pamet (3x souradnice, 3x barva)
    //!pozor nemusi jit o matici
    //      - to by pak mohl byt ale problem s plochou; asi bysme meli body patricne zmnozit
    //      - nebo jeste lepe znulovat (a nastavit patricne blbou barvu)
    //pokud je zadane vygenerovat indexy k x rezum
    //pokud je zadane vygenerovat indexy k y rezum (podle aktualniho poctu prvku ovsem)
    //velke todo - automaticke prizpusobeni delky os a popisky

    t_rt_slice p_slice;   //radek soucasneho spektra
    t_slcircbuf::get(&p_slice, 1);  //vyctem aktualni rez

    t_rt_slice t_amp;  //radek caovych dat
    while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //vyctem radek

        for(int i=0; i<t_amp.v.size(); i++){

            int dd, m  = (sta.nn_run % D);
            double t_filt = bank[m]->Process(t_amp.v[i].A, &dd);  //band pass & decimace

            sta.nn_run += 1;
            sta.nn_tot += 1;

            if((dd == 0) && (mask & (1 << m))){ //z filtru vypadl decimovany vzorek ktery chcem

                if(m & 0x1) //mirroring u lichych pasem
                    t_filt *= -1;

                p_shift.v.last().A += t_filt;  //scitame s prispevky od jinych filtru (pokud jsou vybrany)
                p_shift.v.last().f  = mask;
            }

            if(m == (D-1)){  //mame hotovo (decimace D) vzorek muze jit ven

                p_shift.v = t_rt_slice::t_rt_tf(sta.nn_tot / *sta.fs_in); //pripravim novy

                if((p_shift.v.size()) == R) {

                    t_slcircbuf::write(p_shift); //zapisem novy jeden radek
                    t_slcircbuf::read(&p_shift, 1);
                    p_shift = t_rt_slice(0, 1); //inicializace noveho spektra
                }
            }
        }
    }

    t_slcircbuf::set(&p_shift, 1);  //vratime aktualni cpb spektrum
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




