#ifndef FREQ_ANALYSIS
#define FREQ_ANALYSIS

#include <stdlib.h>
#include <math.h>

#include "t_rw_buffer.h" //pozor verze s rychlou metodou AddSample()
//+ k projektu pridat my_fft.cpp

#define IIR_DIRECT1_STRUCT   0x00
#define IIR_DIRECT2_STRUCT   0x01
#define IIR_LATTICE_STRUCT   0x02
#define IIR_BIQUADR_STRUCT   0x04

#define FIR_DIRECT1_STRUCT   0x08
#define FIR_DELAYLN_STRUCT   0x09
#define FIR_LATTICE_STRUCT   0x10

#define FFT_FILTER_STRUCT    0x80

#define WINDOW_HANN 0x10
#define WINDOW_HAMM 0x11
#define WINDOW_FLAT 0x12
#define WINDOW_BLCK 0x14
#define WINDOW_RECT 0x18
#define WINDOW_BRTL 0x20
#define WINDOW_GAUS 0x21

#define FFT_MODE_OPT_WIN_MASC   0x000000FF      //option typ okenka
#define FFT_MODE_OPT_OVR_MASC   0x0000FF00      //option prekryti okenka
#define FFT_MODE_OPT_PAR_MASC   0xFFFF0000      //option parametr(napr. sigma, scale) okenka, pokud ho ma

#ifndef PI
        #define PI 3.14159265358979323846
#endif

/* koeficienty zadavat podle impulsni charakteristiky numeratoru - od nejstarsiho
tj. jako v matlabu. data take on nejstarsiho (nejnovejsi vzorek na poli s nejvyssim
indexem ). */

typedef struct {

    double *num_coeff; //b(0), ..., b(nb-1)
    int num_N;
    double *den_coeff; //a(0), ..., a(na-1)
    int den_N;

    int structure;
} FILTER_DATA;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class t_Compl{

  public:
    double real,imag;
    t_Compl(double r=0, double i=0):real(r),imag(i) {;}
    ~t_Compl(){;}

    t_Compl operator =(t_Compl src){ real=src.GetRe(); imag=src.GetIm(); return src; }
    t_Compl operator +(t_Compl src){ return t_Compl(real+src.real,imag+src.imag); }
    t_Compl operator -(t_Compl src){ return t_Compl(real-src.real,imag-src.imag); }
    t_Compl operator *(t_Compl src){ return t_Compl(real*src.real-imag*src.imag, src.real*imag+src.imag*real); }

    double GetRe(void){ return real; }
    double GetIm(void){ return imag; }
    double Abs(void){ return sqrt(real*real+imag*imag); }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef struct {

    int     N;   //pocet bodu FFT (v mocnine 2)
    int     M;   //mocnina 2

    t_Compl *TwF; //tabelace twidle factoru 1..N/2
    double *Win; //tabelace okynka
    int *ExI; //tabelace idexu prohazeni vzorku

    double Norm;     //kompenzace na plochu okynka - normovani na 1
    double Over;     //prekryti okynka (pocitase tak aby v miste
} FFT_CONTEXT;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef struct {

    int nS;     //pocet tabelovanych hodnot sigma
    int nF;     //pocet tabelovanych relativnich frekvenci
    int nB;     //velikost sloupcu tabulky Sopt_vs_F
    int I;      //index aktualniho S

    double  **Smeas_vs_F;    //nF x nS - multiline graf pro nalezeni skutecne sirky pasma
    double  *Sopt_vs_F[2];    //x (~2x30 hodnot)
    double  *Ftab;          //nF
    double  *Stab;          //nS
} AGAUSS_CONTEXT;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//Obecny predek pro real-time cislicove filtry
class t_pFilter{

    protected:
        TBuffer<double>   *shift_reg;   //vytvari se instance
        std::array<double> *coeff_num;   //kopiruje se jen ukazatel
        double            *coeff_den;
        double            *shift_dat;   //vytvari se instance
        int               N;            //!!!pocet koeficientu

        int dec_fact;     //mozny je jen faktor > 0, <0 nemaji uplne koser vyznam
        int act_stat;
        int typ_filt;        

    public:
        int     GetTyp(){ return(typ_filt); } //por pozdni identifikaci typu
        void    Reset(double *_shift_reg);    //inicializuje posuvny_reg, pokud NULL pak ho vynuluje
        virtual double Process(double new_smpl, int *n_to_proc) = 0;

        t_pFilter(t_pFilter &src_coeff_cpy);  //nekopiruje shift_reg
        t_pFilter(double *_coeff_num, double *_coeff_den, int _N);
        t_pFilter(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact);
        virtual ~t_pFilter();                 //uvolni coeficienty a destroyne Buffer
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//jen zpozdovaci linka - koeficienty nehraji roli
class t_DelayLine : public t_pFilter{
    public:
        double Process(double new_smpl, int *n_to_proc);

        t_DelayLine(t_pFilter &src_coeff_cpy)
                      :t_pFilter(src_coeff_cpy){ typ_filt = FIR_DELAYLN_STRUCT; }
        t_DelayLine(double *_coeff_num, double *_coeff_den, int _N)
                      :t_pFilter(_coeff_num, _coeff_den, _N+1){ typ_filt = FIR_DELAYLN_STRUCT; }
        t_DelayLine(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact)
                      :t_pFilter(_coeff_num, _coeff_den, _N+1, _dec_fact){ typ_filt = FIR_DELAYLN_STRUCT; }

        ~t_DelayLine(){;}
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//IIR Kannonicka forma - direct form 2nd.class
//N-1 - order of filter => N coeff is needed
class t_CanonFilter : public t_pFilter{
    public:
        double Process(double new_smpl, int *n_to_proc);

        t_CanonFilter(t_pFilter &src_coeff_cpy)
                      :t_pFilter(src_coeff_cpy){ typ_filt = IIR_DIRECT1_STRUCT; }
        t_CanonFilter(double *_coeff_num, double *_coeff_den, int _N)
                      :t_pFilter(_coeff_num, _coeff_den, _N){ typ_filt = IIR_DIRECT1_STRUCT; }
        t_CanonFilter(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact)
                      :t_pFilter(_coeff_num, _coeff_den, _N, _dec_fact){ typ_filt = IIR_DIRECT1_STRUCT; }
        ~t_CanonFilter(){;}
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//IIR Kaskadni forma - biQuadratic
//N - number of stages
class t_BiQuadFilter : public t_pFilter{
    public:
        double Process(double new_smpl, int *n_to_proc);

        t_BiQuadFilter(t_pFilter &src_coeff_cpy)
                       :t_pFilter(src_coeff_cpy){ typ_filt = IIR_BIQUADR_STRUCT; }
        t_BiQuadFilter(double *_coeff_num, double *_coeff_den, int _N)
                       :t_pFilter(_coeff_num, _coeff_den, 3*_N){ typ_filt = IIR_BIQUADR_STRUCT; }
        t_BiQuadFilter(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact)
                       :t_pFilter(_coeff_num, _coeff_den, 3*_N, _dec_fact){ typ_filt = IIR_BIQUADR_STRUCT; }
        ~t_BiQuadFilter(){;}
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//FIR Prima struktura
class t_DirectFilter : public t_pFilter{
    public:
        double Process(double new_smpl, int *n_to_proc);

        t_DirectFilter(t_pFilter &src_coeff_cpy)
                       :t_pFilter(src_coeff_cpy){ typ_filt = FIR_DIRECT1_STRUCT; }
        t_DirectFilter(double *_coeff_num, double *_coeff_den, int _N)
                       :t_pFilter(_coeff_num, _coeff_den, _N){ typ_filt = FIR_DIRECT1_STRUCT; }
        t_DirectFilter(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact)
                       :t_pFilter(_coeff_num, _coeff_den, _N, _dec_fact){ typ_filt = FIR_DIRECT1_STRUCT; }
        ~t_DirectFilter(){;}
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//FIR Krizova - lattice struktura
class t_LatticeFilter : public t_pFilter{
    public:
        double Process(double new_smpl, int *n_to_proc);

        t_LatticeFilter(t_pFilter &src_coeff_cpy)
                       :t_pFilter(src_coeff_cpy){ typ_filt = FIR_LATTICE_STRUCT; }
        t_LatticeFilter(double *_coeff_num, double *_coeff_den, int _N)
                        :t_pFilter(_coeff_num, _coeff_den, _N){ typ_filt = FIR_LATTICE_STRUCT; }
        t_LatticeFilter(double *_coeff_num, double *_coeff_den, int _N, int _dec_fact)
                       :t_pFilter(_coeff_num, _coeff_den, _N, _dec_fact){ typ_filt = FIR_LATTICE_STRUCT; }
        ~t_LatticeFilter(){;}
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//Filter Bank

class t_DiadicFilterBank{

    private:
        t_pFilter **HP_filter;
        t_pFilter **LP_filter;

        double *prep_smpl; //vstupni data pro vyssi stages
        int    *prep_indx; //indexy nalezitosti dat ke stages

        int    M;          //pocet stupnu(stages)
        int    N;          //velikost bufferu

        int    time_indx;
    public:
        int    Process( double new_smpl, double *sum_rslt );
        void   Reset();                                         //posuvne registry

        template<class HP_TYP, class LP_TYP> t_DiadicFilterBank( HP_TYP &_HP_filter, LP_TYP &_LP_filter, int _M );
        ~t_DiadicFilterBank();
};
//pokud potrebujeme realizovat jen prevzorkovaci linku, pak ha HP filtr dat fir s jejinym koef 1.
//------------------------------------------------------------------------------
//naprosto nezbytne je aby mel zadany lp filtr nastavenou decimaci na 2!!!!
template<class HP_TYP, class LP_TYP> t_DiadicFilterBank::t_DiadicFilterBank( HP_TYP &_HP_filter, LP_TYP &_LP_filter, int _M ){

   time_indx = 0;
   if( (M = _M) > 0 ){

       N = 0x1 << (M-1);
       HP_filter = new t_pFilter*[M]; LP_filter = new t_pFilter*[M];
       prep_smpl = new double[N+1];  prep_indx = new int[N];    //+1 pro posledni LP filtraci (u CPB normalne zahazujem)
       for( int i=0; i<M; i++ ){ HP_filter[i] = new HP_TYP(_HP_filter); LP_filter[i] = new LP_TYP(_LP_filter); }
       for( int i=0; i<N; i++ ){ prep_smpl[i] = 0.0; prep_indx[i] = 0; }
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//PacketTree - nema harmonizovany beh jako banka filtru
#define PACKETTREE_MAX_STAGES       12
class t_PacketTree{

    private:
        t_DirectFilter **HP_filter[PACKETTREE_MAX_STAGES];
        t_DirectFilter **LP_filter[PACKETTREE_MAX_STAGES];

        int    M;          //pocet stupnu(stages)
    public:
        int    Process( double new_smpl, double *sum_rslt );
        void   Reset();                                         //posuvne registry

        t_PacketTree(t_DirectFilter &_HP_filter, t_DirectFilter &_LP_filter, int _M );
        ~t_PacketTree();
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

extern void     my_filter(FILTER_DATA *f_specify, double input_x[], int x_N, double init_state[], double *output_y);
extern t_Compl  *my_fft(double *data_r, double *data_i, unsigned int *Nd, int w_typ);
extern t_Compl  *my_fft(double *data_r, double *data_i, unsigned int *Nd);
extern t_Compl  *my_fft(double *data_r, double *data_i, FFT_CONTEXT *con);

extern void gen_window( double *rslt, int N, unsigned int type );
extern int  init_fftcontext(FFT_CONTEXT *con, int _N_FFT, int _WIN, int WinComp, int OvrComp);
extern void rebuild_fftcontext(FFT_CONTEXT *con, int _WIN);
extern void destroy_fftcontext(FFT_CONTEXT *con);

extern void init_agausscontext(AGAUSS_CONTEXT *con, char *path);
extern void destroy_agausscontext(AGAUSS_CONTEXT *con);
extern double process_agauss(AGAUSS_CONTEXT *con, double *fft, int M);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#endif




