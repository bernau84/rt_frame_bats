#ifndef RT_CPB_H
#define RT_CPB_H

#include "rt_analysis.h"
#include "freq_analysis.h"
#include "freq_filtering.h"

#define RT_MAX_OCTAVES_NUMBER      20
#define RT_MAX_BANDS_PER_OCTAVE    24


template <typename T> class t_rt_cpb_te : public i_rt_sl_csimo_te<T>
{

private:
    int gd;    /*! groupdelay */F
    int octn;  /*! number of octaves */
    int octm;  /*! number of bands in one octave */

    t_DiadicFilterBank<T> *bank;   //decimacni vetev
    t_DelayLine<T> *dline[RT_MAX_OCTAVES_NUMBER]; //zpodovaci linka kompenzujici group delay decimacni vetve

    t_pFilter<T> *aline[RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE]; //analyticke pasmove filtry - max. pocet ukazatelu - to neboli
    t_pFilter<T> *aver[RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE]; //vystupni prumerovani

    virtual void update(const void *dt, int size);  //processing functions
    virtual void change();

public:
    t_rt_cpb_te(QObject *parent = 0);
    virtual ~t_rt_cpb_te ();

};


#endif // RT_CPB_H
