#ifndef RT_GRAPH_COLORMAP_H
#define RT_GRAPH_COLORMAP_H


/*! \brief - okno s barevnym gradientem a osou pro min a max
 * jde v podstate o extra jednoduchy graf s vlastnim frameworkem
*/
class rt_color_scale : public i_rt_base<float>, rt_graph_frame {

public:
    rt_graph_context *m_canvas;

public slots:
    on_change();

    /*! \todo
     */

    explicit rt_color_scale(QObject *parent = 0, const QDir &resource = QDir(), rt_graph_context *canvas):
        i_rt_base(parent, resource),
        m_canvas(canvas)
    {
        change();
    }

    virtual ~rt_color_scale(){

    }
};



typedef rt_graph_clr_map QList<QPair<QColor, QColor> >;

class rt_graph_clr_gradient {

private:

    e_rt_graph_clr_interp type;

    /*! \brief - table with pair of input color and desired color
     * gradient
     */
    rt_graph_clr_map table;

    /*! \brief - return index in table
     * with lower nearest value to c in fist position on color
     * channel ch (0 - R, 1 - G, B - 2)
     */
    int nearest(quint8 c, quint8 ch){

        for(int i=1; i<table.size(); i++)
            if(c > (table(i).first.rgba() >> (8*ch)))
                return i-1;

        return table.size()-1;
    }

    quint8 interp(quint8 c, quint8 ch){

        int i = nearest(c, ch);

        switch(type){

            case SPLINE: /*! < \todo */
            case LINEAR: /*! < go to default step interp while color c i out of table bounds */
            {
                if(i < (table.size()-1)){

                    float Ax = table(i+0).first.rgba() >> (8*ch);
                    float Ay = table(i+0).second.rgba() >> (8*ch);
                    float Bx = table(i+1).first.rgba() >> (8*ch);
                    float By = table(i+1).second.rgba() >> (8*ch);

                    if(c >= Ax)
                        return (Ay + (Bx - c)*(By - Ay)/(Bx - Ax + 0.001));
                }
            }

            case STEP:
            defaut:

                return table(i).second.rgba() >> (8*ch);
            break;
        }
    }

public:

    enum e_rt_graph_clr_interp {

        STEP,
        LINEAR,
        SPLINE
    } type;

    /*! \brief - for complex color transformation when each color channel is independant of other
     * for example amplitude is brightness and angle hue, and so...
     */
    QColor convert(QColor &x){

        QColor c;
        c.setRed(interp(x.getRed(), 0));
        c.setGreen(interp(x.getGreen(), 1));
        c.setBlue(interp(x.getBlue(), 2));
        return c;
    }

    /*! \brief - when output color is function ony of one value x
     * (amplitude for example)
     */
    QColor convert(qreal x){

        QColor c;
        c.setRgbF(x, x, x, 1.0);
        return convert(c);
    }

    /*! \brief - costructor request colormap in pair of input and adjacent output color
     * interpolation in-between mapped color is also optional
     */
    rt_graph_clr_gradient(rt_graph_clr_map ta, e_rt_graph_clr_interp ty = STEP):
        table(ta), type(ty){

    }

    ~rt_graph_clr_gradient();

};
#endif // RT_GRAPH_COLORMAP_H
