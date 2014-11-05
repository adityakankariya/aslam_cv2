#ifndef ASLAM_CHANNEL_SERIALIZATION_H_
#define ASLAM_CHANNEL_SERIALIZATION_H_

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include <glog/logging.h>
#include <Eigen/Dense>
#include <opencv2/core/core.hpp>

namespace aslam {
namespace internal {

struct HeaderInformation {
  uint32_t rows;
  uint32_t cols;
  uint32_t depth;
  uint32_t channels; ///< Needed for opencv support
  size_t size() const;
  bool serializeToBuffer(char* buffer, size_t offset) const;
  bool deSerializeFromBuffer(const char* const bufferIn, size_t offset);
};

template<typename SCALAR>
void makeHeaderInformation(int rows, int cols, int channels,
                           HeaderInformation* headerInformation) {
  CHECK_NOTNULL(headerInformation);
  headerInformation->rows = rows;
  headerInformation->cols = cols;
  headerInformation->channels = channels;
  headerInformation->depth = cv::DataType<SCALAR>::depth;
}

template<typename SCALAR>
bool serializeToString(const char* const matrixData,
                       int rows, int cols, int channels, std::string* string) {
  CHECK_GE(rows, 0);
  CHECK_GE(cols, 0);
  CHECK_NOTNULL(string);
  HeaderInformation header;
  makeHeaderInformation<SCALAR>(rows, cols, channels, &header);
  size_t matrixSize = sizeof(SCALAR) * rows * cols * channels;
  size_t totalSize = matrixSize + header.size();

  CHECK_GT(totalSize, 0u);

  string->resize(totalSize);
  char* buffer = &(*string)[0];
  bool success = header.serializeToBuffer(buffer, 0);
  if (!success) {
    LOG(ERROR) << "Failed to serialize header";
    return false;
  }
  size_t offset = header.size();
  memcpy(buffer + offset, matrixData, matrixSize);
  return true;
}

template<typename SCALAR>
bool serializeToBuffer(const char* const matrixData, int rows, int cols,
                       int channels, char** buffer, size_t* totalSize) {
  CHECK_NOTNULL(totalSize);
  CHECK_NOTNULL(buffer);
  HeaderInformation header;
  makeHeaderInformation<SCALAR>(rows, cols, channels, &header);
  size_t matrixSize = sizeof(SCALAR) * rows * cols * channels;
  *totalSize = matrixSize + header.size();

  *buffer = new char[*totalSize];
  bool success = header.serializeToBuffer(*buffer, 0);
  if (!success) {
    delete[] *buffer;
    *buffer = nullptr;
    return false;
  }
  size_t offset = header.size();
  memcpy(*buffer + offset, matrixData, matrixSize);
  return true;
}

template<typename SCALAR, int ROWS, int COLS>
bool serializeToBuffer(const Eigen::Matrix<SCALAR, ROWS, COLS>& matrix,
                       char** buffer, size_t* size) {
  const char* const matrixData = reinterpret_cast<const char*>(matrix.data());
  // Eigen matrices have only one channel
  constexpr int numChannels = 1;
  return serializeToBuffer<SCALAR>(matrixData, matrix.rows(), matrix.cols(),
                                   numChannels, buffer, size);
}

template<typename SCALAR, int ROWS, int COLS>
bool serializeToString(const Eigen::Matrix<SCALAR, ROWS, COLS>& matrix,
                       std::string* string) {
  const char* const matrixData = reinterpret_cast<const char*>(matrix.data());
  // Eigen matrices have only one channel
  constexpr int numChannels = 1;
  return serializeToString<SCALAR>(matrixData, matrix.rows(), matrix.cols(),
                                   numChannels, string);
}

template<typename SCALAR, int ROWS, int COLS>
bool deSerializeFromBuffer(const char* const buffer, size_t size,
                           Eigen::Matrix<SCALAR, ROWS, COLS>* matrix) {
  CHECK_NOTNULL(matrix);
  HeaderInformation header;
  CHECK_GE(size, header.size());
  bool success = header.deSerializeFromBuffer(buffer, 0);
  if (!success) {
    LOG(ERROR) << "Failed to deserialize header from string: " <<
        std::string(buffer, size);
    return false;
  }
  if (ROWS != Eigen::Dynamic) {
    CHECK_EQ(header.rows, static_cast<uint32_t>(ROWS));
  }
  if (COLS != Eigen::Dynamic) {
    CHECK_EQ(header.cols, static_cast<uint32_t>(COLS));
  }
  CHECK_EQ(header.depth, cv::DataType<SCALAR>::depth);
  CHECK_EQ(1u, header.channels) << "Eigen matrices must have one channel.";

  if (ROWS == Eigen::Dynamic && COLS == Eigen::Dynamic) {
    matrix->resize(header.rows, header.cols);
  } else if (ROWS == Eigen::Dynamic && COLS != Eigen::Dynamic) {
    matrix->resize(header.rows, Eigen::NoChange);
  } else if (ROWS != Eigen::Dynamic && COLS == Eigen::Dynamic) {
    matrix->resize(Eigen::NoChange, header.cols);
  }

  size_t matrix_size = sizeof(SCALAR) * matrix->rows() * matrix->cols();
  size_t total_size = matrix_size + header.size();
  CHECK_EQ(size, total_size);
  memcpy(matrix->data(), buffer + header.size(), matrix_size);
  return true;
}

template<typename SCALAR, int ROWS, int COLS>
bool deSerializeFromString(const std::string& string,
                           Eigen::Matrix<SCALAR, ROWS, COLS>* matrix) {
  CHECK_NOTNULL(matrix);
  return deSerializeFromBuffer(string.data(), string.size(), matrix);
}

bool serializeToString(const cv::Mat& image,
                       std::string* string);

bool deSerializeFromString(const std::string& string,
                           cv::Mat* image);

bool deSerializeFromBuffer(const char* const buffer, size_t size,
                           cv::Mat* image);

bool serializeToBuffer(const cv::Mat& matrix,
                       char** buffer, size_t* size);

template<typename SCALAR>
bool serializeToString(const SCALAR& value, std::string* string) {
  CHECK_NOTNULL(string);
  std::stringstream string_stream;
  string_stream << value;
  std::string values_as_string = string_stream.str();
  string->swap(values_as_string);
  return true;
}

template<typename SCALAR>
bool deSerializeFromString(const std::string& string, SCALAR* value) {
  CHECK_NOTNULL(value);
  CHECK(!string.empty());
  (*value) = static_cast<SCALAR>(std::stoll(string));
  return true;
}

template<typename SCALAR>
bool serializeToBuffer(const SCALAR& value, char** buffer, size_t* size) {
  CHECK_NOTNULL(buffer);
  CHECK_NOTNULL(size);
  CHECK_EQ(sizeof(SCALAR),*size);

  *buffer = new char[*size];
  memcpy(*buffer, &value, *size);
  return true;
}
template<typename SCALAR>
bool deSerializeFromBuffer(const char* const buffer, size_t size, SCALAR* value) {
  CHECK_NOTNULL(value);
  CHECK_EQ(size, sizeof(SCALAR));
  memcpy(&value, buffer, size);
  return true;
}

}  // namespace internal
}  // namespace aslam

#endif  // ASLAM_CHANNEL_SERIALIZATION_H_
