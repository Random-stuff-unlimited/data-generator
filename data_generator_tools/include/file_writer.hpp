#ifndef FILE_WRITER_HPP
#define FILE_WRITER_HPP

#include <string>
#include <fstream>
#include <iostream>

class FileWriter {
private:
    std::ofstream file;
    std::string filename;

public:
    FileWriter(const std::string& filename);
    ~FileWriter();

    // Overload for different types instead of template
    FileWriter& operator<<(const std::string& value);
    FileWriter& operator<<(const char* value);
    FileWriter& operator<<(int value);
    FileWriter& operator<<(char value);

    void close();
    bool is_open() const;
};

#endif // FILE_WRITER_HPP
