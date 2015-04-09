#ifndef RT_SND_IN
#define RT_SND_IN

#include <stdint.h>

#include "base\rt_base.h"

template<typename T> class t_rt_snd_in_te : public i_rt_base {

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

        row.append(*(T *)sample, 2.0/fs); //sample an its freq resolution = nyquist
        nproc += 1;

        if(row.isfull()){

            buf->write(&row); //write
            row = t_rt_slice<T>(nproc / fs, M, (T)0); //and prepare new with current timestamp
        }
    }

    /*! \brief reinit buffers and local properties according to actual setting */
    virtual void change(){

        N = par["Multibuffer"].get().toDouble();
        M = par["Refresh"].get().toDouble();  //refresh ratein ms
        fs = par["Sampling"].get().toDouble();  //actual frequency
        M *= (fs  / 1000.0); //refresh rate in samples number
        buf->resize(N); //novy vnitrni multibuffer

        //optionaly
        row = t_rt_slice<T>(nproc/fs, M, (T)0);  //recent slice reset
        //nproc = 0;
    }

    t_rt_snd_in_te(const t_setup_entry &freq, const QDir &resource = QDir(":/config/js_config_sndsource.txt")):
        i_rt_base(resource)
    {
        par.replace("Rates", freq);  //update list
        change();
    }

    virtual ~t_rt_snd_in_te(){
        //empty
    }
};

#endif // RT_SND_IN

