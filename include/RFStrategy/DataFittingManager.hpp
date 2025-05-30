#ifndef DATA_FITTING_MANAGER_HPP
#define DATA_FITTING_MANAGER_HPP

#include <optional>
#include <vector>

#include "DynamicQueryPolicy.hpp"

/**
 * @brief 拟合所需的参数对象
 * - freq：必填频率
 * - power：可选功率
 */
struct FittingParams {
  double freq;
  std::optional<double> power;
  FittingParams(double f, std::optional<double> p = std::nullopt) : freq(f), power(p) {}
};

namespace FittingHelper {

using DataPoint = FittingParams;
using DataPoints = std::vector<DataPoint>;
using FittingRow = std::vector<std::string>;
using FittingRows = std::vector<FittingRow>;
using FittingPolicy = std::function<DataPoints(const DataQueryEngine &, const FittingParams &)>;
static DataPoints CollectNearbyDataPoints(DataQueryEngine &engine, const FittingParams &params,
                                          const FittingPolicy &policy) {
  return policy(engine, params);
}

static std::vector<std::string> BuildFittedRow(const FittingParams &params,
                                               const Row &referenceRow) {
  std::vector<std::string> rowData{std::to_string(params.freq)};
  size_t powerIndex = 0;
  if (params.power.has_value()) {
    rowData.push_back(std::to_string(params.power.value()));
    powerIndex = 1;
  }
  if (!referenceRow.empty()) {
    for (size_t i = powerIndex + 1; i < referenceRow.size(); ++i) {
      std::string val = std::string(referenceRow[i]);
      rowData.push_back(val);
    }
  }
  return rowData;
}
}; // namespace FittingHelper

class IFittingStrategy {
public:
  virtual ~IFittingStrategy() = default;

  /**
   * @param engine  用于读写数据 (插入行)
   * @param params  拟合参数 (freq 必填，power 可选)
   * @return        若插入成功，则返回“新行”的字符串数组；否则 std::nullopt
   */
  virtual std::optional<std::vector<std::string>> DoFittingRow(DataQueryEngine &engine,
                                                               const FittingParams &params) = 0;
};

struct FreqPowerNearbyDataPolicy {
  FittingHelper::DataPoints operator()(const DataQueryEngine &engine,
                                       const FittingParams &params) const {
    FittingHelper::DataPoints dataPoints;
    const auto &allRows = engine.GetOwnership()->GetModuleCFGData();
    for (const auto &row : allRows) {
      double freq = std::stod(std::string{row[0]});
      double value = std::stod(std::string{row[1]});
      if (std::abs(params.freq - freq) < 10e6 &&
          std::abs(params.power.value_or(0) - value) < 10e6) {
        dataPoints.emplace_back(freq, value);
      }
    }
    return dataPoints;
  }
};

// struct FreqPowerNearbyDataPolicy {
//   FittingHelper::FittingRows operator()(const DataQueryEngine &engine,
//                                         const FittingParams &params) const {
//     FittingHelper::FittingRows dataRows;
//     const auto &allRows = engine.GetOwnership()->GetModuleCFGData();
//     if (allRows.empty())
//       return dataRows;
//     auto fitFreq = params.freq / 1e6;
//     auto fitPower = params.power.value();
//     auto size = allRows.size();
//     auto num = size - 1;

//     auto freq = stod(std::string{allRows.front()[0]});
//     auto matchIndex = -0xFFFF;
//     auto freqIndex = 0;
//     auto powerIndex = 0;

//     auto makeMatchRows = [&allRows, &dataRows](auto left, auto right) {
//       if (left < 0 || right < 0)
//         return;
//       auto &leftRow = allRows[left];
//       auto &rightRow = allRows[right];
//       FittingHelper::FittingRow leftrow;
//       for (auto &data : leftRow) {
//         leftrow.emplace_back(std::string{data});
//       }
//       FittingHelper::FittingRow rightrow;
//       for (auto &data : rightRow) {
//         rightrow.emplace_back(std::string{data});
//       }
//       dataRows.emplace_back(leftrow);
//       dataRows.emplace_back(rightrow);
//     };

//     if (fitFreq > stod(std::string{allRows.back()[0]})) {
//       FittingHelper::FittingRow row;
//       auto &datasView = allRows.back();
//       freq = stod(std::string{datasView[0]});
//       powerIndex = size - 1;
//       matchIndex = powerIndex;
//       while (powerIndex > 0) {
//         const auto &row = allRows[powerIndex];
//         const auto &nextrow = allRows[powerIndex - 1];
//         if (freq != stod(std::string{row[0]}) || freq != stod(std::string{nextrow[0]}))
//           break;
//         if ((stod(std::string{row[1]}) - fitPower) * (stod(std::string{nextrow[1]}) - fitPower)
//         <=
//             0) {
//           matchIndex = powerIndex;
//           break;
//         }
//         --powerIndex;
//       }
//       makeMatchRows(matchIndex, matchIndex - 1);
//       return dataRows;
//     }

//     if (fitFreq < stod(std::string{allRows.front()[0]})) {
//       while (powerIndex < num) {
//         const auto &row = allRows[powerIndex];
//         const auto &nextrow = allRows[powerIndex + 1];
//         if (freq != stod(std::string{row[0]}) || freq != stod(std::string{nextrow[0]}))
//           break;
//         if ((stod(std::string{row[1]}) - fitPower) * (stod(std::string{nextrow[1]}) - fitPower)
//         <=
//             0) {
//           matchIndex = powerIndex;
//           break;
//         }
//         ++powerIndex;
//       }
//       makeMatchRows(matchIndex, powerIndex + 1);
//       return dataRows;
//     }

//     while (freqIndex < num) {
//       const auto &row = allRows[freqIndex];
//       if (freq == stod(std::string{row[0]})) {
//         ++freqIndex;
//         continue;
//       }
//       if ((stod(std::string{row[0]}) - fitFreq) * (freq - fitFreq) <= 0) {
//         break;
//       }
//       {
//         freq = stod(std::string{row[0]});
//         powerIndex = freqIndex;
//       }
//       ++freqIndex;
//     }

//     while (powerIndex < num) {
//       const auto &row = allRows[powerIndex];
//       const auto &nextrow = allRows[powerIndex + 1];
//       if ((stod(std::string{row[1]}) - fitPower) * (stod(std::string{nextrow[1]}) - fitPower) <=
//           0) {
//         matchIndex = powerIndex;
//         break;
//       }
//       ++powerIndex;
//     }
//     makeMatchRows(matchIndex, matchIndex + 1);
//     return dataRows;
//   }
// };

/**
 * @brief 自定义拟合策略
 * */
class FreqPowerFittingStrategy : public IFittingStrategy {
public:
  std::optional<std::vector<std::string>> DoFittingRow(DataQueryEngine &engine,
                                                       const FittingParams &params) override {
    /** 找到近邻数据点 */
    auto dataPoints =
        FittingHelper::CollectNearbyDataPoints(engine, params, FreqPowerNearbyDataPolicy());
    if (dataPoints.empty()) {
      // 如果附近也没有任何数据，则直接返回或抛异常
      throw std::runtime_error("No nearby data points found, cannot do fitting.");
    }

    auto [fittedFreq, fittedPower] = *std::min(dataPoints.begin(), dataPoints.end());
    // 构建“拟合策略查询”，看是否已存在一条匹配 fittedFreq/fittedPower 的记录
    FreqPowerQueryPolicy fittedPolicy(fittedFreq, fittedPower.value());
    QueryResult fittedResult = engine.ExecuteQuery(fittedPolicy);

    if (fittedResult.IsEmpty()) {
      throw std::runtime_error("No nearby data points found, cannot do fitting.");
    }
    // 如果找到了一行，把它转换后再插入，模拟“根据现有数据加一行新数据”
    const auto &refRow = fittedResult.GetMatchedRows()[0];
    return FittingHelper::BuildFittedRow(params, refRow);
  }
};

#endif