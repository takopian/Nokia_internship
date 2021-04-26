#pragma once
#include <vector>
#include <variant>
#include <string>
#include <algorithm>
#include <exception>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>

struct TableExpression{

    using arg_type = std::variant<int64_t, std::pair<std::string, int64_t>>;
    std::string op;
    arg_type left_arg;
    arg_type right_arg;

    explicit TableExpression(const std::string& line) noexcept(false){

        auto first_alpha = std::find_if(line.begin(), line.end(), [](char c) { return std::isalpha(c);});
        auto first_digit = std::find_if(line.begin(), line.end(), [](char c) { return std::isdigit(c);});

        if (first_alpha == line.end()) {
            left_arg = parse_int(first_digit);
            op = *first_digit;
            first_digit++;
            right_arg = parse_int(first_digit);
            return;
        }

        if (first_alpha > first_digit) {
            left_arg = parse_int(first_digit);
            op = *first_digit;
            right_arg = parse_ref(first_alpha);
            return;
        }
        left_arg = parse_ref(first_alpha);
        op = *first_alpha;
        ++first_alpha;
        auto next_alpha = std::find_if(first_alpha, line.end(), [](char c) { return std::isalpha(c);});
        auto next_digit = std::find_if(first_alpha, line.end(), [](char c) { return std::isdigit(c);});

        if (next_alpha == line.end()) {
            right_arg = parse_int(next_digit);
            return;
        }
        right_arg = parse_ref(next_alpha);
    }

    static int64_t parse_int(std::string::const_iterator & stream) {
        std::string parsed_int;
        while (std::isdigit(*stream)){
            parsed_int += *stream;
            stream++;
        }
        int64_t result = std::stoi(parsed_int);
        return result;
    }

    static std::pair<std::string, int64_t> parse_ref(std::string::const_iterator & stream) {
        std::string parsed_col;
        std::string parsed_row;
        while (std::isalpha(*stream)){
            parsed_col += *stream;
            stream++;
        }
        while (std::isdigit(*stream)){
            parsed_row += *stream;
            stream++;
        }
        return {parsed_col, std::stoi(parsed_row)};
    }
};


class Table {
    using column_type = std::string;
    using row_type = int64_t ;
    using cell_type = std::variant<int64_t, TableExpression>;

public:
    explicit Table(const std::string& filename) {

        std::ifstream file_stream(filename);
        if (!file_stream.is_open()){
            throw std::runtime_error("Error occurred while reading file");
        }
        std::string line;
        std::getline(file_stream, line, '\n');
        auto cols = init_cols(line);
        columns = cols;

        while (std::getline(file_stream, line, '\n')){

            size_t col_ind = 0;
            std::stringstream row(line);
            std::string row_num;
            std::getline(row, row_num, ',');
            std::string cell;
            std::map<column_type, cell_type> parsed_row;

            while (std::getline(row, cell, ',')){
                if (col_ind >= columns.size()){
                    break;
                }

                if (!cell.empty()) {

                    auto value = cell_type();
                    if (cell[0] == '='){
                        value = TableExpression(cell);
                    } else {
                        auto iter = cell.cbegin();
                        value = TableExpression::parse_int(iter);
                    }
                    parsed_row.insert({cols[col_ind], value});
                    col_ind += 1;
                    continue;
                }
            }

            auto iter = row_num.cbegin();
            auto parsed_row_num = TableExpression::parse_int(iter);
            rows.push_back(parsed_row_num);
            data.insert({parsed_row_num, parsed_row});
        }
    }

    void print_table() {
        for (const auto& col : columns) {
            std::cout << ',' << col;
        }
        std::cout << '\n';
        for (auto row : rows) {
            std::cout << row;
            for (const auto& col : columns) {
                int64_t result;
                try{
                    result = eval_expr(data[row][col]);
                    std::cout  << ',' << result;
                } catch (std::out_of_range&  e) {
                    std::cout << ',' << e.what();
                }
                    catch (std::overflow_error& e) {
                    std::cout << ',' << e.what();
                }
                    catch (std::runtime_error& e) {
                    std::cout << ',' << e.what();
                }
            }
            std::cout << '\n';
        }
    }

private:

    int64_t eval_expr(cell_type& cell) {
        size_t cell_type_ind = cell.index();
        return cell_type_ind == 0 ? eval_expr(std::get<0>(cell)) : eval_expr(std::get<1>(cell), 0);
    }

    int64_t eval_expr(int64_t value) {
        return value;
    }

    int64_t eval_expr(const TableExpression& expr, size_t count_of_calls) {
        if (count_of_calls >= 500) {
            throw std::runtime_error("Cyclic dep.");
        }
        size_t left_type_ind = expr.left_arg.index();
        size_t right_type_ind = expr.right_arg.index();

        int64_t left_val = left_type_ind == 0 ? eval_expr(std::get<0>(expr.left_arg))
                : eval_expr(std::get<1>(expr.left_arg), count_of_calls + 1);
        int64_t right_val = right_type_ind == 0 ? eval_expr(std::get<0>(expr.right_arg))
                : eval_expr(std::get<1>(expr.right_arg), count_of_calls + 1);

        if (expr.op == "+") {
            return left_val + right_val;
        }
        if (expr.op == "-") {
            return left_val - right_val;
        }
        if (expr.op == "*") {
            return left_val * right_val;
        }
        if (right_val == 0) {
            throw std::overflow_error("Divide by zero.");
        }
        return left_val / right_val;
    }

    int64_t eval_expr(const std::pair<std::string, int64_t>& ref, size_t count_of_calls){
        if (count_of_calls >= 500) {
            throw std::runtime_error("Cyclic dep.");
        }
        try{auto value = data.at(ref.second).at(ref.first);
            size_t value_type_ind = value.index();
            if (value_type_ind == 0) {
                return eval_expr(std::get<0>(value));
            }
            return eval_expr(std::get<1>(value), count_of_calls + 1);
        } catch (std::out_of_range& e) {
            std::string msg = "Wrong cell number: ";
            msg += ref.first;
            msg += std::to_string(ref.second);
            throw std::out_of_range(msg);
        }

    }


    std::vector<column_type> init_cols(const std::string& line){
        std::stringstream header_row(line);
        std::string col_name;
        std::vector<column_type> cols;
        while (std::getline(header_row, col_name, ',')){
            if (!col_name.empty()) {
                cols.push_back(col_name);
            }
        }
        return cols;
    }


    std::vector<row_type> rows;
    std::vector<column_type> columns;
    std::map<row_type , std::map<column_type, cell_type>> data;
};
