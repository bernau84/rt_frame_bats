#ifndef RT_BASE_SLBUF_EX
#define RT_BASE_SLBUF_EX

#include "rt_base.h"

/*! \brief encapsulation of base and multibuffer classes
 * simplified manipulation with buffer and save/general
 * on_update(void *) implementation
 *
 * inherited class
 */
template <typename T> class i_rt_base_slbuf_ex : public virtual i_rt_base
{
protected:
    rt_idf_circ_simo<t_rt_slice<T> > *buf;  /*! output buffer */
    using i_rt_base::par;

public:
    virtual const void *read(int n){ return (buf) ? buf->read(n) : NULL; }
    virtual const void *get(int n){ return (buf) ? buf->get(n) : NULL;  }
    virtual int readSpace(int n){ return (buf) ? buf->readSpace(n) : -1; }

    i_rt_base_slbuf_ex(const QDir resource, e_rt_regime mode = RT_QUEUED):
            i_rt_base(resource, mode),
            buf(NULL)
    {
    }

    virtual ~i_rt_base_slbuf_ex(){

       if(buf) delete buf;
       buf = NULL;
    }

    void buf_resize(int n){

       if(buf) delete(buf);
       buf = NULL;

       buf = (rt_idf_circ_simo<t_rt_slice<T> > *) new rt_idf_circ_simo<t_rt_slice<T> >(n);
    }

    void buf_append(t_rt_slice<T> &smp){

       if(buf) i_rt_base::signal(RT_SIG_SOURCE_UPDATED, buf->write(&smp));  //write & signal with global pointer into buffer
    }

    virtual void update(t_rt_slice<T> &sample) = 0;

    /*! \brief common action for  */
    virtual void update(const void *sample){

        //can we process all samples at once - and from oldes?
        if((reader_i >= 0) && (source != NULL)){

            while(source->readSpace(reader_i)){

                t_rt_slice<T> *out = (t_rt_slice<T> *)source->read(reader_i);
                if(out == NULL) return;
                update(*out);
            }

            return;
        }

        //convert & test sample; we uses pointer instead of references because reference can be tested
        //only with slow try catch block which would be cover all processing loop
        t_rt_slice<T> *out = (t_rt_slice<T> *)sample;
        if(out == NULL) return;
        update(*out);
    }

};

#endif // RT_BASE_SLBUF_EX

