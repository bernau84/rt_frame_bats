#ifndef RT_BASICTYPES_H
#define RT_BASICTYPES_H

#include <QString>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QVariantList>

class t_collection_entry;

/*! hodonoty nastaveni
 * podporuje klice / menu / min / max / default / multiselect
 */
class t_setup_entry : public QMap<QString, QVariant> {

private:

    enum restype { MIN, MAX, DEF, ENUM, UNDEF };

    void init(const QString &name, const QVariant &val, uint tmap = 0){

        insert(name, val);
        if((1 << DEF) & tmap) insertMulti("def", val);  //tech muze byt vicero
        else if((1 << MIN) & tmap) insert("min", val);
        else if((1 << MAX) & tmap) insert("max", val);
        else if(0){;}
    }

    void init(const QStringList &name, const QVariantList &val, int idef = 0, bool autob = false){

        int N = qMin(name.count(), val.count());
        for(int n=0; n<N; n++){

            uint mask = 0;
            mask |= (idef == n) ? (1 << DEF) : 0;
            mask |= ((autob) && (n == 0)) ? (1 << MIN) : 0;
            mask |= ((autob) && (n == N-1)) ? (1 << MAX) : 0;
            init(name[n], val[n], mask);
        }
    }

public:

    /*! read actual number */
    QVariant get(QString name = QString("val")){

        return value(name, value("def"));
    }

    QVariantList getm(QString title = QString("val")){

        return values(title);
    }

    /*! set arbitrary value between boundaries
     * assumes vales can be converted to double for
     * min max comparison
     */
    QVariant set(const QVariant &val){

        QMap<QString, QVariant>::const_iterator i_max = find("max");
        QMap<QString, QVariant>::const_iterator i_min = find("min");

        double v_min = i_min.value().toDouble();
        double v_max = i_max.value().toDouble();

        if((i_min != constEnd()) && (v_min > val.toDouble()))
            insert("val", i_min.value());
        else if((i_max != constEnd()) && (v_max < val.toDouble()))
            insert("val", i_max.value());

        return get();
    }

    /*! add new predefined value and select it*/
    QVariant set(const QString &name, const QVariant &val, restype t = UNDEF){

        init(name, val, 1 << t);
        return sel(name);
    }

    /*! from enum menu or min, max, def, val keywords */
    QVariant sel(const QString &name){

        return insert("val", value(name)).value();
    }

    /*! from enum menu o min, max, def, val keywords */
    QVariant selm(const QString &name){

        return insertMulti("val", value(name)).value();
    }

    /*! seznam dovolenych prvku - idef je index defaultniho (pokud <0 pak zadny neni),
     *  autob jsou automaticke meze (min je prvni, max poslednim porvkem)
     */
    t_setup_entry(const QVariantList &val, const QStringList &name, int idef = 0, bool autob = false){

        init(name, val, idef, autob);
    }

    /*! seznam dovolenych prvku - idef je index defaultniho (pokud <0 pak zadny neni),
     *  autob jsou automaticke meze (min je prvni, max poslednim porvkem)
     */
    t_setup_entry(const QVariantList &val, QString unit, int idef = 0, bool autob = false){

        QStringList name;
        for(int n=0; n<val.count(); n++)
            name << val[n].toString() + unit;

        init(name, val, idef, autob);
    }

    /*! jenoducha polozka - ma jen defaultni hodnotu
     */
    t_setup_entry(const QVariant &val){

        insert("def", val);
    }
};


class t_collection_entry : public QMap<QString, t_setup_entry>, public QObject {

    Q_OBJECT
signals:
    void changed(QString title);

public:

    QVariantList vlist(){    //pomocna fce

        QVariantList t;
        return t;
    }

    QStringList slist(){    //pomocna fce

        QStringList t;
        return t;
    }

    /*! \brief inicializace polozky ALE jen pokud uz neexistuje
    */
    bool initdef(const QString &title, const t_setup_entry &val){

        if(contains(title)) return false;
        insert(title, val);
        return true;
    }


    /*! \brief export nastaveni prvku do json struktury
     *  vysledkem je seznam bjetu kde kady je jednim prvkem
     * a obsahuje podobekty jako "val", "min", "min", "max", a seznam predefinovanych hodnot
    */
    void tojson(QJsonArray &list){

        /* podpora ukladani menu vcetne multival selectu klicu a preddefinovanych hodnot */
        /* map -> json leze ale nepodporuje multivalue z toho duvodu to nemuzem udelat rovnou */

        QMap<QString, int>::const_iterator i;
        for (i = constBegin(); i != constEnd(); ++i){

            QJsonArray mval = fromVarinatList(i.value().getm());   //seznam multival hodnot
            QJsonObject options = QJsonObject::fromVariantMap(i.value());  //a mame skoro hotovo..
            options.insert("val", mval);  //..jen multival selest musim ulozit jako pole protoze to jinak prevede jen 1. hodnotu

            QJsonObject param; //kazdy parametr musi byt pojmenovan -> musi to byt objekt
            param.insert(i.key(), options);
            list.append(ji);  //pridame
        }
    }

    /*! \brief import z json */
    void fromjson(){

        /*! \todo - az budu umet export */

    }

};

#endif // RT_BASICTYPES_H
