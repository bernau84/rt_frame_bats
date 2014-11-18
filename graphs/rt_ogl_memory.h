#ifndef RT_GRAPH_MEMORY_H
#define RT_GRAPH_MEMORY_H

#include <QList>

#define RT_GRAPH_ALLOC_DEFSZ    2000000

/*!
 * \brief The rt_graph_memory class
 * \todo reallocation - application gets only reference, not pointers directly
 */
class rt_graph_memory {

private:
    typedef struct {
        void *buf;
        quint32 chunk;
        bool reserved;
    } t_graph_mem_chunk;

    QList<t_graph_mem_chunk> parts;

public:

    /*!
     * \brief return memory addres
     */
    const void *base(){

        return parts[0].buf;
    }

    /*!
     * \brief return memory size
     */
    quint32 size(){

        quint size = 0;
        foreach(t_graph_mem_chunk i, parts)
            size += chunk;

        return size;
    }

    /*!
     * \brief alloc memory
     * \param nfloats - number of float world to reserve
     * \return NULL if we no space left for nfloat
     */
    GLfloat *alloc(quint nfloats){

        nfloats *= sizeof(GLfloat);
        int bi = -1;
        foreach(t_graph_mem_chunk i, parts){
            if(!i.reserved)
                if((i.chunk - nfloats) > parts[bi].chunk)
                    bi = parts.indexOf(i);
        }

        if(bi < 0)
            return NULL;

        GLfloat *rb = parts[bi].buf; //remember
        parts[bi].chunk -= nfloat;

        parts.insert(bi, t_graph_mem_chunk(parts[bi].buf, nfloat));
        return rb;
    }

    /*!
     * \brief free part of previously reserved memory
     * \param p - pointer to unused memory
     * \return 0 if p wasn't find in alloc table, otherwise number of freed floats
     */
    quint32 free(GLfloat *p){

        int bi = -1;
        foreach(t_graph_mem_chunk i, parts){
            if((i.buf == p) && (i.reserved))
                bi = parts.indexOf(i);
        }

        if(bi < 0)
            return 0;

        int ret = parts[bi].chunk;
        parts[bi].reserved = 0;

        //merge with previous or successive
        if((bi > 0) && (!parts[bi-1].reserved)){

            parts[bi-1].chunk += parts[bi].chunk;
            parts.erase(bi);
        } else if((bi < (parts.size()-1)) && (!parts[bi+1].reserved)){

            parts[bi+1].chunk += parts[bi].chunk;
            parts.erase(bi);
        }

        return ret;
    }

    rt_graph_memory(qint32 size = RT_GRAPH_ALLOC_DEFSZ){

        /*! \todo exception handling - rewrite to new*/
        parts << t_graph_mem_chunk(malloc(size), size);
    }

    ~rt_graph_memory(){

        if(parts.size())
            if(parts[0].buf)
                free(buf);

        buf = NULL;
        parts.clear();
    }
};

#endif // RT_GRAPH_MEMORY_H
