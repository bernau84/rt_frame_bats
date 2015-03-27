#ifndef RT_DATATYPES_H
#define RT_DATATYPES_H

#include <vector>
using namespace std;

/*! \brief amplitude-frequency row/slice for rt buffer
 * 2D extension for simple rt_multibuffer
 */
template <class T> class t_rt_slice {

private:
    int irecent;  //last write index - -1 unitialized, 0 - first valid

public:

    T         t;  //time mark of this slice
    vector<T> A;  //amplitude
    vector<T> I;  //index / frequency

    //some useful operators on <A, I> pair
    typedef struct {

        T A;
        T I;
    } t_rt_ai;

    /*! \brief - test */
    bool isempty(){

       return (irecent < A.size()) ? true : false;
    }
    /*! \brief - reading from index */
    const t_rt_ai read(int i){

       i %= A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - read last written */
    const t_rt_ai last(){

       if(irecent < 0) irecent = 0;  //special case - not inited
       int i = irecent % A.size(); //prevent overrange index
       t_rt_ai v = { A[i], I[i] };
       return v;
    }
    /*! \brief - set last written */
    void set(const t_rt_ai &v){

        if(irecent < 0) irecent = 0; //special case - not inited
        A[irecent] = v.A;
        I[irecent] = v.I;
    }
    /*! \brief - writing, returns number of remaining positions */
    int append(const t_rt_ai &v){

       if(++irecent < A.size()){

           A[irecent] = v.A;
           I[irecent] = v.I;
       }

       return (A.size() - irecent);
    }

    t_rt_slice &operator= (const t_rt_slice &d){

        A = vector<T>(d.A);
        I = vector<T>(d.I);
        t = d.t;
        irecent = -1;
    }

    t_rt_slice(const t_rt_slice &d):
        t(d.t), A(d.A), I(d.I), irecent(-1){;}

    t_rt_slice(T time = T(), int N = 0, T def = T()):
        t(time), A(N, def), I(N), irecent(-1){;}
};

#endif // RT_DATATYPES_H
