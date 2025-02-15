// src/logging/analytics_manager.cpp
#include "analytics_manager.h"
#include <fstream>
#include <iomanip>

AnalyticsManager::AnalyticsManager(const DataLogger& logger)
    : m_logger(logger) {}

AnalyticsManager::LaneAnalysis AnalyticsManager::analyzeLane(LaneId lane) const {
    LaneAnalysis analysis;
    analysis.averageQueueLength = m_logger.getAverageQueueLength(lane);
    analysis.peakQueueLength = calculatePeakQueueLength(lane);
    // ... complete the analysis
    return analysis;
}

float AnalyticsManager::calculateSystemEfficiency() const {
    // Calculate overall system efficiency based on various metrics
    const auto& history = m_logger.getMetricsHistory();
    if (history.empty()) return 0.0f;

    float totalEfficiency = 0.0f;
    for (const auto& metrics : history) {
        // Consider factors like wait times, queue lengths, and throughput
        float waitTimeScore = 1.0f / (1.0f + metrics.averageWaitTime);
        float queueScore = 0.0f;
        for (const auto& [lane, length] : metrics.queueLengths) {
            queueScore += 1.0f / (1.0f + length);
        }
        queueScore /= metrics.queueLengths.size();

        totalEfficiency += (waitTimeScore + queueScore) / 2.0f;
    }

    return totalEfficiency / history.size();
}

void AnalyticsManager::generateReport(const std::string& outputPath) const {
    std::ofstream report(outputPath);
    if (!report.is_open()) return;

    report << "Traffic System Analysis Report\n";
    report << "=============================\n\n";

    // System-wide metrics
    report << "System Efficiency: "
           << std::fixed << std::setprecision(2)
           << calculateSystemEfficiency() * 100 << "%\n\n";

    // Per-lane analysis
    report << "Lane Analysis:\n";
    for (LaneId lane = LaneId::AL1_INCOMING;
         lane <= LaneId::DL3_FREELANE;
         lane = static_cast<LaneId>(static_cast<int>(lane) + 1)) {

        auto analysis = analyzeLane(lane);
        report << "Lane " << getLaneString(lane) << ":\n"
               << "  Average Queue Length: " << analysis.averageQueueLength << "\n"
               << "  Peak Queue Length: " << analysis.peakQueueLength << "\n"
               << "  Average Wait Time: " << analysis.averageWaitTime << "s\n"
               << "  Total Vehicles: " << analysis.totalVehiclesProcessed << "\n\n";
    }

    // Write utilization statistics
    auto utilization = calculateLaneUtilization();
    report << "Lane Utilization:\n";
    for (const auto& [lane, util] : utilization) {
        report << "  " << getLaneString(lane) << ": "
               << std::fixed << std::setprecision(1)
               << util * 100 << "%\n";
    }
}
