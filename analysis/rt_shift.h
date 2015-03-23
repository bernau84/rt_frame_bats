#ifndef RT_SHIFT_H
#define RT_SHIFT_H

#include "rt_analysis.h"
#include "freq_analysis.h"
#include "freq_filtering.h"

#define RT_MAX_OCTAVES_NUMBER      20

/*! \class frekvencni posuv vybraneho pasma - parametry jsou
 * decimacni faktor (pocet pasem) a vybrane indexy pasem
 *
 * pocitaji se vsechna pasma (multifazove)
 * s tim ze na vystup se mixuji vybrana
 */
template <typename T> class t_rt_shift_te : public i_rt_sl_csimo_te<T>
{

private:
    int decif; /*! pocet pasem / decimacni faktor */
    int numb; /*! pocet bodu v radku */
    double refr; /*! refresh rate */
    quint32 mask; /*! vybrana pasma */

    t_pFilter<T> *bank[RT_MAX_BANDS_PER_OCTAVE];   //pasmova propust co se de posouvat

    virtual void update(const void *dt, int size);  //processing functions
    virtual void change();

public:
    t_rt_shift_te(QObject *parent = 0);
    virtual ~t_rt_shift_te();
};

#endif // RT_SHIFT_H
