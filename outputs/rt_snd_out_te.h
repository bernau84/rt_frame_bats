#ifndef RT_SND_OUT_TE
#define RT_SND_OUT_TE

#include <stdint.h>
#include <math.h>

#include "base\rt_base.h"

template<typename T> class t_rt_snd_out_te : public i_rt_base {

protected:
    //local properties
    int M;      //refresh rate in number of samples in one row
    int N;      //multibuffer length
    double fs;  //sampling freq
    uint64_t nproc; //number of proceed samples

    t_rt_slice<T> row;                     /*! active row in multibuffer */
    rt_idf_circ_simo<t_rt_slice<T> > *buf;  /*! spectrum buffer */

public:
    virtual const void *read(int n){ return (buf) ? buf->read(n) : NULL; }
    virtual const void *get(int n){ return (buf) ? buf->get(n) : NULL;  }
    virtual int readSpace(int n){ return (buf) ? buf->readSpace(n) : -1; }

    /*! \brief format & copy samples into the multibuffer */
    virtual void update(const void *sample){

        if(!buf || !sample)
            return;

        row.append(*(T *)sample, fs/2.0); //sample an its freq resolution = nyquist
        nproc += 1;

        if(row.isfull()){

            signal(RT_SIG_SOURCE_UPDATED, buf->write(&row)); //write & signal with global pointer inside buffer
            row = t_rt_slice<T>(nproc / fs, M, (T)0); //and prepare new with current timestamp
        }
    }

    /*! \brief reinit buffers and local properties according to actual setting */
    virtual void change(){

        fs = par["Rates"].get().toDouble();  //actual frequency
        N = par["Multibuffer"].get().toDouble(); //slice number
        M = fs * par["Time"].get().toDouble();  //[Hz] * refresh rate [s] = slice point

        if(buf) delete buf;
        buf = (rt_idf_circ_simo<t_rt_slice<T> > *) new rt_idf_circ_simo<t_rt_slice<T> >(N);

        //optionaly
        row = t_rt_slice<T>(nproc/fs, M, (T)0);  //recent slice reset
        //nproc = 0;

    }

    t_rt_snd_out_te(const t_setup_entry &freq, const QDir &resource = QDir(":/config/js_config_sndsink.txt")):
        i_rt_base(resource, RT_QUEUED),
        buf(NULL)
    {
        par.replace("Rates", freq);  //update list
        nproc = 0;
        change();

        subscribe(this); //reserve fist reader for itself
    }

    virtual ~t_rt_snd_out_te(){

        if(buf) delete buf;
    }
};

#endif // RT_SND_OUT_TE

