#ifndef RT_WINDOW_H
#define RT_WINDOW_H

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>

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

    /*! \todo - multi framebuffer */
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


#endif // RT_WINDOW_H
