#ifndef SIMPLETABLE_H
#define SIMPLETABLE_H

#include <noo_server_interface.h>

#include <QObject>

#include <unordered_set>


struct LoadTableColumn {
    QString    name;
    QCborArray values;

    LoadTableColumn() = default;

    template <class Container>
    LoadTableColumn(QString s, Container const& r) : name(std::move(s)) {
        for (auto v : r) {
            values << v;
        }
    }

    LoadTableColumn(QCborValue var) {
        auto obj = var.toMap();

        name = obj[QStringLiteral("name")].toString();

        values = obj[QStringLiteral("data")].toArray();
    }
};

struct LoadTableArg {
    std::vector<LoadTableColumn> cols;

    LoadTableArg(QCborValue var);
};


template <size_t I = 0, class... Ts>
constexpr void
fill_array(std::tuple<Ts...> const& tuple, size_t row, QCborArray& array) {
    if constexpr (I == sizeof...(Ts)) {
        return;
    } else {
        array << std::get<I>(tuple)[row];
        fill_array<I + 1>(tuple, row, array);
    }
}

template <size_t I = 0, class... Ts>
constexpr void decode_array(std::tuple<Ts...>& tuple, QCborArray const& array) {
    if constexpr (I == sizeof...(Ts)) {
        return;
    } else {
        auto& v = std::get<I>(tuple).emplace_back();
        noo::from_cbor(array[I], v);
        decode_array<I + 1>(tuple, array);
    }
}

template <size_t I = 0, class... Ts>
constexpr void
decode_array_at(std::tuple<Ts...>& tuple, size_t row, QCborArray const& array) {
    if constexpr (I == sizeof...(Ts)) {
        return;
    } else {
        auto& v = std::get<I>(tuple)[row];
        noo::from_cbor(array[I], v);
        decode_array_at<I + 1>(tuple, row, array);
    }
}


template <size_t I = 0, class... Ts>
constexpr void delete_at(std::tuple<Ts...>& tuple, size_t row) {
    if constexpr (I == sizeof...(Ts)) {
        return;
    } else {
        auto& l = std::get<I>(tuple);
        l.erase(l.begin() + row);

        delete_at<I + 1>(tuple, row);
    }
}

template <size_t I = 0, class... Ts>
constexpr void clear_all(std::tuple<Ts...>& tuple) {
    if constexpr (I == sizeof...(Ts)) {
        return;
    } else {
        auto& l = std::get<I>(tuple);
        l.clear();

        clear_all<I + 1>(tuple);
    }
}

template <class... Args>
class SpecificTable : public noo::ServerTableDelegate {
    QString         m_name;
    QStringList     m_headers;
    QVector<qint64> m_key_list;

    using TupleType = std::tuple<QVector<Args>...>;
    TupleType m_data_list;

    std::unordered_map<quint64, quint64> m_key_to_row_map;

    static constexpr inline size_t m_num_cols = std::tuple_size_v<TupleType>;

    QHash<QString, noo::Selection> m_selections;

    size_t m_counter = 0;

    QCborArray m_cached_keys;
    QCborArray m_cached_rows;

    uint64_t next_counter() {
        auto save = m_counter;
        m_counter++;
        return save;
    }

    void rebuild_cache() {
        m_cached_keys.clear();
        m_cached_rows.clear();

        for (auto k : m_key_list) {
            m_cached_keys << k;
        }

        auto num_rows = m_key_list.size();

        for (auto ri = 0; ri < num_rows; ri++) {
            QCborArray row;

            fill_array(m_data_list, ri, row);

            m_cached_rows << row;
        }
    }

    QCborArray get_row(int i) const {
        QCborArray arr;
        fill_array(m_data_list, i, arr);
        return arr;
    }

    std::pair<QCborArray, QCborArray>
    common_insert(QCborArray const& new_rows) {
        QCborArray ret_keys;
        QCborArray ret_rows;

        for (int i = 0; i < new_rows.size(); i++) {
            auto key = next_counter();
            m_key_list << key;
            m_cached_keys << (qint64)key;
            ret_keys << (qint64)key;

            auto row = std::get<0>(m_data_list).size();

            m_key_to_row_map[key] = row;

            decode_array(m_data_list, new_rows[i].toArray());

            m_cached_rows << get_row(row);
            ret_rows << m_cached_rows.last();
        }

        return { ret_keys, ret_rows };
    }

public:
    SpecificTable(QStringList headers, QCborArray init_rows)
        : noo::ServerTableDelegate(nullptr) {
        m_headers = headers;

        while (m_headers.size() > m_num_cols) {
            m_headers.pop_back();
        }
        while (m_headers.size() < m_num_cols) {
            m_headers << QString();
        }

        common_insert(init_rows);
    }

    SpecificTable(QString name, std::vector<LoadTableColumn> const& raw_cols)
        : noo::ServerTableDelegate(nullptr) {
        m_name = name;

        auto local_c = raw_cols;

        while (local_c.size() > m_num_cols) {
            local_c.pop_back();
        }
        while (local_c.size() < m_num_cols) {
            local_c.push_back(LoadTableColumn());
        }

        for (auto const& l : local_c) {
            m_headers << l.name;
        }

        size_t num_rows = local_c.at(0).values.size();

        QCborArray datas;

        for (auto i = 0; i < num_rows; i++) {
            QCborArray row;

            for (auto hi = 0; hi < m_headers.size(); hi++) {
                row << local_c.at(hi).values.at(i);
            }

            datas << row;
        }

        common_insert(datas);
    }

    auto name() const { return m_name; }

    auto const& columns() { return m_data_list; }

    auto get_all_keys() const { return std::span(m_key_list); }

    template <size_t I>
    auto get_column_at_key(int key) const {
        return std::get<I>(m_data_list)[m_key_to_row_map.at(key)];
    }

    QStringList get_headers() override { return m_headers; }
    std::pair<QCborArray, QCborArray> get_all_data() override {
        return { m_cached_keys, m_cached_rows };
    }
    QList<noo::Selection> get_all_selections() override {
        return m_selections.values();
    }

    void handle_insert(QCborArray const& new_rows) override {
        auto [keys, rows] = common_insert(new_rows);

        emit table_row_updated(keys, rows);
    }

    void handle_update(QCborArray const& raw_keys,
                       QCborArray const& raw_rows) override {

        qDebug() << Q_FUNC_INFO << raw_keys.toCborValue().toDiagnosticNotation()
                 << raw_rows.toCborValue().toDiagnosticNotation();

        QCborArray fixed_rows;

        for (int i = 0; i < raw_rows.size(); i++) {
            auto actual_row_iter =
                m_key_to_row_map.find(raw_keys[i].toInteger(-1));

            if (actual_row_iter == m_key_to_row_map.end()) continue;

            auto actual_row = actual_row_iter->second;

            auto raw_row = raw_rows[i].toArray();

            while (raw_row.size() < m_headers.size()) {
                raw_row << QCborValue();
            }

            while (raw_row.size() > m_headers.size()) {
                raw_row.pop_back();
            }

            m_cached_rows[actual_row] = raw_row;

            decode_array_at(m_data_list, actual_row, raw_row);

            fixed_rows << raw_row;
        }

        emit table_row_updated(raw_keys, fixed_rows);
    }

    void handle_deletion(QCborArray const& keys) override {
        // take array, sort, and process in reverse to avoid movement issues

        auto list = noo::coerce_to_int_list(keys.toCborValue());

        // turn into a row list

        std::vector<int64_t> row_ids;

        for (auto key : list) {
            row_ids.push_back(m_key_to_row_map[key]);
        }

        std::sort(row_ids.begin(), row_ids.end());
        row_ids.erase(std::unique(row_ids.begin(), row_ids.end()),
                      row_ids.end());

        // this is a horrible way to do this;
        for (auto iter = row_ids.rbegin(); iter != row_ids.rend(); ++iter) {
            size_t row = *iter;

            m_cached_keys.erase(m_cached_keys.begin() + row);
            m_cached_rows.erase(m_cached_rows.begin() + row);

            m_key_list.erase(m_key_list.begin() + row);

            delete_at(m_data_list, row);
        }

        emit table_row_deleted(keys);
    }

    void handle_reset() override {
        clear_all(m_data_list);
        m_key_list.clear();
        rebuild_cache();

        emit table_reset();
    }

    void handle_set_selection(noo::Selection const& s) override {
        m_selections[s.name] = s;
        emit table_selection_updated(s);

        if (s.row_ranges.empty() and s.rows.empty()) {
            m_selections.remove(s.name);
        }
    }

    void
    modify_selection(QString slot, std::span<int64_t> keys, int select_action) {
        // we DONT play with the ranges!
        // TODO: figure out how to deal with ranges


        // is there a current selection with that slot?

        noo::Selection ref;
        ref.name = slot;

        auto iter = m_selections.find(slot);

        if (iter == m_selections.end()) {
            // add if not deselecting

            if (select_action < 0) return;

            ref.rows = QVector<int64_t>(keys.begin(), keys.end());


            handle_set_selection(ref);

            return;
        }

        auto const& current_selection = iter.value();

        // slot exists

        if (select_action == 0) {
            ref.rows = QVector<int64_t>(keys.begin(), keys.end());

            handle_set_selection(ref);

            return;
        }

        if (select_action < 0) {
            // diff

            // current list
            QVector<int64_t> new_list;

            std::unordered_set<int64_t> to_del_list;
            to_del_list.insert(keys.begin(), keys.end());

            for (auto key : current_selection.rows) {
                if (to_del_list.contains(key)) { continue; }

                new_list.push_back(key);
            }

            ref.rows = new_list;

            handle_set_selection(ref);

            return;
        }

        if (select_action > 0) {
            // add

            QVector<int64_t> new_list;
            new_list << QVector<int64_t>(keys.begin(), keys.end());
            new_list << current_selection.rows;

            std::sort(new_list.begin(), new_list.end());
            auto last = std::unique(new_list.begin(), new_list.end());

            new_list.erase(last, new_list.end());

            ref.rows = new_list;

            handle_set_selection(ref);

            return;
        }

        // well, this cant be good

        qWarning() << "Unknown selection action";
    }
};

#endif // SIMPLETABLE_H
