// src/logging/analytics_manager.h
#ifndef ANALYTICS_MANAGER_H
#define ANALYTICS_MANAGER_H

#include "data_logger.h"
#include <map>

class AnalyticsManager {
public:
    AnalyticsManager(const DataLogger& logger);

    // Analysis functions
    struct LaneAnalysis {
        float averageQueueLength;
        float peakQueueLength;
        float averageWaitTime;
        float maxWaitTime;
        int totalVehiclesProcessed;
    };

    LaneAnalysis analyzeLane(LaneId lane) const;
    float calculateSystemEfficiency() const;
    std::map<LaneId, float> calculateLaneUtilization() const;
    void generateReport(const std::string& outputPath) const;

private:
    const DataLogger& m_logger;

    // Helper methods
    float calculatePeakQueueLength(LaneId lane) const;
    float calculateAverageProcessingTime() const;
};

#endif // ANALYTICS_MANAGER_H
