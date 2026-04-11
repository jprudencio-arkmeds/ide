#pragma once
#include <QString>
#include <vector>

struct Value {
    enum class Type { INT, REAL, CHAR, STRING, VOID };
    Type type = Type::INT;

    long long intVal  = 0;
    double    realVal = 0.0;
    QChar     charVal = QChar(0);
    QString   strVal;

    static Value ofInt(long long v)         { Value r; r.type=Type::INT;    r.intVal=v;  return r; }
    static Value ofReal(double v)           { Value r; r.type=Type::REAL;   r.realVal=v; return r; }
    static Value ofChar(QChar v)            { Value r; r.type=Type::CHAR;   r.charVal=v; return r; }
    static Value ofString(const QString& v) { Value r; r.type=Type::STRING; r.strVal=v;  return r; }
    static Value ofVoid()                   { Value r; r.type=Type::VOID;               return r; }

    long long toInt() const {
        switch (type) {
            case Type::INT:    return intVal;
            case Type::REAL:   return static_cast<long long>(realVal);
            case Type::CHAR:   return charVal.toLatin1();
            case Type::STRING: return strVal.toLongLong();
            default:           return 0;
        }
    }
    double toReal() const {
        switch (type) {
            case Type::INT:    return static_cast<double>(intVal);
            case Type::REAL:   return realVal;
            case Type::STRING: return strVal.toDouble();
            default:           return 0.0;
        }
    }
    bool toBool() const {
        switch (type) {
            case Type::INT:    return intVal  != 0;
            case Type::REAL:   return realVal != 0.0;
            case Type::CHAR:   return charVal != QChar(0);
            case Type::STRING: return !strVal.isEmpty();
            default:           return false;
        }
    }
    QString toString() const {
        switch (type) {
            case Type::INT:    return QString::number(intVal);
            case Type::REAL: {
                QString s = QString::number(realVal, 'g', 10);
                return s;
            }
            case Type::CHAR:   return QString(charVal);
            case Type::STRING: return strVal;
            default:           return "";
        }
    }

    bool isNumeric() const { return type == Type::INT || type == Type::REAL; }
};
