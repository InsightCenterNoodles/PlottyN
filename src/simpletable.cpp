#include "simpletable.h"


#include <unordered_set>

#include <QDebug>

#if 0

TableColumn::TableColumn(RealList&& o) : column_data(std::move(o)) { }
TableColumn::TableColumn(StringList&& o) : column_data(std::move(o)) { }
TableColumn::TableColumn(VectorList&& o) : column_data(std::move(o)) { }

size_t TableColumn::size() const {
    return std::visit([](auto const& v) { return v.size(); }, column_data);
}

auto TableColumn::as_reals() -> RealList& {
    return std::get<RealList>(column_data);
}

auto TableColumn::as_strings() -> StringList& {
    return std::get<StringList>(column_data);
}

auto TableColumn::as_vectors() -> VectorList& {
    return std::get<VectorList>(column_data);
}

auto TableColumn::as_reals() const -> RealList const& {
    return std::get<RealList>(column_data);
}

auto TableColumn::as_strings() const -> StringList const& {
    return std::get<StringList>(column_data);
}

auto TableColumn::as_vectors() const -> VectorList const& {
    return std::get<VectorList>(column_data);
}

void TableColumn::push_back(double v) {
    as_reals().push_back(v);
}
void TableColumn::push_back(std::string s) {
    as_strings().push_back(s);
}
void TableColumn::push_back(glm::vec3 v) {
    as_vectors().push_back(v);
}

double TableColumn::get_real_at(size_t i) const {
    return as_reals().at(i);
}
std::string TableColumn::get_string_at(size_t i) const {
    return as_strings().at(i);
}
glm::vec3 TableColumn::get_vector_at(size_t i) const {
    return as_vectors().at(i);
}

// =================================

SimpleTableRowQuery::SimpleTableRowQuery(int64_t             row,
                                         std::span<int64_t>  columns,
                                         TableStorage const& s) {
    m_data.reserve(columns.size());
    for (auto const& vid : columns) {
        if (vid >= s.data.size()) {
            m_data.push_back(0);
            continue;
        }

        auto& col = s.data[vid];

        if (!col.is_reals()) {
            m_data.push_back(0);
            continue;
        }

        if (row >= col.size()) {
            m_data.push_back(0);
            continue;
        }

        auto& reals = col.as_reals();

        m_data.push_back(reals[row]);
    }
}

size_t SimpleTableRowQuery::next() const {
    return m_data.empty() ? 0 : m_data.size();
}

void SimpleTableRowQuery::write_next_to(std::span<double> target) {
    if (target.size() == m_data.size()) {
        std::copy(m_data.begin(), m_data.end(), target.begin());
        m_data.clear();
    }
}


// =================================

SimpleTableBlockQuery::SimpleTableBlockQuery(std::pair<int64_t, int64_t> rows,
                                             std::span<int64_t>  columns,
                                             TableStorage const& s) {

    for (auto const& vid : columns) {
        if (vid >= s.data.size()) continue;

        auto& col = s.data[vid];

        for (size_t row = rows.first; row < rows.second; row++) {
            if (row >= col.size()) continue;

            // dumb, but works for now
            if (!col.is_reals()) {
                m_data.push_back(0);
                continue;
            }

            auto& reals = col.as_reals();

            m_data.push_back(reals[row]);
        }
    }
}

size_t SimpleTableBlockQuery::next() const {
    return m_data.empty() ? 0 : m_data.size();
}

void SimpleTableBlockQuery::write_next_to(std::span<double> target) {
    if (target.size() == m_data.size()) {
        std::copy(m_data.begin(), m_data.end(), target.begin());
        m_data.clear();
    }
}

// =================================


SimpleTable::SimpleTable(TableStorage&& s) : m_storage(std::move(s)) { }

SimpleTable::~SimpleTable() = default;

std::vector<std::string> SimpleTable::get_columns() {
    return m_storage.header;
}

size_t SimpleTable::get_num_rows() {
    if (m_storage.data.empty()) return 0;
    return m_storage.data.at(0).size();
}

noo::QueryPtr SimpleTable::get_row(int64_t row, std::span<int64_t> columns) {
    return std::make_unique<SimpleTableRowQuery>(row, columns, m_storage);
}

noo::QueryPtr SimpleTable::get_block(std::pair<int64_t, int64_t> rows,
                                     std::span<int64_t>          columns) {
    return std::make_unique<SimpleTableBlockQuery>(rows, columns, m_storage);
}

noo::QueryPtr
SimpleTable::get_selection_data(std::string_view /*selection_id*/) {
    return nullptr;
}

bool SimpleTable::request_row_insert(int64_t /*row*/,
                                     std::span<double> /*data*/) {
    return false;
}
bool SimpleTable::request_row_update(int64_t /*row*/,
                                     std::span<double> /*data*/) {
    return false;
}
bool SimpleTable::request_row_append(std::span<std::span<double>> new_data) {

    qDebug() << Q_FUNC_INFO << new_data.size();

    // auto at = get_num_rows();

    size_t max_col = get_columns().size();

    for (auto row : new_data) {
        for (size_t col_i = 0; col_i < std::min(row.size(), max_col); col_i++) {
            m_storage.data[col_i].push_back(row[col_i]);
        }
    }

    return true;
}

bool SimpleTable::request_deletion(noo::Selection const&) {
    return false;
}

noo::SelectionRef SimpleTable::get_selection(std::string_view) {
    return noo::SelectionRef();
}

bool SimpleTable::request_set_selection(std::string_view, noo::SelectionRef) {
    return false;
}

std::vector<std::string> SimpleTable::get_all_selections() {
    return {};
}

#endif

LoadTableArg::LoadTableArg(noo::AnyVarRef var) {
    if (var.has_list()) {
        auto l = var.to_vector();

        auto ls = l.size();

        for (size_t i = 0; i < ls; i++) {
            cols.emplace_back(l[i]);
        }
    } else if (auto m = var.to_map(); m.size()) {

        for (auto const& [k, v] : m) {
            cols.emplace_back(k, v.coerce_real_list().span());
        }
    }
}

SimpleTable::SimpleTable(std::string_view               _n,
                         std::vector<LoadTableColumn>&& cols)
    : noo::TableSource(nullptr), name(_n) {

    if (cols.empty()) return;

    for (auto& a : cols) {
        auto& new_a = m_columns.emplace_back();

        if (a.reals.size()) {
            new_a = std::move(a.reals);
        } else {
            new_a = std::move(a.strings);
        }
        new_a.name = a.name;
    }

    m_counter = m_columns[0].size();
    m_row_to_key_map.resize(m_counter);

    std::iota(m_row_to_key_map.begin(), m_row_to_key_map.end(), 0);

    for (size_t i = 0; i < m_row_to_key_map.size(); i++) {
        m_key_to_row_map[i] = i;
    }
}


void SimpleTable::modify_selection(std::string_view   slot,
                                   std::span<int64_t> keys,
                                   int                select_action) {

    // we DONT play with the ranges!
    // TODO: figure out how to deal with ranges


    // is there a current selection with that slot?

    noo::SelectionRef ref;

    auto slot_name = std::string(slot);

    auto iter = m_selections.find(slot_name);

    if (iter == m_selections.end()) {
        // add if not deselecting

        if (select_action < 0) return;

        ref.rows = keys;

        ask_update_selection(slot, ref);

        return;
    }

    auto const& current_selection = iter->second;

    // slot exists

    if (select_action == 0) {
        ref.rows = keys;

        ask_update_selection(slot, ref);

        return;
    }

    if (select_action < 0) {
        // diff

        // current list
        std::vector<int64_t> new_list;

        std::unordered_set<int64_t> to_del_list;
        to_del_list.insert(keys.begin(), keys.end());

        for (auto key : current_selection.rows) {
            if (to_del_list.contains(key)) { continue; }

            new_list.push_back(key);
        }

        ref.rows = new_list;

        ask_update_selection(slot, ref);

        return;
    }

    if (select_action > 0) {
        // add

        std::vector<int64_t> new_list;
        new_list.insert(new_list.end(), keys.begin(), keys.end());
        new_list.insert(new_list.end(),
                        current_selection.rows.begin(),
                        current_selection.rows.end());

        std::sort(new_list.begin(), new_list.end());
        auto last = std::unique(new_list.begin(), new_list.end());

        new_list.erase(last, new_list.end());

        ref.rows = new_list;

        ask_update_selection(slot, ref);

        return;
    }

    // well, this cant be good

    qWarning() << "Unknown selection action";
}
