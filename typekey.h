#ifndef TYPEKEY_H
#define TYPEKEY_H

#include <QMap>
//#include <cstddef>

#define zkey(x) q_macro_factory::_aa55<0>(#x, sizeof(x))

namespace q_macro_factory {

template<int a>
auto _aa55(const char* y, std::size_t) -> QString
{
    static QMap<const char*, QString> m;
    static int k = 0;
    if(m.contains(y)) return m.value(y);
    auto v = QString::number(k++);
    m.insert(y, v);
    return v;
}
} // ENDOF namespace q_macro_factory
/*
#define key(x) _createkey<0>(#x, sizeof(x))

template<int a>
auto _createkey(const char* y, std::size_t) -> QString
{
    static QMap<const char*, QString> m;
    static int k = 0;
    if(m.contains(y)) return m.value(y);
    auto v = QString::number(++k)
                 m.insert(y, v);
    return v;
}
*/

#endif // TYPEKEY_H
