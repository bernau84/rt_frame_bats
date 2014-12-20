#ifndef RT_ANALYSIS_H
#define RT_ANALYSIS_H

#include <QList>

#include "base\rt_basictypes.h"
#include "base\t_rt_base.h"
#include "inputs\rt_sources.h"
#include "freq_analysis.h"
#include "freq_rt_filtering.h"

#define RT_MAX_OCTAVES_NUMBER      20
#define RT_MAX_BANDS_PER_OCTAVE    24

class t_rt_analysis : public t_rt_base<double>
{
    Q_OBJECT

public:

    virtual void analyse(const t_rt_slice<double> &w) = 0;

    t_rt_analysis(QObject *parent, const QDir &config = QDir());
    virtual ~t_rt_analysis(){;}

public slots:
    virtual void start(){
        t_rt_base::start();
    }

    virtual void stop(){
        t_rt_base::stop();
    }

    virtual void process(){

        t_rt_source *source = dynamic_cast<t_rt_source *>pre;
        if(!source) return; //navazujem na zdroj dat?

        t_rt_slice<double> w;  //radek caovych dat
        while(source->t_slcircbuf::read(&w, rd_i)){ //vyctem radek

            if(sta.state != t_rt_status::ActiveState){

                int n_dummy = source->t_slcircbuf::readSpace(rd_i); //zahodime data
                source->t_slcircbuf::readShift(n_dummy, rd_i);
            } else {

                analyse(w);
            }
        }

    }

    virtual void change(){



    }

};

class t_rt_cpb : public t_rt_analysis
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
class t_rt_shift : public t_rt_analysis
{
    Q_OBJECT

private:

    int decif; /*! pocet pasem / decimacni faktor */
    int numb; /*! pocet bodu v radku */
    double refr; /*! refresh rate */
    quint32 mask; /*! vybrana pasma */

    t_pFilter<double> *bank[RT_MAX_BANDS_PER_OCTAVE];   //pasmova propust co se de posouvat

public slots:

    void process();
    void change();

public:
    t_rt_shift(QObject *parent = 0);
    virtual ~t_rt_shift();

};


#endif // RT_ANALYSIS_H
