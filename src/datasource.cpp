#include "datasource.h"

DataSource::DataSource(noo::DocumentTPtr            doc,
                       std::shared_ptr<SimpleTable> table)
    : m_table_data(table) {

    noo::TableData table_data;
    table_data.name   = table->name;
    table_data.source = table;

    m_noo_table = noo::create_table(doc, table_data);
}


SimpleTable& DataSource::table() {
    return *m_table_data;
}

std::span<double const> DataSource::get_doubles_at(int i) {
    if (!has_column(i)) return {};

    return m_table_data->get_columns()[i].as_doubles();
}
bool DataSource::has_column(int i) {
    if (i < 0) return false;
    if (i >= m_table_data->get_columns().size()) return false;

    return true;
}
