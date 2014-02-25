#ifndef RT_ANALYSIS_H
#define RT_ANALYSIS_H

#include <QList>

#include "rt_basictypes.h"
#include "t_rt_base.h"
#include "freq_analysis.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//do budoucna lepsi nacitat to z ini_file

const double def_allps_num[] = {
                                        1.0
                                };
const double def_allps_den[] = {
                                        0.0
                                };

const double def_decim_num[] = {   //podle toho jestli IIR nebo FIR - preventivne tak i cit a jmen s max 256 cleny
                                        #include "include/a_3fir.h"
                                  };
const double def_decim_den[] = {
                                        0.0
                                  };

const double def_01cpb_num[1][6*3] =    {
                                            {
                                                #include "include/1_1elip_num.h"
                                            }
                                        };
const double def_01cpb_den[1][6*3] =    {
                                            {
                                                #include "include/1_1elip_den.h"
                                            }
                                        };
const double def_03cpb_num[3][3*3] = {  {
                                        //#include "include/3_3inv_num.h"
                                        #include "include/3_3cheb_num.h"
                                        }, {
                                        //#include "include/2_3inv_num.h"
                                        #include "include/2_3cheb_num.h"
                                        }, {
                                        //#include "include/1_3inv_num.h"
                                        #include "include/1_3cheb_num.h"
                                     }  };
const double def_03cpb_den[3][3*3] = {  {
                                        //#include "include/3_3inv_den.h"
                                        #include "include/3_3cheb_den.h"
                                        }, {
                                        //#include "include/2_3inv_den.h"
                                        #include "include/2_3cheb_den.h"
                                        }, {
                                        //#include "include/1_3inv_den.h"
                                        #include "include/1_3cheb_den.h"
                                     }  };
const double def_12cpb_num[12][3*2] = {
                                        {0} //#include "01cpb_num.h"
                                    };
const double def_12cpb_den[12][3*2] = {
                                        {0} //#include "01cpb_num.h"
                                    };
const double def_24cpb_num[24][3*1] = {
                                        {0} //#include "03cpb_num.h"
                                    };
const double def_24cpb_den[24][3*1] = {
                                        {0} //#include "03cpb_num.h"
                                    };

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
