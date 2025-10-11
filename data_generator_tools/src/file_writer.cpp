#include "file_writer.hpp"

#include <stdexcept>

FileWriter::FileWriter(const std::string& filename) : filename(filename) {
	file.open(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing: " + filename);
	}
	std::cout << "Writing to file: " << filename << std::endl;
}

FileWriter::~FileWriter() {
	if (file.is_open()) {
		file.close();
		std::cout << "Finished writing to: " << filename << std::endl;
	}
}

FileWriter& FileWriter::operator<<(const std::string& value) {
	file << value;
	return *this;
}

FileWriter& FileWriter::operator<<(const char* value) {
	file << value;
	return *this;
}

FileWriter& FileWriter::operator<<(int value) {
	file << value;
	return *this;
}

FileWriter& FileWriter::operator<<(char value) {
	file << value;
	return *this;
}

void FileWriter::close() {
	if (file.is_open()) {
		file.close();
		std::cout << "Finished writing to: " << filename << std::endl;
	}
}

bool FileWriter::isOpen() const { return file.is_open(); }
