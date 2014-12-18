#ifndef BUFFER_TMPLTS
#define BUFFER_TMPLTS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef UINT
#define UINT    unsigned int
#endif //UINT
#ifndef SINT
#define SINT    signed int
#endif //SINT

typedef enum /*TbufStates*/ { BS_EMPTY=0, BS_RUNNIG=1, BS_FULL=2, BS_OVERFLOW=3 } TbufState;                     //stavy user bufferu
typedef enum /*TbufStates*/ { BM_CIRC=0, BM_LIFO=1 /*, BM_READ=2, BM_FIFO=3 */ } TbufModes;

template <class DT_TYP> class TBuffer {
    private:
        TbufState       state;
        TbufModes       mode;

        DT_TYP         *b_pos;//base
        DT_TYP         *a_pos;//actual
        UINT           length;
    public:
        //zakladni operace
        TbufState      AddSample(const DT_TYP sample);   //jednoduche pridani vzorku - aby se to nekomplikovalo memcpy
        TbufState      GetSample( DT_TYP *sample );  //jednoduche cteni vzorku
        //opreace na pointrech
        TbufState      SetActual( DT_TYP *_a_pos );  //cteni aktualni hodnoty - bez posuvu
        TbufState      MoveActual( int shift );      //posuv v bufferu
        TbufState      GetActual( DT_TYP **_a_pos ); //jakop ram. vraci pointr
        //znovu jen rychlejsi - pokud nas nezajima stav. druhy op. je pro pretizeni
        void      AddSample( DT_TYP sample, int );
        void      MoveActual( int shift, int );
        void      SetActual( DT_TYP *_a_pos, int );
        //obecne blokove operace
        TbufState      AddSample(const DT_TYP *p_src, UINT *n_src);          //vzdy posunuje ukazatel dopredu
        TbufState      GetSample( DT_TYP *p_dst, UINT *n_dst );          //pokud neni kruhovy, pak jde ukazatel dozadu

        TBuffer( DT_TYP *_b_pos, UINT _length, TbufModes _mode );
        TBuffer( TBuffer &_origin );
        ~TBuffer(){;}
};

/******************************************************************************/
/******************************************************************************/
template <class DT_TYP> TBuffer<DT_TYP>::TBuffer(DT_TYP *_b_pos, UINT _length, TbufModes _mode):
      mode(_mode), b_pos(_b_pos), length(_length) {

      a_pos = b_pos;
      state = BS_EMPTY;
}

/******************************************************************************/
template <class DT_TYP> TBuffer<DT_TYP>::TBuffer(TBuffer<DT_TYP> &_origin):
      mode(_origin.mode), length(_origin.length), b_pos(_origin.b_pos){

      a_pos = b_pos;
      state = BS_EMPTY;
}
//BASIC OPERATION FULL IMPLEMENTATION
/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::SetActual( DT_TYP *_a_pos ){

      a_pos = _a_pos;
      if( (a_pos >= b_pos) && ((a_pos - b_pos)<length) ){

         if( a_pos != b_pos ){

              return state;
         }  else  return( (state=BS_EMPTY) );
      } else  return( (state=BS_OVERFLOW) );             //!!coz je dost velka chyba!!, vznikne pri nesmyslnych _a_pos
}

/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::MoveActual( int shift ) {
//je to blbuvzdorny
//u CIRC lepe mozna predpisem a_pos = b_pos[((a_pos-b_pos)+shift+length)%length];
    DT_TYP *n_pos = a_pos; n_pos += shift;

       if( n_pos < b_pos ){

           if( mode == BM_CIRC ){ n_pos += length; state = BS_OVERFLOW; }
              else { n_pos = b_pos; state = BS_EMPTY; }
       }
       else if( (n_pos-b_pos)>=length ){

           if( mode == BM_CIRC ){ n_pos -= length; state = BS_OVERFLOW; }
              else { n_pos = b_pos; n_pos += length; state = BS_FULL; }
       }

       a_pos = n_pos;
       return state;
}

/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::AddSample(const DT_TYP sample ) {

      if( state != BS_FULL ) *a_pos++ = sample;
      if( (a_pos - b_pos) == length ){ //nebo a_pos = b_pos + (b_pos-a_pos) % length

            if( mode == BM_CIRC ){

                   a_pos = b_pos;
                   return( (state=BS_OVERFLOW) );
            } else {
                   a_pos = &b_pos[length-1];
                   return( (state=BS_FULL) );
            }
      }
      return(state);
}
/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::GetSample( DT_TYP *sample ) {

       *sample = *a_pos++;
       if( (a_pos - b_pos) == length ){ //nebo a_pos = b_pos + (b_pos-a_pos) % length

            if( mode == BM_CIRC ){

                   a_pos = b_pos;
                   return( (state=BS_OVERFLOW) );
            } else {
                   a_pos = &b_pos[length-1];
                   return( (state=BS_EMPTY) );
            }
       }
       return(state);

}
//BASIC OPERATION, FAST IMPLEMENTATION ~ zachazeni jako s CIRC
/******************************************************************************/
template <class DT_TYP> void TBuffer<DT_TYP>::SetActual( DT_TYP *_a_pos, int ){

      UINT indx = _a_pos - b_pos;
      if( (indx>=0) && (indx<length) ) a_pos = _a_pos;
}

/******************************************************************************/
//!!pokud chceme aby vysledek op. % byl int, musi take oba oprerandy byti int
template <class DT_TYP> void TBuffer<DT_TYP>::MoveActual( int shift, int ) {

    //defaultne jako CIRC, protoze tak se nemuzeme indexem dostat mimo rozsah

    a_pos = &b_pos[((a_pos-b_pos) + (shift%int(length)) + length) % length];
    //ziskani new_indx  =  (act_i    +    posuv     +    kvuli zap.cislum ) % len
    //       <0, len-1>  <0, len-1>  <-len+1,len-1>        <0, 2len-1>     <0, len>
}

/******************************************************************************/
template <class DT_TYP> void TBuffer<DT_TYP>::AddSample( DT_TYP sample, int ) {

    //defaultne jako CIRC, protoze tak se nemuzeme indexem dostat mimo rozsah
    *a_pos++ = sample;
         if( (a_pos - b_pos) == length ) a_pos = b_pos;
         //u point. asi rychl. nez a_pos = &b_pos[ (a_pos - b_pos)%length ];

}
/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::GetActual( DT_TYP **_a_pos ){

      *_a_pos = a_pos;
      return state;
}
//BLOCK OPERATION
/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::AddSample(const DT_TYP *p_src, UINT *n_src ) {

      UINT n_free;

      if( state == BS_FULL ) return state; //neni nutne, ale rychlejsi

      if( (n_free = length-(a_pos-b_pos)) <= *n_src ){             //zjisti volne misto a porovna
                                                                   //!!!a_pos je nezadan pozice
            memcpy( (void *)a_pos, (void *)p_src, (n_free)*sizeof(DT_TYP) );        //ulozi co muze nebo zaplni celej nebo i nulu
            if( mode == BM_CIRC ){

                 memcpy( (void *)b_pos, (void *)&p_src[n_free], (*n_src-n_free)*sizeof(DT_TYP) );   //ulozi zbytek - i kdyby 0
                 a_pos = &b_pos[*n_src-n_free]; state = BS_OVERFLOW;                //ukazatel na dalsi volnou pozici
            } else {

                 a_pos = &b_pos[length]; state = BS_FULL;                          //!!ukazatel mimo pole
                 *n_src = n_free;
            }
      } else {

            memcpy( (void *)a_pos, (void *)p_src, (*n_src)*sizeof(DT_TYP) );
            a_pos += *n_src; state = BS_RUNNIG;
      }

      return( state );

}

/******************************************************************************/
template <class DT_TYP> TbufState TBuffer<DT_TYP>::GetSample( DT_TYP *p_dst, UINT *n_dst ) {

      UINT n_free;

      if( state == BS_EMPTY ) return state;

      if( mode == BM_CIRC ){   /*!!!kontrola nutna!!!*/

           //dopredne cteni - do prava
           if( (n_free = length-(a_pos-b_pos)) <= *n_dst ){                                      //zjisti volne misto a porovna

              memcpy( (void *)p_dst, (void *)a_pos, (n_free)*sizeof(DT_TYP) );                   //precte co muze, docte do konce
              memcpy( (void *)&p_dst[n_free], (void *)b_pos, (*n_dst-n_free)*sizeof(DT_TYP) );   //ulozi zbytek - i kdyby 0
              a_pos = &b_pos[*n_dst-n_free]; state = BS_OVERFLOW;                                //ukazatel na dalsi volnou pozici

           } else {

              memcpy( (void *)p_dst, (void *)a_pos, (*n_dst)*sizeof(DT_TYP) );
              a_pos += *n_dst; state = BS_RUNNIG;
           }
       } else {

           //cteni smerem do leva
           if( (n_free = (a_pos-b_pos)) <= *n_dst ){  //pri stavu FULL je a_pos = b_pos[length] --> OK

              memcpy( (void *)p_dst, (void *)b_pos, (n_free)*sizeof(DT_TYP) );           //cte co se da do a_pos-1
              a_pos = &b_pos[0]; state = BS_EMPTY;
              *n_dst = n_free;
           } else {
           
              a_pos -= *n_dst;
              memcpy( (void *)p_dst, (void *)a_pos, (*n_dst)*sizeof(DT_TYP) );
              state = BS_RUNNIG;
           }
       }

      return( state );
}

#endif // BUFFER_TMPLTS
