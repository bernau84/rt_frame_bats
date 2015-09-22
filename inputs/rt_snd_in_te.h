#ifndef RT_SND_IN
#define RT_SND_IN

#define RT_SND_IN_SIMUL_F   1000

#include <stdint.h>
#include <math.h>

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

#ifdef RT_SND_IN_SIMUL_F
        //T dbg = 0.8 * sin(RT_SND_IN_SIMUL_F * (2*M_PI*nproc) / fs);
        T dbg = nproc;
        sample = &dbg;
#endif //RT_SND_IN_SIMUL_F

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

        signal(RT_SIG_SOURCE_UPDATED, "something"); //inform sucessor and may be the sampler as well
    }

    t_rt_snd_in_te(const t_setup_entry &freq, const std::string &conf):
        i_rt_base(conf, ":/config/js_config_sndsource.txt", RT_QUEUED),
        buf(NULL)
    {
        par.replace("Rates", freq);  //update list
        nproc = 0;
        change();
    }

    virtual ~t_rt_snd_in_te(){

        if(buf) delete buf;
    }
};

#endif // RT_SND_IN

