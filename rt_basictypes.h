#ifndef RT_BASICTYPES_H
#define RT_BASICTYPES_H

#include <QString>
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

            case VAL: QJsonObject::insert("__val", val); break;
            case DEF: QJsonObject::insert("__def", val); break;
            case MIN: QJsonObject::insert("__min", val); break;
            case MAX: QJsonObject::insert("__max", val); break;
            case INFO: QJsonObject::insert("__info", val); break;
            default: break;
        }
    }

    void init(const QStringList &name, const QJsonArray &val){

        int N = qMin<int>(name.count(), val.count());
        for(int n=0; n<N; n++)
            QJsonObject::insert(name[n], val[n]);
    }

public:

    /*! \brief read actual number */
    QJsonValue get(QString name = QString("__val")){

        if(constFind(name) == constEnd())
            return value("__def");
        else
            return value(name);
    }

    /*! \brief read reserved value */
    QJsonValue get(t_restype t){

        switch(t){

            case MIN: return value("__min");
            case MAX: return value("__max");
            case INFO: return value("__info");
            case DEF: return value("__def");
            default: break;
        }

        return get(); //VAL is assumed
    }

    /*! \brief convert attribute value to double array
     */
    int db(const QString &name, double *v, int N){

        QJsonValue ar = value(name);

        if(!v || !N || !ar.isArray())
            return 0;

        int n = 0;
        foreach(QJsonValue vv, ar)
            if((vv.isDouble()) && (++n < N))
                *v++ = vv.toDouble();

        return n;
    }

    /*! \brief set new value
     * support arbitrary value between boundaries (for doubles) with hard limitation,
     * free value, select from menu
     */
    QJsonValue set(const QJsonValue &val){

        QJsonObject::const_iterator i_max = constFind("__max");
        QJsonObject::const_iterator i_min = constFind("__min");
        QJsonObject::const_iterator i_def = constFind("__def");
        QJsonValue::Type t = val.type();

        if(val.isDouble()){

            /*! out of high bound */
            if((i_max != QJsonObject::constEnd()) && (i_max.value().isDouble()))
                if(val.toDouble() > i_max.value().toDouble())
                    return QJsonObject::insert("__val", i_max.value()).value();

            /*! out of low bound */
            if((i_min != QJsonObject::constEnd()) && (i_min.value().isDouble()))
                if(val.toDouble() < i_min.value().toDouble())
                    return QJsonObject::insert("__val", i_min.value()).value();

            /*! in bound */
            if((i_max != QJsonObject::constEnd()) || (i_min != constEnd()))
                return QJsonObject::insert("__val", val).value();
        }

        /*! free value (defined only with default) */
        if((count() <= 2) && (i_def != constEnd()))
            return QJsonObject::insert("__val", val).value();

        /*! else enum value is assumed */
        for(QJsonObject::const_iterator i = constBegin(); i != constEnd(); i++)
            if(i.value() == val)
                return QJsonObject::insert("__val", val).value();

        /*! value outside emun - no change possible - return actual value */
        return get();
    }

    /*! \brief add new predefined value and select it*/
    QJsonValue set(const QString &name, const QJsonValue &val, t_restype t = DEF){

        QJsonObject::insert(name, val);
        return sel(name);
    }

    /*! \brief sel from predefined - set default if name doesnt exist */
    QJsonValue sel(const QString &name){

        return QJsonObject::insert("__val", get(name)).value();
    }

    /*! \brief sel from reserved - min, max, def */
    QJsonValue sel(t_restype t){

        return QJsonObject::insert("__val", get(t)).value();
    }

    /*! \brief init from lists
     */
    t_setup_entry(const QJsonArray &val, const QStringList &name){

        init(name, val);
    }

    /*! \brief init from array
     */
    t_setup_entry(const QJsonArray &val, const QString &unit = ""){

        QStringList name;
        foreach(QJsonValue i, val)
            name.append(i.toString() + unit);

        init(name, val);
    }

    /*! \brief simple default item
     */
    t_setup_entry(const QJsonValue &val){

        QJsonObject::insert("__def", val);
    }

    /*! \brief create & inicialize
     */
    t_setup_entry(const QJsonObject &val = QJsonObject()):
        QJsonObject(val){

    }
};

/*! list of JSonObject as attrbutes presendet outside as t_setup_enty item
 * list of object is hidden as private, read must be done via ask,
 * write via udate */

class t_collection_entry : private QObject, QJsonObject {

    Q_OBJECT
signals:
    void changed(QString title);

public:

    /*! helpers */
    QJsonArray vlist(){  QJsonArray t; return t; }
    QStringList slist(){  QStringList t; return t; }
    QStringList slist(QVariantList v, QString unit = ""){

        QStringList t;
        for(int i=0; i < v.count(); i++) t << v[i].toString() + unit;
        return t;
    }

    /*! \brief insert or replace attribute from user
    */
    void insert(QString &title, const t_setup_entry &attribute){

        QJsonObject::iterator ai = find(title);
        if(ai == end())
            return;

        /*! update all keys version */
        *ai = attribute;
    }

    /*! \brief access/test attribute
    */
    const t_setup_entry *ask(const QString &title){

        QJsonObject::iterator ai = find(title);
        if(ai == end())
            return &(ai.value());

        return NULL;
    }

    /*! \brief create & inicialize
     */
    explicit t_collection_entry(QObject *parent = 0, QJsonObject &def = QJsonObject()):
        QObject(parent), QJsonObject(def){

    }
};

#endif // RT_BASICTYPES_H
