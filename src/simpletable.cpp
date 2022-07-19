#include "simpletable.h"

#include <QDebug>

LoadTableArg::LoadTableArg(QCborValue var) {
    if (var.isArray()) {
        auto l = var.toArray();

        auto ls = l.size();

        for (int i = 0; i < ls; i++) {
            cols.emplace_back(l[i]);
        }
    } else if (auto m = var.toMap(); m.size()) {

        for (auto const& [k, v] : m) {
            cols.emplace_back(k.toString(), noo::coerce_to_real_list(v));
        }
    }
}

QStringList get_names(std::vector<LoadTableColumn> const& cols) {
    QStringList list;

    for (auto const& c : cols) {
        list << c.name;
    }

    return list;
}

QCborArray get_rows(std::vector<LoadTableColumn> const& cols) {

    QCborArray rows;

    size_t row_count = -1;

    for (auto const& c : cols) {
        row_count = std::min<size_t>(c.values.size(), row_count);
    }

    if (row_count == -1) { return {}; }

    for (size_t i = 0; i < row_count; i++) {
        QCborArray row;
        for (auto const& c : cols) {
            row << c.values[i];
        }
        rows << row;
    }

    return rows;
}
