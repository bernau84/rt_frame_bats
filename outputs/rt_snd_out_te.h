#ifndef RT_SND_OUT_TE
#define RT_SND_OUT_TE

#include <stdint.h>
#include <math.h>

#include "base\rt_base_slbuf_ex.h"

template<typename T> class t_rt_snd_out_te : public virtual i_rt_base_slbuf_ex<T> {

protected:
    //local properties
    int M;      //refresh rate in number of samples in one row
    int N;      //multibuffer length
    double fs;  //sampling freq
    uint64_t nproc; //number of proceed samples

    t_rt_slice<T> row;                     /*! active row in multibuffer */

    using i_rt_base_slbuf_ex<T>::par;
    using i_rt_base_slbuf_ex<T>::buf_resize;
    using i_rt_base_slbuf_ex<T>::buf_append;
    using i_rt_base_slbuf_ex<T>::subscribe;

public:
    /*! \brief format & copy samples into the multibuffer */
    virtual void update(t_rt_slice<T> &smp){

        for(unsigned i=0; i<smp.A.size(); i++){ //just copy to another buffer

            if(row.A.size() == 0){ //

                double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2
                row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                            t_rt_slice<T>(t, smp.A.size(), (T)0);  //auto-slice size (derived from input with respect to decimation)
            }

            row.append(smp.A[i], smp.I[i]); //D can reduce freq. resolution because of change fs
            if(row.isfull()){  //in auto mode (M == 0) with last sample of input slice

                buf_append(row); //write + send notifications
                row.A.clear(); //force new row initializacion
            }
        }
    }

    /*! \brief reinit buffers and local properties according to actual setting */
    virtual void change(){

        fs = par["Rates"].get().toDouble();  //actual frequency
        N = par["Multibuffer"].get().toDouble(); //slice number
        M = fs * par["Time"].get().toDouble();  //[Hz] * refresh rate [s] = slice point

        buf_resize(N);

        //optionaly
        row = t_rt_slice<T>(nproc/fs, M, (T)0);  //recent slice reset
    }

    t_rt_snd_out_te(const t_setup_entry &freq, const QDir &resource = QDir(":/config/js_config_sndsink.txt")):
        i_rt_base(resource, RT_QUEUED),  //!!because i_rt_base is virtual base class, constructor has to be defined here!!
        i_rt_base_slbuf_ex<T>(resource),
        row(0, 0)
    {
        if(false == freq.empty())
            par.replace("Rates", freq);  //update list

        nproc = 0;
        change();

        subscribe(this); //reserve fist reader for itself
    }

    virtual ~t_rt_snd_out_te(){
    }
};

#endif // RT_SND_OUT_TE

