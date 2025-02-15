// src/logging/data_logger.cpp
#include "data_logger.h"
#include <filesystem>
#include <iomanip>
#include <sstream>

std::string TrafficMetrics::serialize() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);

    // Write timestamp and global metrics
    ss << timestamp << ","
       << totalVehicles << ","
       << (priorityMode ? "1" : "0") << ","
       << averageWaitTime << ","
       << maxWaitTime;

    // Write queue lengths for each lane
    for (const auto& [lane, length] : queueLengths) {
        ss << "," << length;
    }

    // Write wait times for each lane
    for (const auto& [lane, time] : waitTimes) {
        ss << "," << time;
    }

    return ss.str();
}

DataLogger::DataLogger(const std::string& logDir)
    : m_logDirectory(logDir) {
    createLogDirectory();
    startNewSession();
}

DataLogger::~DataLogger() {
    endSession();
}

void DataLogger::createLogDirectory() {
    namespace fs = std::filesystem;

    // Create main log directory if it doesn't exist
    if (!fs::exists(m_logDirectory)) {
        fs::create_directories(m_logDirectory);
    }
}

void DataLogger::startNewSession() {
    std::lock_guard<std::mutex> lock(m_logMutex);

    // Generate session ID based on timestamp
    std::string sessionId = generateSessionId();
    m_currentSessionPath = m_logDirectory + "/session_" + sessionId;

    namespace fs = std::filesystem;
    fs::create_directory(m_currentSessionPath);

    // Open log files
    m_metricsFile.open(m_currentSessionPath + "/metrics.csv");
    m_eventFile.open(m_currentSessionPath + "/events.csv");

    // Write headers
    writeMetricsHeader();
    writeEventHeader();
}

void DataLogger::endSession() {
    std::lock_guard<std::mutex> lock(m_logMutex);

    if (m_metricsFile.is_open()) {
        m_metricsFile.close();
    }
    if (m_eventFile.is_open()) {
        m_eventFile.close();
    }
}

void DataLogger::logTrafficMetrics(const FlowCoordinator& coordinator) {
    std::lock_guard<std::mutex> lock(m_logMutex);

    // Create metrics record
    TrafficMetrics metrics;
    metrics.timestamp = SDL_GetTicks() / 1000.0f;
    metrics.totalVehicles = coordinator.getTotalVehiclesProcessed();
    metrics.priorityMode = coordinator.isPriorityModeActive();
    metrics.averageWaitTime = coordinator.getAverageWaitTime();
    metrics.maxWaitTime = coordinator.getMaxWaitTime();

    // Get queue lengths and wait times for each lane
    for (LaneId lane = LaneId::AL1_INCOMING;
         lane <= LaneId::DL3_FREELANE;
         lane = static_cast<LaneId>(static_cast<int>(lane) + 1)) {
        metrics.queueLengths[lane] = coordinator.getLaneManager().getVehicleCount(lane);
        metrics.waitTimes[lane] = coordinator.getLaneManager().getAverageWaitTime(lane);
    }

    // Add to history
    m_metricsHistory.push_back(metrics);
    trimHistory();

    // Write to file
    if (m_metricsFile.is_open()) {
        m_metricsFile << metrics.serialize() << std::endl;
    }
}

void DataLogger::logEvent(const std::string& event, const std::string& details) {
    std::lock_guard<std::mutex> lock(m_logMutex);

    if (m_eventFile.is_open()) {
        float timestamp = SDL_GetTicks() / 1000.0f;
        m_eventFile << std::fixed << std::setprecision(2)
                   << timestamp << ","
                   << event << ","
                   << details << std::endl;
    }
}

std::string DataLogger::generateSessionId() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

void DataLogger::writeMetricsHeader() {
    if (!m_metricsFile.is_open()) return;

    m_metricsFile << "Timestamp,TotalVehicles,PriorityMode,AvgWaitTime,MaxWaitTime";

    // Add headers for each lane's queue length
    for (LaneId lane = LaneId::AL1_INCOMING;
         lane <= LaneId::DL3_FREELANE;
         lane = static_cast<LaneId>(static_cast<int>(lane) + 1)) {
        m_metricsFile << ",Queue_" << getLaneString(lane);
    }

    // Add headers for each lane's wait time
    for (LaneId lane = LaneId::AL1_INCOMING;
         lane <= LaneId::DL3_FREELANE;
         lane = static_cast<LaneId>(static_cast<int>(lane) + 1)) {
        m_metricsFile << ",Wait_" << getLaneString(lane);
    }

    m_metricsFile << std::endl;
}

void DataLogger::writeEventHeader() {
    if (!m_eventFile.is_open()) return;
    m_eventFile << "Timestamp,Event,Details" << std::endl;
}

void DataLogger::trimHistory() {
    if (m_metricsHistory.size() > MAX_HISTORY_SIZE) {
        m_metricsHistory.erase(
            m_metricsHistory.begin(),
            m_metricsHistory.begin() + (m_metricsHistory.size() - MAX_HISTORY_SIZE)
        );
    }
}

float DataLogger::getAverageQueueLength(LaneId lane) const {
    if (m_metricsHistory.empty()) return 0.0f;

    float total = 0.0f;
    for (const auto& metrics : m_metricsHistory) {
        auto it = metrics.queueLengths.find(lane);
        if (it != metrics.queueLengths.end()) {
            total += it->second;
        }
    }

    return total / m_metricsHistory.size();
}

float DataLogger::getAverageWaitTime() const {
    if (m_metricsHistory.empty()) return 0.0f;

    float total = 0.0f;
    for (const auto& metrics : m_metricsHistory) {
        total += metrics.averageWaitTime;
    }

    return total / m_metricsHistory.size();
}

