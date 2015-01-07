#ifndef RT_GRAPH_OBJECT_H
#define RT_GRAPH_OBJECT_H

#include <QVector>
#include <QtGui/QOpenGLFunctions>

class rt_graph_object;
class rt_graph_context;


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


class rt_graph_object : public QMatrix4x4 {  //transformation matrix integral part of object

public:
    GLuint m_ver; //vertices vbo
    GLuint m_ind; //indices vbo
    GLuint m_col; //colors vbo
    GLuint m_size; //number of indices

public:

    /*! \brief - according to definition render object to context
     * default attribute definition
     * special future object can process attribute differentialy
     */
    virtual void render(rt_graph_context *context){

        context->glBindBuffer(GL_ARRAY_BUFFER, m_ver);
        context->glVertexAttribPointer(0, sizeof(t_rt_graph_v),
                                         GL_FLOAT, GL_FALSE,
                                         sizeof(t_rt_graph_v), 0);
        context->glEnableVertexAttribArray(0); //m_posAttr

        context->glBindBuffer(GL_ARRAY_BUFFER, m_col);
        context->glVertexAttribPointer(1, sizeof(t_rt_graph_c),
                                         GL_FLOAT, GL_FALSE,
                                         sizeof(t_rt_graph_c), 0);
        context->glEnableVertexAttribArray(1);  //m_colAttr

        context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ind);
    }

    rt_graph_object(){

        m_ver = m_col = m_ind = GL_INVALID_VALUE;
        m_size = 0;
    }

    rt_graph_object(GLuint ver, GLuint col, GLuint ind, GLuint size){

        m_ver = ver;
        m_col = col;
        m_ind = ind;
        m_size = size;
    }

    virtual ~rt_graph_object(){;}
};

class rt_graph_points : public rt_graph_object {

    rt_graph_points() :
        rt_graph_object(POINTS){

    }

    virtual render(rt_graph_context *context){

        rt_graph_object::render(context);
        /*! render points */
        context->glDrawElements(GL_POINTS, size, GL_UNSIGNED_SHORT, 0);

    }

    virtual ~rt_graph_points(){;}
};

class rt_graph_lines : public rt_graph_object {

    rt_graph_lines() :
        rt_graph_object(MULTILINE){

    }

    virtual render(rt_graph_context *context){

        rt_graph_object::render(context);
        /*! render multiline */
        context->glDrawElements(GL_LINES, size, GL_UNSIGNED_SHORT, 0);
    }

    virtual ~rt_graph_lines(){;}
};

class rt_graph_surfaces : public rt_graph_object {

    rt_graph_surfaces() :
        rt_graph_object(TRISURFACE){

    }

    virtual render(rt_graph_context *context){

        rt_graph_object::render(context);
        /*! render triangles */
        context->glDrawElements(GL_TRIANGLES_STRIP, size, GL_UNSIGNED_SHORT, 0);
    }

    virtual ~rt_graph_surfaces(){;}
};

#endif // RT_GRAPH_OBJECT_H
