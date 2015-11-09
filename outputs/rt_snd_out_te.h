#ifndef RT_SND_OUT_TE
#define RT_SND_OUT_TE

#include <stdint.h>
#include <math.h>

#include "base\rt_base_slbuf_ex.h"

//#define RT_SND_OUT_SIMUL_F 1200
#define RT_SND_CACHE_LIMIT_P 25

template<typename T> class t_rt_snd_out_te : public virtual i_rt_base_slbuf_ex<T> {

protected:
    //local properties
    int M;      //refresh rate in number of samples in one row - if known
    int N;      //multibuffer length
    int fs;  //sampling freq - given vs actual

    bool caching;

    t_rt_slice<T> row;                     /*! active row in multibuffer */

    using i_rt_base_slbuf_ex<T>::par;
    using i_rt_base_slbuf_ex<T>::buf_resize;
    using i_rt_base_slbuf_ex<T>::buf_append;
    using i_rt_base_slbuf_ex<T>::subscribe;

public:

    /*! \brief for rt player signalizing not to play yet while caching data */
    bool is_caching(){

        return caching;
    }

    /*! \brief format & copy samples into the multibuffer */
    virtual void update(t_rt_slice<T> &smp){

        for(unsigned i=0; i<smp.A.size(); i++){ //just copy to another buffer

            if(row.A.size() == 0){ //

                double t = smp.t + i/(2*smp.I[i]); //I[x] ~ freq. resol of sample = nyquist fr = fs/2
                row = (M) ? t_rt_slice<T>(t, M, (T)0) :   //version with fixed known slice size
                    t_rt_slice<T>(t, smp.A.size(), (T)0);  //auto-slice size (derived from input with respect to decimation)
            }

            T sample = smp.A[i];

#ifdef RT_SND_OUT_SIMUL_F
            sample = 0.05 * sin(RT_SND_OUT_SIMUL_F * 2*M_PI*(smp.t + i/(2*smp.I[i])));
#endif //RT_SND_OUT_SIMUL_F

            row.append(sample, smp.I[i]); //D can reduce freq. resolution because of change fs
            if(row.isfull()){  //in auto mode (M == 0) with last sample of input slice

                int rds = this->readSpace(0);
                if(rds == 0) caching = true;
                else if(rds >= ((N*RT_SND_CACHE_LIMIT_P)/100+1)) caching = false;

                buf_append(row); //write + send notifications
                row.A.clear(); //force new row initializacion
            }
        }
    }

    /*! \brief reinit buffers and local properties according to actual setting */
    virtual void change(){

        //fs = par["Rate"].get().toDouble();  //output frequency

        /*! \todo - hm....shoul be varible accroding to freq. resolution of samples
         * we need to promote change from worker to qt node to adjust sound card or whatever
         *
         * solution - fire signal CONFIG_CHANGE from update() and catch it in node in special manner -
         * before promotion to follower nod re-set data sink device to new sample rate
         */

        N = par["Multibuffer"].get().toDouble(); //slice number
        buf_resize(N);

        if((M = par["Slice"].get().toDouble()) == 0)  //slice points
            if((M = par["__auto_slicesize"].get().toDouble()) == 0)  //auto - read value from sound card
                M = 128; //emergency value
    }

    t_rt_snd_out_te(const t_setup_entry &freq, const QDir &resource = QDir(":/config/js_config_sndsink.txt")):
        i_rt_base(resource, RT_QUEUED),  //!!because i_rt_base is virtual base class, constructor has to be defined here!!
        i_rt_base_slbuf_ex<T>(resource),
        row(0, 0)
    {
        if(false == freq.empty())
            par.replace("Rate", freq);  //update list - not sure if it is ideal; should be automatic according to input samples
                    //but ok; user can set fixed (for wav recording for example)

        change();
        subscribe(this); //reserve fist reader for itself
    }

    virtual ~t_rt_snd_out_te(){
    }
};

#endif // RT_SND_OUT_TE

