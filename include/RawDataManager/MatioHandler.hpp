#ifndef MATIO_CPP17_WRAPPER_H
#define MATIO_CPP17_WRAPPER_H

#include <iostream>
#include <matio.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class MatioCpp17Wrapper {
public:
  // 文件打开模式枚举
  enum class OpenMode { ReadOnly, ReadWrite };

  // 构造函数，用于创建或打开一个 .mat 文件
  explicit MatioCpp17Wrapper(const std::string &filename, OpenMode mode = OpenMode::ReadWrite) {
    openMatFile(filename, mode);
  }

  // 禁用拷贝构造和拷贝赋值
  MatioCpp17Wrapper(const MatioCpp17Wrapper &) = delete;
  MatioCpp17Wrapper &operator=(const MatioCpp17Wrapper &) = delete;

  // 启用移动构造和移动赋值
  MatioCpp17Wrapper(MatioCpp17Wrapper &&other) noexcept = default;
  MatioCpp17Wrapper &operator=(MatioCpp17Wrapper &&other) noexcept = default;

  // 更优雅的资源管理，通过析构函数来自动关闭文件
  ~MatioCpp17Wrapper() = default;

  // 写入矩阵数据到 MAT 文件
  template <typename T>
  void writeMatrix(const std::string &varName, const std::vector<T> &data, size_t rows,
                   size_t cols) {
    if (data.size() != rows * cols) {
      throw std::invalid_argument("Data size does not match specified dimensions.");
    }

    size_t dims[2] = {rows, cols};
    auto matvar = std::unique_ptr<matvar_t, decltype(&Mat_VarFree)>(
        Mat_VarCreate(varName.c_str(), getMatClass<T>(), getMatType<T>(), 2, dims,
                      (void *)data.data(), 0),
        Mat_VarFree);

    if (!matvar) {
      throw std::runtime_error("Failed to create MAT variable: " + varName);
    }

    if (Mat_VarWrite(matfp_.get(), matvar.get(), MAT_COMPRESSION_NONE) != 0) {
      throw std::runtime_error("Failed to write variable to MAT file: " + varName);
    }
  }

  // 读取矩阵数据从 MAT 文件
  template <typename T> std::optional<std::vector<T>> readMatrix(const std::string &varName) {
    matvar_t *matvar = Mat_VarRead(matfp_.get(), varName.c_str());
    if (!matvar) {
      std::cerr << "Failed to read variable: " << varName << std::endl;
      return std::nullopt;
    }

    if (matvar->data_type != getMatType<T>() || matvar->rank != 2) {
      std::cerr << "Unsupported variable type or dimensions for: " << varName << std::endl;
      Mat_VarFree(matvar);
      return std::nullopt;
    }

    size_t rows = matvar->dims[0];
    size_t cols = matvar->dims[1];
    size_t dataSize = rows * cols;
    std::vector<T> data(dataSize);
    std::memcpy(data.data(), matvar->data, dataSize * sizeof(T));

    Mat_VarFree(matvar);
    return data;
  }

  // 写入字符串到 MAT 文件
  void writeString(const std::string &varName, const std::string &str) {
    size_t dims[2] = {1, str.size()};
    auto matvar = std::unique_ptr<matvar_t, decltype(&Mat_VarFree)>(
        Mat_VarCreate(varName.c_str(), MAT_C_CHAR, MAT_T_UTF8, 2, dims, (void *)str.c_str(), 0),
        Mat_VarFree);

    if (!matvar) {
      throw std::runtime_error("Failed to create MAT variable: " + varName);
    }

    if (Mat_VarWrite(matfp_.get(), matvar.get(), MAT_COMPRESSION_NONE) != 0) {
      throw std::runtime_error("Failed to write variable to MAT file: " + varName);
    }
  }

  // 读取字符串从 MAT 文件
  std::optional<std::string> readString(const std::string &varName) {
    matvar_t *matvar = Mat_VarRead(matfp_.get(), varName.c_str());
    if (!matvar) {
      std::cerr << "Failed to read variable: " << varName << std::endl;
      return std::nullopt;
    }

    if (matvar->data_type != MAT_T_UTF8) {
      std::cerr << "Unsupported variable type for: " << varName << std::endl;
      Mat_VarFree(matvar);
      return std::nullopt;
    }

    std::string result(static_cast<char *>(matvar->data), matvar->nbytes);
    Mat_VarFree(matvar);
    return result;
  }

  // 写入头信息到 MAT 文件
  void writeHeader(const std::unordered_map<std::string, double> &headerInfo) {
    for (const auto &[key, value] : headerInfo) {
      writeMatrix(key, std::vector<double>{value}, 1, 1);
    }
  }

  // 写入 I/Q 数据到 MAT 文件
  void writeIQData(const std::string &varNameI, const std::string &varNameQ,
                   const std::vector<int16_t> &data) {
    if (data.size() % 2 != 0) {
      throw std::invalid_argument("Input data size must be even for I/Q representation.");
    }

    size_t numRows = data.size() / 2;
    std::vector<double> I_Data(numRows);
    std::vector<double> Q_Data(numRows);

    for (size_t i = 0; i < numRows; ++i) {
      I_Data[i] = static_cast<double>(data[2 * i]);
      Q_Data[i] = static_cast<double>(data[2 * i + 1]);
    }

    writeMatrix(varNameI, I_Data, numRows, 1);
    writeMatrix(varNameQ, Q_Data, numRows, 1);
  }

private:
  void openMatFile(const std::string &filename, OpenMode mode) {
    if (mode == OpenMode::ReadOnly) {
      matfp_ = std::unique_ptr<mat_t, decltype(&Mat_Close)>(
          Mat_Open(filename.c_str(), MAT_ACC_RDONLY), Mat_Close);
    } else {
      matfp_ = std::unique_ptr<mat_t, decltype(&Mat_Close)>(
          Mat_CreateVer(filename.c_str(), nullptr, MAT_FT_MAT5), Mat_Close);
    }

    if (!matfp_) {
      throw std::runtime_error("Failed to open MAT file: " + filename);
    }
  }

  template <typename T> matio_classes getMatClass() const {
    if constexpr (std::is_same_v<T, double>) {
      return MAT_C_DOUBLE;
    } else if constexpr (std::is_same_v<T, float>) {
      return MAT_C_SINGLE;
    } else {
      throw std::invalid_argument("Unsupported data type.");
    }
  }

  template <typename T> matio_types getMatType() const {
    if constexpr (std::is_same_v<T, double>) {
      return MAT_T_DOUBLE;
    } else if constexpr (std::is_same_v<T, float>) {
      return MAT_T_SINGLE;
    } else {
      throw std::invalid_argument("Unsupported data type.");
    }
  }

  std::unique_ptr<mat_t, decltype(&Mat_Close)> matfp_{nullptr, Mat_Close};
};

#endif // MATIO_CPP17_WRAPPER_H
