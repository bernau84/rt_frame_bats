#include "rt_graphics.h"

#include <QtCore/QCoreApplication>


//#define DEMO 1


void rt_graphics::change()
{
    QJsonValue ts = set["Type"].get();
    if(ts.isArray()) m_selected = ts.toArray();  //vyber multigraph
    else if(ts.isDouble()) m_selected.append(ts.toDouble());  //one selection

    M = N = 0;
    t_slcircbuf *l_mb = dynamic_cast<t_slcircbuf *>(parent());  //local ptr
    for(t_rt_slice l1_amp, l2_amp; l1_amp.t <= l2_amp.t; l_mb->get(&l1_amp, rd_i)){

        l2_amp.t = l1_amp.t;
        l_mb->readShift(rd_i);
        if(M < l2_amp.v.size()) M = l2_amp.v.size();
        N += 1;
    }

    int s_size = 0, l_size = M*N*sizeof(GLfloat);  //number of points
    RT_GR_OBJ_VERTEXBUF = 0,    /*!< 0 - vertex buffer */
    RT_GR_OBJ_TIMESL,       /*!< 1 - vertical slices indexes */
    RT_GR_OBJ_FREQSL,       /*!< 2 - horizontal slices */
    RT_GR_OBJ_TRISURF,      /*!< 3 - surface triangulation */
    RT_GR_OBJ_AXES,       /*!< 4 - axis and fractions */
    RT_GR_OBJ_LABELS,       /*!< 5 - axis and fractions */
    RT_GR_OBJ_GRAD,       /*!< 6 - color gradient sub */
    RT_GR_OBJ_NUMBER
    if(m_selected.contains(RT_GR_OBJ_VERTEXBUF)){

       s_size += l_size * 3; //3 axes
       s_size += l_size * 3;    //2 colors per pixel
    }
    if(m_selected.contains(RT_GR_OBJ_TIMESL)){

       s_size += l_size * 3; //for line color
    }
    m_bufRaw = realloc(m_bufRaw, M*N*sizeof(GLfloat));
}


void rt_graphics::process()
{
#ifndef DEMO

    /*! \todo - if and only if the time from last refresh exceed the
     * selected refresh rate
     * this->startTimer(set["Refresh"].get().toDouble());
     */

    t_rt_base *pre = dynamic_cast<t_rt_base *>(parent());
    if(!pre) return; //navazujem na zdroj dat?

    if(sta.state != t_rt_status::ActiveState){

        int n_dummy = pre->t_slcircbuf::readSpace(rd_i); //zahodime data
        pre->t_slcircbuf::readShift(n_dummy, rd_i);
        return;  //bezime?
    }



    //zjistiti pocet prvku a rezervovat odpovidajici pamet (3x souradnice, 3x barva)
    //!pozor nemusi jit o matici
    //      - to by pak mohl byt ale problem s plochou; asi bysme meli body patricne zmnozit
    //      - nebo jeste lepe znulovat (a nastavit patricne blbou barvu)
    //pokud je zadane vygenerovat indexy k x rezum
    //pokud je zadane vygenerovat indexy k y rezum (podle aktualniho poctu prvku ovsem)
    //velke todo - automaticke prizpusobeni delky os a popisky

    t_rt_slice p_slice;   //radek soucasneho spektra
    t_slcircbuf::get(&p_slice, 1);  //vyctem aktualni rez

    t_rt_slice t_amp;  //radek caovych dat
    while(pre->t_slcircbuf::read(&t_amp, rd_i)){ //vyctem radek

        for(int i=0; i<t_amp.v.size(); i++){

            int dd, m  = (sta.nn_run % D);
            double t_filt = bank[m]->Process(t_amp.v[i].A, &dd);  //band pass & decimace

            sta.nn_run += 1;
            sta.nn_tot += 1;

            if((dd == 0) && (mask & (1 << m))){ //z filtru vypadl decimovany vzorek ktery chcem

                if(m & 0x1) //mirroring u lichych pasem
                    t_filt *= -1;

                p_shift.v.last().A += t_filt;  //scitame s prispevky od jinych filtru (pokud jsou vybrany)
                p_shift.v.last().f  = mask;
            }

            if(m == (D-1)){  //mame hotovo (decimace D) vzorek muze jit ven

                p_shift.v = t_rt_slice::t_rt_tf(sta.nn_tot / *sta.fs_in); //pripravim novy

                if((p_shift.v.size()) == R) {

                    t_slcircbuf::write(p_shift); //zapisem novy jeden radek
                    t_slcircbuf::read(&p_shift, 1);
                    p_shift = t_rt_slice(0, 1); //inicializace noveho spektra
                }
            }
        }
    }

    t_slcircbuf::set(&p_shift, 1);  //vratime aktualni cpb spektrum
#endif //DEMO

#if DEMO
    #define N 32
    GLfloat graph[N][N][3*3];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float x = (i - N / 2) / (N / 2.0);
            float y = (j - N / 2) / (N / 2.0);
            float d = hypotf(x, y) * 4.0;
            float z = (1 - d * d) * expf(d * d / -2.0);

            graph[i][j][0] = (i - N/2)/(N * 1.0);
            graph[i][j][1] = (j - N/2)/(N * 1.0);
            graph[i][j][2] = roundf(z * N/2 + N/2)/(N * 1.0);

            graph[i][j][3] = 0.0;  //for vertical
            graph[i][j][4] = 0.0;
            graph[i][j][5] = graph[i][j][2]-0.3;

            graph[i][j][6] = 0.0;    //for horizontal lines
            graph[i][j][7] = graph[i][j][2]-0.3;
            graph[i][j][8] = 0.0;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_bufObject[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof graph, graph, GL_STATIC_DRAW);

    // Create an array of indices into the vertex array that traces both horizontal and vertical lines
    GLushort ilines_h[(N-1) * N * 2];
    int i = 0;

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N-1; x++) {
            ilines_h[i++] = y * N + x;
            ilines_h[i++] = y * N + x + 1;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufObject[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof ilines_h, ilines_h, GL_STATIC_DRAW);

    GLushort ilines_v[(N-1) * N * 2];
    i = 0;
    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N-1; y++) {
            ilines_v[i++] = y * N + x;
            ilines_v[i++] = (y + 1) * N + x;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufObject[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof ilines_v, ilines_v, GL_STATIC_DRAW);

    GLushort itri[(N-1) * (N-1) * 6];
    i = 0;

    for (int x = 0; x < N-1; x++) {
        for (int y = 0; y < N-1; y++) {

            itri[i++] = y * N + x;
            itri[i++] = (y + 1) * N + x;
            itri[i++] = (y + 1) * N + x + 1;

            itri[i++] = y * N + x;
            itri[i++] = y * N + x + 1;
            itri[i++] = (y + 1) * N + x + 1;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufObject[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof itri, itri, GL_STATIC_DRAW);
#endif //DEMO

    render(m_bufObject, RT_GR_OBJ_NUMBER);
}




