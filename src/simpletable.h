#ifndef SIMPLETABLE_H
#define SIMPLETABLE_H

#include <noo_server_interface.h>

#include <QObject>

struct LoadTableColumn {
    std::string         name;
    std::vector<double> reals;

    LoadTableColumn() = default;

    LoadTableColumn(std::string s, std::span<double const> r)
        : name(std::move(s)), reals(r.begin(), r.end()) { }

    LoadTableColumn(std::string s, std::vector<double>&& r)
        : name(std::move(s)), reals(std::move(r)) { }

    LoadTableColumn(noo::AnyVarRef var) {
        auto obj = var.to_map();

        name = std::string(obj["name"].to_string());

        auto ls = obj["data"].coerce_real_list();

        reals = std::vector<double>(ls.begin(), ls.end());
    }
};

struct LoadTableArg {
    std::vector<LoadTableColumn> cols;

    LoadTableArg(noo::AnyVarRef var);
};

class SimpleTable : public noo::TableSource {
public:
    std::string name;

    SimpleTable(std::string_view _n, std::vector<LoadTableColumn>&& cols);
};

#endif // SIMPLETABLE_H
