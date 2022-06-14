#ifndef SIMPLETABLE_H
#define SIMPLETABLE_H

#include <noo_server_interface.h>

#include <QObject>

struct LoadTableColumn {
    QString         name;
    QVector<double> reals;
    QStringList     strings;

    LoadTableColumn() = default;

    LoadTableColumn(QString s, std::span<double const> r)
        : name(std::move(s)), reals(r.begin(), r.end()) { }

    LoadTableColumn(QString s, QVector<double>&& r)
        : name(std::move(s)), reals(std::move(r)) { }

    LoadTableColumn(QString s, QStringList&& r)
        : name(std::move(s)), strings(std::move(r)) { }

    LoadTableColumn(QCborValue var) {
        auto obj = var.toMap();

        name = obj[QStringLiteral("name")].toString();

        reals = noo::coerce_to_real_list(obj[QStringLiteral("data")]);
    }
};

struct LoadTableArg {
    std::vector<LoadTableColumn> cols;

    LoadTableArg(QCborValue var);
};

class SimpleTable : public noo::TableSource {
public:
    QString name;

    SimpleTable(QString n, std::vector<LoadTableColumn>&& cols);

    void modify_selection(QString, std::span<int64_t> keys, int select_action);
};

#endif // SIMPLETABLE_H
