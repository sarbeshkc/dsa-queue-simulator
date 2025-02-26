#ifndef VEHICLE_H
#define VEHICLE_H

#include <string>
#include <SDL3/SDL.h>
#include <ctime>

class Vehicle {
public:
    Vehicle(const std::string& id, char lane, int laneNumber, bool isEmergency = false);
    ~Vehicle();

    // Getters and setters
    std::string getId() const;
    char getLane() const;
    void setLane(char lane);
    int getLaneNumber() const;
    void setLaneNumber(int number);
    bool isEmergencyVehicle() const;
    time_t getArrivalTime() const;

    // Animation related
    float getAnimationPos() const;
    void setAnimationPos(float pos);
    bool isTurning() const;
    void setTurning(bool turning);
    float getTurnProgress() const;
    void setTurnProgress(float progress);
    float getTurnPosX() const;
    void setTurnPosX(float x);
    float getTurnPosY() const;
    void setTurnPosY(float y);

    // Update vehicle position
    void update(uint32_t delta, bool isGreenLight, float targetPos);

    // Render vehicle
    void render(SDL_Renderer* renderer, SDL_Texture* vehicleTexture, int queuePos);

    // Calculate turn path
    void calculateTurnPath(float startX, float startY, float controlX, float controlY,
                          float endX, float endY, float progress);

private:
    std::string id;
    char lane;
    int laneNumber;
    bool isEmergency;
    time_t arrivalTime;

    // Animation properties
    float animPos;
    bool turning;
    float turnProgress;
    float turnPosX;
    float turnPosY;

    // Constants
    static constexpr float VEHICLE_LENGTH = 20.0f;
    static constexpr float VEHICLE_WIDTH = 10.0f;
    static constexpr float VEHICLE_GAP = 15.0f;
    static constexpr float TURN_SPEED = 0.0008f;
    static constexpr float MOVE_SPEED = 0.2f;

    // Helper methods
    float easeInOutQuad(float t) const;
};

#endif // VEHICLE_H
