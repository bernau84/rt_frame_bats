#ifndef RT_ANALYSIS_H
#define RT_ANALYSIS_H

#include <QList>

#include "rt_basictypes.h"
#include "t_rt_base.h"
#include "freq_analysis.h"

#define RT_MAX_OCTAVES_NUMBER      20
#define RT_MAX_BANDS_PER_OCTAVE    24

class t_rt_analysis : public t_rt_base
{
    Q_OBJECT

public slots:
    void start();
    void pause();

public:
    t_rt_analysis(QObject *parent = 0);
};

class t_rt_cpb : public t_rt_analysis
{
    Q_OBJECT

private:
    t_DiadicFilterBank *bank;   //decimacni vetev
    t_DelayLine *dline[RT_MAX_OCTAVES_NUMBER]; //zpodovaci linka kompenzujici group delay decimacni vetve

    t_pFilter *aline[RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE]; //analyticke pasmove filtry - max. pocet ukazatelu - to neboli
    t_pFilter *aver[RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE]; //vystupni prumerovani

public slots:

    void process();
    void change();

public:
    t_rt_cpb(QObject *parent = 0);

};

/*! \class frekvencni posuv vybraneho pasma - parametry jsou
 * decimacni faktor (pocet pasem) a vybrane indexy pasem
 *
 * pocitaji se vsechna pasma (multifazove)
 * s tim ze na vystup se mixuji vybrana
 */
class t_rt_shift : public t_rt_analysis
{
    Q_OBJECT

private:
    t_pFilter *bank[RT_MAX_BANDS_PER_OCTAVE];   //pasmova propust co se de posouvat

public slots:

    void process();
    void change();

public:
    t_rt_shift(QObject *parent = 0);

};


#endif // RT_ANALYSIS_H
