#ifndef __DATA_HANDLER_H__
#define __DATA_HANDLER_H__

#include <complex>
#include <iostream>
#include <map>
#include <variant>
#include <vector>

class RawDataUtils {
public:
  using SitePinRawData =
      std::map<uint32_t, std::map<std::string, std::vector<int16_t>>>;
  using SitePinSignalData =
      std::map<uint32_t,
               std::map<std::string, std::vector<std::complex<double>>>>;
  using SitePinFileData =
      std::map<uint32_t, std::map<std::string, std::string>>;
  using VariantData = std::variant<SitePinSignalData, SitePinFileData>;

  enum class RawDataType { SIGNAL = 0, FILE };
  enum class RawDataMode { LOCAL = 0, REMOTE };
};

class DataProcessor {
public:
  static RawDataUtils::SitePinSignalData
  ProcessSignalData(const RawDataUtils::SitePinRawData &rawData) {
    RawDataUtils::SitePinSignalData signalData;
    return signalData;
  }

  static RawDataUtils::SitePinFileData
  ProcessFileData(const RawDataUtils::SitePinRawData &rawData) {
    RawDataUtils::SitePinFileData fileData;
    return fileData;
  }
};

class DataHandler {
public:
  virtual ~DataHandler() = default;
  virtual void handleData(const RawDataUtils::SitePinRawData &rawData) = 0;
};

class IQSignalHandler : public DataHandler {
public:
  void handleData(const RawDataUtils::SitePinRawData &rawData) override {
    mSignalData = DataProcessor::ProcessSignalData(rawData);
  }
  const RawDataUtils::SitePinSignalData &GetSignalData() const {
    std::cout << "-- IQSignalHandler::GetSignalData --" << std::endl;
    return mSignalData;
  }

private:
  RawDataUtils::SitePinSignalData mSignalData;
};

class FileHandler : public DataHandler {
public:
  void handleData(const RawDataUtils::SitePinRawData &rawData) override {
    mFileData = DataProcessor::ProcessFileData(rawData);
  }
  RawDataUtils::SitePinFileData &GetFileData() {
    std::cout << "-- FileHandler::GetFileData --" << std::endl;
    return mFileData;
  }

private:
  RawDataUtils::SitePinFileData mFileData;
};

class RawDataMap {
public:
  void SetType(RawDataUtils::RawDataType type) { mType = type; }
  void SetMode(RawDataUtils::RawDataMode mode) { mMode = mode; }

  template <typename HandlerType> void SetHandler(const HandlerType &handler) {
    switch (mType) {
    case RawDataUtils::RawDataType::SIGNAL:
      if constexpr (std::is_same_v<HandlerType, IQSignalHandler>) {
        SetData(handler.GetSignalData());
      }
      break;

    case RawDataUtils::RawDataType::FILE:
      if constexpr (std::is_same_v<HandlerType, FileHandler>) {
        SetData(handler.GetFileData());
      }

    default:
      throw std::runtime_error("Invalid handler type.");
      break;
    }
  }
  template <typename VariantType> const VariantType &GetData() const {
    if (std::holds_alternative<VariantType>(mData)) {
      return std::get<VariantType>(mData);
    } else {
      throw std::runtime_error("Data type mismatch in GetData.");
    }
  }

private:
  template <typename...> struct always_false : std::false_type {};
  void SetData(const RawDataUtils::VariantData &data) { mData = data; }

  RawDataUtils::RawDataType mType{RawDataUtils::RawDataType::SIGNAL};
  RawDataUtils::RawDataMode mMode{RawDataUtils::RawDataMode::LOCAL};
  RawDataUtils::VariantData mData{RawDataUtils::SitePinSignalData{}};
};

#endif // __DATA_HANDLER_H__
