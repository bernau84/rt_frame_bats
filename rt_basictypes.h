#ifndef RT_BASICTYPES_H
#define RT_BASICTYPES_H

#include <QString>
#include <QObject>
#include <QJsonDocument>

class t_collection_entry;

/*! hodonoty nastaveni
 * podporuje klice / menu / min / max / default / multiselect
 */
class t_setup_entry : public QJsonObject {

public:
    enum t_restype { INFO, MIN, MAX, DEF, VAL, ANY };

private:

    void insert(t_restype f, const QJsonValue &val){

        switch(f){

            case DEF: insert("__def", val); break;
            case MIN: insert("__min", val); break;
            case MAX: insert("__max", val); break;
            case INFO: insert("__info", val); return;
        };
    }

    void init(const QStringList &name, const QJsonArray &val){

        int N = qMin(name.count(), val.count());
        for(int n=0; n<N; n++)
            insert(name[n], val[n], mask);
    }

public:

    /*! \brief read actual number */
    QJsonValue get(QString name = QString("__val")){

        return value(name, value("__def"));
    }

    /*! \brief read actual number */
    QJsonValue get(t_restype t = VAL){

        switch(f){

            case MIN: get("__min"); return;
            case MAX: get("__max"); return;
            case INFO: get("__info"); return;
            case INFO: get("__val"); return;
        }

        return get("__def");
    }

    /*! \brief read multivalue */
    QJsonArray getm(QString title = QString("val")){

        return get(title).toArray();
    }

    /*! \brief set arbitrary value between boundaries
     * assumes vales can be converted to double for
     * min max comparison
     */
    QJsonValue set(const QJsonValue &val){

        QJsonObject::const_iterator i_max = find("__max");
        QJsonObject::const_iterator i_min = find("__min");
        QJsonObject::Type t = val.type();

        if(val.isDouble()){

            if((i_max != constEnd()) && (i_max->isDouble()))
                if(val.toDouble() > i_max->toDouble())
                    return insert("__val", i_max->value())->value();

            if((i_min != constEnd()) && (i_min->isDouble()))
                if(val.toDouble() < i_min->toDouble())
                    return insert("__val", i_min->value())->value();
        }

        return insert("__val", val)->value();
    }

    /*! \brief convert attribute value to double array
     */
    int db(const QString &name, double *v, int N){

        int n = 0;
        QJsonArray ar;

        if(!v || !N || !((ar = value[name]).isArray()))
            return 0;

        foreach(QJsonValue vv, ar)
            if((vv.isDouble()) && (++n < N))
                *v++ = vv.toDouble();

        return n;
    }


    /*! \brief add new predefined value and select it*/
    QJsonValue set(const QString &name, const QJsonValue &val, restype t = UNDEF){

        if(UNDEF != t) init(name, val, t);
            else init(name, val);

        return sel(name);
    }

    /*! \brief from enum menu or min, max, def, val keywords */
    QJsonValue sel(const QString &name){

        return insert("__val", value(name)).value();
    }

    /*! \brief seznam dovolenych prvku - idef je index defaultniho (pokud <0 pak zadny neni),
     *  autob jsou automaticke meze (min je prvni, max poslednim porvkem)
     */
    t_setup_entry(const QJsonArray &val, const QStringList &name){

        init(name, val);
    }

    /*! \brief simple default item
     */
    t_setup_entry(const QJsonValue &val){

        insert("__def", val);
    }

    /*! \brief create & inicialize
     */
    t_setup_entry(const QJsonObject &val = QJsonObject()):
        QJsonObject(val){

    }
};


class t_collection_entry : public QObject, QJsonObject {

    Q_OBJECT
signals:
    void changed(QString title);

public:

    /*! helpers */
    QJsonArray vlist(){  QJsonArray t; return t; }

    QStringList slist(){  QStringList t; return t; }
    
    template<T> QJsonArray vlist(const T *v, int n){

        QJsonArray t;
        for(int i=0; (i<n) && (v); i++) t << *v++;
        return t;
    }

    /*! \brief update from user
     *  \return number of succesfuly updated attributes (only that which was already contained)
     * inserting is not supported
    */
    int update(t_setup_entry &attributes){

        int ret = 0;
        QStringList kl = attributes.keys();
        for(QStringList::const_iterator ki = kl.constBegin(); ki != kl.constEnd(); ki++){

            QJsonObject::iterator oi = find(ki);
            if(oi == end())
                continue;

            *io = attributes[ki];
            ret += 1;
        }

        return ret;
    }

    /*! \brief acces attribute
    */
    const t_setup_entry ask(const QString &title){

        t_setup_entry v(value[attribute]);
        return v;
    }

    /*! \brief create & inicialize
     */
    explicit t_collection_entry(QObject *parent = 0, QJsonObject &def = QJsonObject()):
        QObject(parent), QJsonObject(def){

    }
};

#endif // RT_BASICTYPES_H
