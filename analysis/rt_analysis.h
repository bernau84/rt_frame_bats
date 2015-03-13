#ifndef RT_ANALYSIS_H
#define RT_ANALYSIS_H

#include <QList>

#include "base\rt_doublebuffer.h"
#include "base\rt_base.h"
#include "inputs\rt_sources.h"
#include "freq_analysis.h"
#include "freq_filtering.h"

#define RT_MAX_OCTAVES_NUMBER      20
#define RT_MAX_BANDS_PER_OCTAVE    24

/*! \brief - interface for other sliced floating point analysis items in rt network */
class i_rt_fp_analysis : public i_rt_fp_base // == circ buffer of t_rt_slice<double>
{
    Q_OBJECT

protected:
     virtual void analyse(const t_rt_slice<double>) = 0;

public:
    i_rt_fp_analysis(QObject *parent, const QDir &config = QDir());
    virtual ~i_rt_fp_analysis(){;}

public slots:
    virtual void start(){
        i_rt_base::start();
    }

    virtual void stop(){
        i_rt_base::stop();
    }

    virtual void process(){

        /*! ommits pre validation cause it should be granted
         * by restrictions in attach() method */

        t_rt_slice<double> A;  //radek casovych dat
        while(pre->read<t_rt_slice<double> >(A, rd_i)){ //vyctem radek

            if(sta.state != t_rt_status::ActiveState){

                int n_dummy = source->t_slcircbuf::readSpace(rd_i); //zahodime data
                pre->shift(n_dummy, rd_i);
            } else {

                analyse(A);
            }
        }
    }
};


class t_rt_cpb : public i_rt_fp_analysis
{
    Q_OBJECT

private:
    int gd;    /*! groupdelay */
    int octn;  /*! number of octaves */
    int octm;  /*! number of bands in one octave */

    t_DiadicFilterBank<double> *bank;   //decimacni vetev
    t_DelayLine<double> *dline[RT_MAX_OCTAVES_NUMBER]; //zpodovaci linka kompenzujici group delay decimacni vetve

    t_pFilter<double> *aline[RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE]; //analyticke pasmove filtry - max. pocet ukazatelu - to neboli
    t_pFilter<double> *aver[RT_MAX_OCTAVES_NUMBER * RT_MAX_BANDS_PER_OCTAVE]; //vystupni prumerovani

    virtual void analyse(const t_rt_slice<double> &w);

public slots:

    void process();
    void change();

public:
    t_rt_cpb(QObject *parent = 0);
    virtual ~t_rt_cpb();

};

/*! \class frekvencni posuv vybraneho pasma - parametry jsou
 * decimacni faktor (pocet pasem) a vybrane indexy pasem
 *
 * pocitaji se vsechna pasma (multifazove)
 * s tim ze na vystup se mixuji vybrana
 */
class t_rt_shift : public i_rt_fp_analysis
{
    Q_OBJECT

private:
    int decif; /*! pocet pasem / decimacni faktor */
    int numb; /*! pocet bodu v radku */
    double refr; /*! refresh rate */
    quint32 mask; /*! vybrana pasma */

    t_pFilter<double> *bank[RT_MAX_BANDS_PER_OCTAVE];   //pasmova propust co se de posouvat

    virtual void analyse(const t_rt_slice<double> &w);

public slots:
    void process();
    void change();

public:
    t_rt_shift(QObject *parent = 0);
    virtual ~t_rt_shift();
};


#endif // RT_ANALYSIS_H
