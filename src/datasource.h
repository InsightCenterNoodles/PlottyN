#ifndef DATASOURCE_H
#define DATASOURCE_H

#include "simpletable.h"

#include <noo_server_interface.h>

#include <array>

template <class TableType>
class DataSource {
public:
    DataSource() = default;
    DataSource(noo::DocumentTPtr doc, std::shared_ptr<TableType> table)
        : m_table_data(table) {
        noo::TableData table_data;
        table_data.name   = table->name();
        table_data.source = table;

        m_noo_table = noo::create_table(doc, table_data);
    }

    TableType&       table() { return *m_table_data; }
    TableType const& table() const { return *m_table_data; }

    template <size_t I>
    auto column() {
        return std::span(std::get<I>(m_table_data->columns()));
    }

    //    std::span<double const> get_doubles_at(int i);
    //    bool                    has_column(int i);

    //    template <class Function, size_t N>
    //    void for_columns(Function&&            f,
    //                     std::array<int, N>    columns,
    //                     std::array<double, N> missing) {

    //        std::array<std::span<double>, N> spans;

    //        size_t num_rows = 0;

    //        for (size_t i = 0; i < N; i++) {
    //            auto column = columns[i];

    //            if (!has_column(column)) {
    //                spans[i] = std::span(&missing[i], 1);
    //                num_rows = std::max(num_rows, 1ul);
    //                continue;
    //            }

    //            spans[i] = get_doubles_at(column);
    //            num_rows = std::max(num_rows, spans[i].size());
    //        }

    //        std::array<double, N> cache;

    //        for (size_t i = 0; i < num_rows; i++) {

    //            for (size_t c = 0; c < N; c++) {
    //                auto sp_size = spans[c].size();
    //                cache[c]     = spans[c][i % sp_size];
    //            }

    //            std::apply(f, cache);
    //        }
    //    }

private:
    std::shared_ptr<TableType> m_table_data;
    noo::TableTPtr             m_noo_table;
};

#endif // DATASOURCE_H
