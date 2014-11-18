#ifndef RT_WINDOW_H
#define RT_WINDOW_H

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>


#define RT_OGL_ALLOCATOR_MEM    0 //o - if by defaultunused, 2000000 by default

class rt_graph_object;
class rt_graph_context;
class rt_graph_memory;

//QList<const rt_graph_object *> empty_rt_graph_object_List;

enum e_rt_graph_obj_type {

    POINTS = 0,     //points are just points
    MULTILINE,      //points are peaks of line
    TRISURFACE      //points are triangle interpolation of surface
};

enum e_rt_graph_object_item {

    RT_OBJ_VERTEXBUF = 0,    /*!< 0 - vertex buffer */
    RT_OBJ_INDEXBUF_1 = 1,
    RT_OBJ_INDEXBUF_2 = 2,
    /* and so */
    RT_OBJ_NUMBER = 1000
};

#define RT_VBO_NUMEBR RT_OBJ_NUMBER  //despite it can be independant from OBJ number

/*!
 * \brief The rt_graph_frame class
 * factory patter & smart graph object and vbo sharing
 */

class rt_graph_frame {

private:
    rt_graph_context *m_context;  //ogl context + qwindow

    QMatrix4x4 m_matrix;  //perspective matrix
    GLuint m_posAttr;
    GLuint m_colAttr;
    GLuint m_matrixUniform;

    rt_graph_memory m_allocator;  //optional - ogl memory reservation and provider
    GLuint m_framebuffer; //optional - for future use
    GLuint m_vao;   //optional - for future use

    GLuint              m_vbo[RT_VBO_NUMBER];  //vbo stack all for vertices, colors, indices
    rt_graph_object    *m_rto[RT_OBJ_NUMBER];  //framebuffer objects data

    //new object is not copy into gpu memory unless it is not already
    //in cache. use of global memory is assumed!
    //only for floats (vertexes and colors)

    struct t_cache { //cache assesment between vbo and raw memory

        const GLfloat *base;  //remember origin buffer to provide data sharing
        GLuint sz;  //number of bytes / indices; also indicitor of used vbo!
        QList<const rt_graph_object *> ref;  //list of grpah object refers to this vbo
    } m_mem[RT_VBO_NUMBER];

    QList<const rt_graph_object *> empty_rt_graph_object_List;  //helper
    t_cache empty_t_cache;  //helper

    /*! \todo - consider use of vao */

public slots:
    //called from window gui
    void update_orientation(QPoint angles){

        //recount m_matrix
    }

    void update_button(QPoint coords, quint32 id, bool down_up){

        //special handling - zoom, focus on part. object, ...
    }

public slots:

    /*! \brief update vertex buffer with original buffer
     * selected object or all for NULL input
     */
    void update(const rt_graph_object *o = NULL){

        for(int i=0; i<RT_VBO_NUMBER; i++)  //update all cached vbo from original buffers
            if(m_mem[i].base && ((o == NULL) || (m_mem[i].ref.contains(o)))){

                m_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo[i]);
                m_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                        m_mem[i].sz,
                                        m_mem[i].base,
                                        GL_STATIC_DRAW);
            }
    }

    /*! \brief render all defined objects
     */
    void render(void){

        /*! \todo bind framebuffer
        */
        for(int i=0; i<RT_OBJ_NUMBER; i++)  //render all object in frame
            if(m_rto[i])
                m_rto[i]->render(m_context);
    }

private:
    /*! \brief annonce future memory requirements
    */
    GLfloat *alloc(quint nfloats){

        return m_allocator.alloc(nfloats);
    }

    /*! \brief free unused mem
    */
    quint32 free(GLfloat *p){

        return m_allocator.free(p);
    }

    /*! \brief get first unused vbo index - that is for vbo default GL_INVALID_VALUE
     * if vbo is set than return index of vbo record
     */
    qint32 get_vbo_i(GLuint vbo = GL_INVALID_VALUE){

        for(int n=0; n<RT_VBO_NUMBER; n++)
            if(m_vbo[n] == vbo)
                return n;

        return -1;
    }


public:

    /*! \brief define particular graph object, return id
    */
    rt_graph_object *add(e_rt_graph_obj_type ty,
                         GLuint ver, GLuint col, GLuint ind, quint32 size){ //VBOs

        for(quint32 oi = 0; oi < RT_OBJ_NUMBER; oi += 1)
            if(m_rto[oi] == NULL){

                switch(ty){

                    case e_rt_graph_obj_type::POINTS:
                        m_rto[oi] = new rt_graph_points(ver, col, ind, size);
                        break;
                    case e_rt_graph_obj_type::MULTILINE:
                        m_rto[oi] = new rt_graph_lines(ver, col, ind, size);
                        break;
                    case e_rt_graph_obj_type::TRISURFACE:
                        m_rto[oi] = new rt_graph_surface(ver, col, ind, size);
                        break;
                }

                m_mem[get_vbo_i(ver)].ref << m_rto[oi];
                m_mem[get_vbo_i(col)].ref << m_rto[oi];
                m_mem[get_vbo_i(ind)].ref << m_rto[oi];
                return m_rto[oi];
             }

        return NULL;
    }

    /*! \brief delete particular graph object and empty associated vbo
    */
    bool del(const rt_graph_object *o){

        for(quint oi = 0; oi < RT_OBJ_NUMBER; oi += 1)
            if(m_rto[oi] == o){

                delete m_rto[fpos];
                m_rto[fpos] = NULL;

                for(int vi = 0; vi < RT_VBO_NUMBER; vi += 1)
                    if(m_mem[vi].ref.contains(o)){

                        m_mem[vi].ref.removeOne(o);
                        if(m_mem[vi].ref.isEmpty())  //with last reference empty the cache record
                            m_mem[vi] = empty_t_cache;
                    }

                return true;
            }

        return false;
    }

    /*! \brief creates new object; if no free vbo than return NULL
    */
    rt_graph_object *from(e_rt_graph_obj_type ty,
                          QPair<const t_rt_graph_v *, quint32> &ver,
                          QPair<const t_rt_graph_c *, quint32> &col,
                          QVector<GLuint> &ind){

        GLuint vbo_ver, vbo_col, vbo_ind;
        vbo_ver = vbo_col = vbo_ind = GL_INVALID_VALUE;

#if defined (RT_OGL_ALLOCATOR_MEM == 0)

        for(int n=0; n<RT_VBO_NUMBER; n++){

            if(ver.first == mem[n].base)
                vbo_ver = m_vbo[n];

            if(col.first == mem[n].base)
                vbo_col = m_vbo[n];

            if((vbo_ver != GL_INVALID_VALUE) &&
                    (vbo_col != GL_INVALID_VALUE))
                break;
        }
#else
        /*! \todo - deep copy vertices and colors into memory from allocator */
        return NULL;
#endif //(RT_OGL_ALLOCATOR_MEM == 0)

        qint32 vbo_i;

        if((ver.second == 0) || ((vbo_i = get_vbo_i(vbo_ver)) < 0))
            return NULL;

        vbo_ver = m_vbo[vbo_i];  //init / update vbo buffer
        m_context->glBindBuffer(GL_ARRAY_BUFFER, vbo_ver);
        m_context->glBufferData(GL_ARRAY_BUFFER, ver.second, ver.first, GL_STATIC_DRAW);

        m_mem[vbo_i].base = ver.first;
        m_mem[vbo_i].sz = ver.second;

        if((col.second == 0) || ((vbo_i = get_vbo_i(vbo_col)) < 0))
            return NULL;

        vbo_col = m_vbo[vbo_i];  //init / update vbo buffer
        m_context->glBindBuffer(GL_ARRAY_BUFFER, vbo_col);
        m_context->glBufferData(GL_ARRAY_BUFFER, col.second, col.first, GL_STATIC_DRAW);

        m_mem[vbo_i].base = col.first;
        m_mem[vbo_i].sz = col.second;

        if((indices.size() == 0) || ((vbo_i = get_vbo_i(vbo_ind)) < 0))
            return NULL;

        vbo_ind = m_vbo[vbo_i];
        m_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_ind);
        m_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                ind.size(), ind.buffer(),
                                GL_STATIC_DRAW);

        m_mem[vbo_i].base = NULL; //not need; only for sharing purpose
        m_mem[vbo_i].sz = ind.size(); //number of vertices (not bytes)

        return add(ty, vbo_ver, vbo_col, vbo_ind, ind.size());
    }

    /*! \brief constructor & initializer
     */
    rt_frame(rt_graph_context *context):
        m_context(context),
        m_allocator(RT_OGL_ALLOCATOR_MEM)
    {

        empty_t_cache.base = NULL;
        empty_t_cache.sz = 0;
        empty_t_cache.ref = empty_rt_graph_object_List;

        for(int n=0; n<RT_VBO_NUMBER; n++){

            m_mem[n] = empty_t_cache;
            m_vbo[n] = GL_INVALID_VALUE;
        }

        for(int n=0; n<RT_OBJ_NUMBER; n++){

            m_rto[n] = NULL;
        }

        m_context->glGenBuffers(RT_VBO_NUMBER, m_vbo);
    }

    /*! \brief destructor & dealocate
     */
    virtual ~rt_frame(){

        for(quint oi = 0; oi < RT_OBJ_NUMBER; oi += 1)
            if(m_rto[oi]){

                delete m_rto[oi];
                m_rto[oi] = NULL;
            }
    }
};


#endif // RT_WINDOW_H
