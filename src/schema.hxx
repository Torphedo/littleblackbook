#pragma once
#include <string>

// Interface for a struct that maps to a SQL database table
class schema {
public:
    // Get a CREATE TABLE IF NOT EXISTS SQL statement for this structure
    static const char* table_sql() noexcept;

    // Generate INSERT statement for this instance
    virtual void insert_sql(std::string& out) const noexcept = 0;
};
