#ifndef FREQ_ANALYSIS
#define FREQ_ANALYSIS

#include <stdlib.h>
#include <math.h>

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

extern void     f_filter(FILTER_DATA *f_specify, double input_x[], int x_N, double init_state[], double *output_y);
extern t_Compl  *f_fft(double *data_r, double *data_i, unsigned int *Nd, int w_typ);
extern t_Compl  *f_fft(double *data_r, double *data_i, unsigned int *Nd);
extern t_Compl  *t_fft(double *data_r, double *data_i, FFT_CONTEXT *con);

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




