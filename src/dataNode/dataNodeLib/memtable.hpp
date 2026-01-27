#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <unordered_map>

using Value = std::variant<u_int64_t, int64_t, double, std::string>;
enum class ColumnType{
    INT64, 
    UINT64, 
    DOUBLE, 
    STRING 
};

struct Column{
    ColumnType type;
    std::vector<Value> data;
    void append(Value val){
        if (val.index() != static_cast<size_t>(type)){
            throw std::invalid_argument("Value type does not match column type");
        }
        this->data.push_back(val);
    }
};

struct ColumnarSeries{
    std::vector<u_int64_t> timestamps;
    std::unordered_map<std::string, Column> columns;
    ColumnarSeries(std::unordered_map<std::string, ColumnType>& col_names){
        this->columns = std::unordered_map<std::string, Column>();
        for (const auto& [name, type] : col_names){
            this->columns[name] = Column{type, std::vector<Value>()};
        }
    }
    // return memory size used by new row
    void insert(u_int64_t timestamp, const std::unordered_map<std::string, Value>& row){
        timestamps.push_back(timestamp);
        for (const auto& [col_name, val] : row){
            if (columns.find(col_name) == columns.end()){
                throw std::invalid_argument("Column does not exist: " + col_name);
            }
            try {
                columns[col_name].append(val);
            } catch (const std::invalid_argument& e){
                throw e;
            }
        }
    }
};

struct KeyField{
    std::string name;
    Value value;
};

struct KeySeries{
    std::vector<KeyField> fields;
    u_int64_t capacity()const{
        u_int64_t total_size = 0;
        for (const auto& f : fields){
            total_size += f.name.size();
            if (std::holds_alternative<u_int64_t>(f.value)) {
                total_size += sizeof(u_int64_t);
            } else if (std::holds_alternative<int64_t>(f.value)) {
                total_size += sizeof(int64_t);
            } else if (std::holds_alternative<double>(f.value)) {
                total_size += sizeof(double);
            } else if (std::holds_alternative<std::string>(f.value)) {
                total_size += std::get<std::string>(f.value).size();
            }
        }
        return total_size;
    }
    bool operator==(const KeySeries& other) const {
        if (fields.size() != other.fields.size()) return false;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (fields[i].name != other.fields[i].name || fields[i].value != other.fields[i].value)
                return false;
        }
        return true;
    }
};

struct KeySeriesHash {
    std::size_t operator()(const KeySeries& key) const {
        std::size_t h = 0;
        for (const auto& f : key.fields) {
            std::hash<std::string> hash_str;
            std::hash<int64_t> hash_int;
            std::hash<double> hash_double;

            std::size_t hv = 0;
            if (std::holds_alternative<u_int64_t>(f.value)) {
                hv = hash_int(std::get<u_int64_t>(f.value));
            } else if (std::holds_alternative<int64_t>(f.value)) {
                hv = hash_double(std::get<int64_t>(f.value));
            } else if (std::holds_alternative<double>(f.value)) {
                hv = hash_double(std::get<double>(f.value));
            } else if (std::holds_alternative<std::string>(f.value)) {
                hv = hash_str(std::get<std::string>(f.value));
            }

            h ^= hv + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash_str(f.name) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

struct Row{
    u_int64_t timestamp;
    std::vector<Value> data;
};

class Memtable{
private:
    u_int64_t current_capacity;
    std::vector<Row> table;
    // std::unordered_map<KeySeries, ColumnarSeries, KeySeriesHash> table;
    std::unordered_map<std::string, ColumnType> column_names;
    u_int64_t column_name_size;
    bool mut;
    u_int64_t var_data_size;
public:
    Memtable(std::unordered_map<std::string, ColumnType> column_names){
        this->table = std::vector<Row>();
        this->column_names = column_names;
        this->column_name_size = 0;
        this->var_data_size = 0;
        for (const auto& [name, _] : column_names){
            this->column_name_size += name.size();
        }
        this->current_capacity = this->column_name_size;
        this->current_capacity += sizeof(ColumnType) * column_names.size();
        this->mut = true;
    }
    ~Memtable() = default;
    void write(const KeySeries& key, u_int64_t ts, const std::unordered_map<std::string, Value>& fields) {
        if (!this->mut){
            throw std::runtime_error("Memtable is frozen and cannot be modified");
        }
        Row row = {ts, std::vector<Value>()};
        for (const auto& [col_name, _] : this->column_names){
            if (fields.find(col_name) == fields.end()){
                throw std::invalid_argument("Missing column in fields: " + col_name);
            }
            row.data.push_back(fields.at(col_name));
        }
        this->table.push_back(row);
        addRowSize(fields);
    }
    u_int64_t capacity()const {
        return this->current_capacity;
    }
    u_int64_t varCapacity()const {
        return this->var_data_size;
    }
    u_int64_t columnNameSize()const {
        return this->column_name_size;
    }
    void addRowSize(const std::unordered_map<std::string, Value>& fields) {
        u_int64_t row_size = sizeof(u_int64_t); // timestamp size
        u_int64_t var_row_size = 0;
        for (const auto& [col_name, val] : fields){
            if (std::holds_alternative<u_int64_t>(val)) {
                row_size += sizeof(u_int64_t);
            } else if (std::holds_alternative<int64_t>(val)) {
                row_size += sizeof(int64_t);
            } else if (std::holds_alternative<double>(val)) {
                row_size += sizeof(double);
            } else if (std::holds_alternative<std::string>(val)) {
                row_size += std::get<std::string>(val).size();
                var_row_size += std::get<std::string>(val).size();
            }
        }
        this->current_capacity += row_size;
        this->var_data_size += var_row_size;
    }
    void freeze(){
        this->mut = false;
    }
    const std::vector<Row>& getTable() const {
        return this->table;
    }
    const std::unordered_map<std::string, ColumnType>& getColumnNames() const{
        return this->column_names;
    }
};