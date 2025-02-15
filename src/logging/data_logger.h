// src/logging/data_logger.h
#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include "../traffic/flow_coordinator.h"

// Structure to hold traffic metrics at a point in time
struct TrafficMetrics {
    float timestamp;
    std::map<LaneId, int> queueLengths;
    std::map<LaneId, float> waitTimes;
    int totalVehicles;
    bool priorityMode;
    float averageWaitTime;
    float maxWaitTime;

    // Serialization helper
    std::string serialize() const;
};

class DataLogger {
public:
    DataLogger(const std::string& logDir);
    ~DataLogger();

    // Core logging functions
    void logTrafficMetrics(const FlowCoordinator& coordinator);
    void logEvent(const std::string& event, const std::string& details);
    void startNewSession();
    void endSession();

    // Data access
    std::vector<TrafficMetrics> getMetricsHistory() const;
    float getAverageQueueLength(LaneId lane) const;
    float getAverageWaitTime() const;

private:
    std::string m_logDirectory;
    std::string m_currentSessionPath;
    std::ofstream m_metricsFile;
    std::ofstream m_eventFile;
    std::vector<TrafficMetrics> m_metricsHistory;
    std::mutex m_logMutex;

    static constexpr int MAX_HISTORY_SIZE = 1000;  // Keep last 1000 metrics in memory

    // Helper methods
    void createLogDirectory();
    std::string generateSessionId() const;
    void writeMetricsHeader();
    void writeEventHeader();
    void trimHistory();
};
