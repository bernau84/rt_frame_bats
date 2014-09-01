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

class rt_window;


enum e_rt_graph_object_item {

    RT_GR_OBJ_VERTEXBUF = 0,    /*!< 0 - vertex buffer */
    RT_GR_OBJ_TIMESL,       /*!< 1 - vertical slices indexes */
    RT_GR_OBJ_FREQSL,       /*!< 2 - horizontal slices */
    RT_GR_OBJ_TRISURF,      /*!< 3 - surface triangulation */
    RT_GR_OBJ_AXES,       /*!< 4 - axis and fractions */
    RT_GR_OBJ_LABELS,       /*!< 5 - axis and fractions */
    RT_GR_OBJ_GRAD,       /*!< 6 - color gradient sub */
    RT_GR_OBJ_NUMBER
};

typedef rt_graph_clr_map QList<QPair<QColor, QColor> >;

class rt_graph_clr_gradient {

private:

    e_rt_graph_clr_interp type;

    /*! \brief - table with pair of input color and desired color 
     * gradient 
     */
    rt_graph_clr_map table;

    /*! \brief - return index in table
     * with lower nearest value to c in fist position on color 
     * channel ch (0 - R, 1 - G, B - 2)
     */
    int nearest(quint8 c, quint8 ch){

        for(int i=1; i<table.size(); i++)
            if(c > (table(i).first.rgba() >> (8*ch))) 
                return i-1;

        return table.size()-1;
    }

    quint8 interp(quint8 c, quint8 ch){

        int i = nearest(c, ch);

        switch(type){

            case SPLINE: /*! < \todo */
            case LINEAR: /*! < go to default step interp while color c i out of table bounds */
            {
                if(i < (table.size()-1)){

                    float Ax = table(i+0).first.rgba() >> (8*ch);
                    float Ay = table(i+0).second.rgba() >> (8*ch);
                    float Bx = table(i+1).first.rgba() >> (8*ch);
                    float By = table(i+1).second.rgba() >> (8*ch);

                    if(c >= Ax)
                        return (Ay + (Bx - c)*(By - Ay)/(Bx - Ax + 0.001));
                }
            }

            case STEP:
            defaut:

                return table(i).second.rgba() >> (8*ch);
            break;    
        }
    }

public:

    enum e_rt_graph_clr_interp {

        STEP,
        LINEAR,
        SPLINE
    } type;

    /*! \brief - for complex color transformation when each color channel is independant of other
     * for example amplitude is brightness and angle hue, and so...
     */
    QColor convert(QColor &x){

        QColor c;
        c.setRed(interp(x.getRed(), 0));
        c.setGreen(interp(x.getGreen(), 1));
        c.setBlue(interp(x.getBlue(), 2));
        return c;
    }

    /*! \brief - when output color is function ony of one value x
     * (amplitude for example)
     */
    QColor convert(qreal x){

        QColor c;
        c.setRgbF(x, x, x, 1.0);
        return convert(c);
    }    

    /*! \brief - costructor request colormap in pair of input and adjacent output color
     * interpolation in-between mapped color is also optional
     */
    rt_graph_clr_gradient(rt_graph_clr_map ta, e_rt_graph_clr_interp ty = STEP):
        table(ta), type(ty){

    }

    ~rt_graph_clr_gradient();

};

typedef struct {
    
    GLfloat x;
    GLfloat y;
    GLfloat z;    
} t_rt_graph_v __attribute__ ((packed));


typedef struct {

    GLfloat r;
    GLfloat g;
    GLfloat b; 
    GLfloat a; 
} t_rt_graph_c __attribute__ ((packed));

class rt_graph_object {

private:
    friend class rt_window;

    enum e_rt_graph_obj_type {

        POINTS = 0,     //points are just points
        MULTILINE,      //points are peaks of line
        TRISURFACE      //points are triangle interpolation of surface
    } type;

    t_rt_graph_v    *vertices;  //reference to global memory bindend in graphic card
    t_rt_graph_c    *colors;    //reference to global memory bindend in graphic card
    QVector<GLuint> indices;   //deep copy is required!

public:

    void *fromVertices(QVector<t_rt_graph_v> &ver, QVector<t_rt_graph_c> &col){

        if(ver.size())
            vertices = ver.data();

        if(col.size())
            colors = col.data();

        return vertices;
    }

    void *fromIndices(t_rt_graph_v *offs, QVector<GLuint> &ind, QVector<t_rt_graph_c> &col){

        if(NULL == offs)
            return NULL;

        vertices = offs;

        if(ind.size())
            indices = ind; //deep copy hopefully

        if(col.size())
            colors = col.data();

        return vertices;         
    }

    /*! \brief - according to definition render object to context
     * \todo - the queston is to place this directly into graph window object
     */
    void render(/*context....*/){

        if((vertices == NULL) || (colors == NULL))
            return;

        if(indices){
            //indirect render
        } else {
            //direct rendering
        }
    }

    rt_graph_object(e_rt_graph_obj_type ty) :
        type(ty){

        vertices = NULL;
        colors = NULL;
        indices = NULL;
    }
};

class rt_window : public QWindow, protected QOpenGLFunctions {

private:
    QOpenGLContext *m_context;
    QOpenGLPaintDevice *m_device;
    QOpenGLShaderProgram *m_program;

    QMatrix4x4 m_matrix;  //perspective matrix
    QPoint  m_fp;  //coord mouse press down

    GLfloat *m_bufRaw;
    GLuint m_bufFree;
    GLuint m_bufAlloc;

    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;
    GLuint m_vbo[RT_GR_OBJ_NUMBER];
    rt_graph_object m_rto[RT_GR_OBJ_NUMBER];

    virtual void mouseMoveEvent(QMouseEvent *ev);
    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void mouseReleaseEvent(QMouseEvent *ev);
    virtual bool event(QEvent *event);

public:
    /*! \brief render all defined objects
     */
    void render(void){

        //bind m_bufRaw

        for(int i=0; i<RT_GR_OBJ_NUMBER; i++){

            if(NULL == m_rto[i].vertices)
                continue;


        }
    }

    /*! \brief annonce future memory requirements */
    void reserv(int nfloats){

        m_bufAlloc += nfloats * sizeof(GLfloat);
    }

    /*! \brief allocate float memory */
    GLfloat *allocf(int bytes){

        if(m_bufRaw == NULL){

            m_bufFree = n_bufAlloc;
            m_bufBuf = (GLfloat *) malloc(n_bufAlloc);
        }

        if((m_bufRaw != NULL) && (m_bufFree > 0)){

            GLfloat *ret = m_bufRaw;  //backup

            //shift buffer
            int nfloats = (bytes + sizeof(GLfloat) - 1) / sizeof(GLfloat);
            m_bufRaw += nfloats;

            //downsize free bytes
            bytes = nfloats * sizeof(GLfloat);
            m_bufFree -= bytes;

            return ret;
        }

        return NULL;
    }

    /*! \brief define particular graph object
    */
    int set(e_rt_graph_object_item t, const rt_graph_object &obj){

        if((int)t < RT_GR_OBJ_NUMBER)
            return 0;

        m_rto[(int)t] = obj;
        return 1;
    }

    /*! \brief constructor & initializer
     */
    rt_window(QObject *parent){

        for(int n=0; n<RT_GR_OBJ_NUMBER; n++){

            m_vbo[n] = 0;
            m_rto[n] = NULL;
        }

        m_bufAlloc = 0;
    }

    /*! \brief destructor & dealocate
     */
    virtual ~rt_window(){

        if(m_bufRaw != NULL)
            free(m_bufRaw);

        m_bufRaw = NULL;

        if(m_device)
            delete m_device;
    }
};


class rt_axe : public rt_graph_object, public t_rt_base {

    /*! \todo
     * - trivial rt object counting axis autoscale & parts optimal + axis labeling
     * - configuration import
     * - autoscale / axis base handling
     * - export text and coord
     */
    double refresh;

public slots:
    void start(){
        /*! autoscale on */
        t_rt_base::start();
    }

    void pause(){
        /*! autroscale off, freeze actual scale */
        t_rt_base::pause();
    }

    void process(){
        /*! recalculate scale */
        t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
        if(!pre) return;

        if(sta.state != t_rt_status::ActiveState){

            int n_dummy = pre->t_slcircbuf::readSpace(rd_i); //throw data
            pre->t_slcircbuf::readShift(n_dummy, rd_i);
            return;
        }

        t_rt_slice t_scl;   //actual scale [loscale, hiscale]
        t_slcircbuf::get(&t_scl, 1);  //read out
        double min = t_scl.v[0];
        double max = t_scl.v[1];
        double rc = 2 / (sta.fs_out * refresh);

        t_rt_slice t_amp;  //amplitudes
        while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //read out

            for(int i=0; i<t_amp.size(); i++){

                sta.nn_run += 1;
                sta.nn_tot += 1;

                if(min > t_amp) min = t_amp.v[i];
                    else if(max < t_amp) max = t_amp.v[i];
            }

            t_scl.v[0] += (t_scl.v[0] - min)*rc;  //floating average
            t_scl.v[1] += (t_scl.v[1] - max)*rc;

            t_slcircbuf::write(&t_scl, 1);  // out
        }
    }

    void change(){

        refresh = set["Dynamic"].get().toDouble();

        sta.fs_out = refresh;

        t_rt_status::t_rt_a_sta pre_sta = sta.state;

        pause();

        this->resize(pre->size(), false);  //follow previous

        emit on_change(); //poslem dal

        switch(pre_sta){

            case t_rt_status::SuspendedState:
            case t_rt_status::StoppedState:
                break;

            case t_rt_status::ActiveState:

                start();
                break;
        }
    }
};

class rt_color_scale {

public:
    rt_window *m_canvas;
    rt_axe m_axe;

public slots:
    on_change()

    /*! \todo
     * - trully stupid object - only calculate
     * - 2d box with color scale with labeled axis
     * - another framebuffer needed
     * - need
     */
};

class rt_graphics : public t_rt_base
{
    Q_OBJECT

private:
    rt_window *m_canvas;

public slots:
    void start();
    void pause();
    void process();
    void change();

public:

    explicit rt_graphics(QObject *parent = 0, const QDir &resource = QDir()):
        t_rt_base(parent, resource),
        m_canvas(new rt_window(parent))
    {
        change();
    }

    virtual ~rt_graphics(){

        if(m_bufRaw) free(m_bufRaw);
    }
};


#endif // RT_GRAPHICS_H
