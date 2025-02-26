#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <SDL3/SDL.h>
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <cmath>
#include <queue>
#include <mutex>

// include the necessary headers for logging
#include "utils/DebugLogger.h"

namespace fs = std::filesystem;

// constants
const int window_width = 800;
const int window_height = 800;
const std::string data_dir = "data/lanes";
const int priority_threshold_high = 10;
const int priority_threshold_low = 5;
const float vehicle_length = 20.0f;
const float vehicle_width = 10.0f;
const float vehicle_gap = 15.0f;
const float turn_duration = 1500.0f;
const float bezier_control_offset = 80.0f;
const float turn_speed = 0.0008f;
const float move_speed = 0.2f;

// car colors for variety with more vibrant tones
const SDL_Color car_colors[] = {
    {230, 30, 30, 255},    // bright red
    {30, 30, 230, 255},    // bright blue
    {30, 180, 30, 255},    // bright green
    {230, 230, 30, 255},   // bright yellow
    {120, 120, 120, 255},  // silver
    {30, 180, 180, 255},   // teal
    {180, 30, 180, 255},   // purple
    {230, 120, 30, 255},   // orange
    {20, 20, 20, 255}      // dark
};
const int num_car_colors = 9;

// road colors
const SDL_Color road_color = {50, 50, 50, 255};           // darker road
const SDL_Color lane_marker_color = {255, 255, 255, 255}; // white lane markers
const SDL_Color yellow_marker_color = {255, 220, 30, 255}; // brighter yellow
const SDL_Color intersection_color = {40, 40, 40, 255};   // darker intersection
const SDL_Color sidewalk_color = {200, 200, 200, 255};    // lighter sidewalk
const SDL_Color grass_color = {100, 220, 100, 255};       // brighter grass

// forward declarations
class vehicle;

// simple logging function
void log_message(const std::string& msg) {
    std::cout << "[simulator] " << msg << std::endl;

    // also log to file
    std::ofstream log("simulator_debug.log", std::ios::app);
    if (log.is_open()) {
        log << "[simulator] " << msg << std::endl;
        log.close();
    }

    // also use DebugLogger if already initialized
    try {
        DebugLogger::log(msg);
    } catch (...) {
        // ignore errors if DebugLogger is not yet initialized
    }
}

// ensure data directories exist
bool ensure_directories() {
    try {
        if (!fs::exists(data_dir)) {
            fs::create_directories(data_dir);
            log_message("created directory: " + data_dir);
        }
        return true;
    } catch (const std::exception& e) {
        log_message("error creating directories: " + std::string(e.what()));
        return false;
    }
}

// create a random number generator
std::mt19937 rng(std::random_device{}());

// direction enum for vehicles
enum class direction {
    north, // up
    east,  // right
    south, // down
    west   // left
};

// enhanced vehicle class that matches the c code version
class vehicle {
public:
    std::string id;
    char currentlane; // current road (a, b, c, d)
    int currentlanenumber; // current lane number (1, 2, 3)
    char targetlane; // target road to exit on
    int targetlanenumber; // target lane number to exit on
    float x, y;
    float speed;
    bool isemergency;
    SDL_Color color;
    bool isturning;
    float turnprogress;
    float startx, starty, endx, endy;
    float controlx, controly;
    direction direction;
    float lanechangingprogress;
    bool ischanginglane;
    float lanechangestartx, lanechangestarty;
    float lanechangeendx, lanechangeendy;
    time_t arrivaltime;

    vehicle(const std::string& id, char lane, int lanenumber, char targetlane = '\0', int targetlanenumber = 0, bool isemergency = false)
        : id(id),
          currentlane(lane),
          currentlanenumber(lanenumber),
          targetlane(targetlane != '\0' ? targetlane : getrandomtargetlane(lane)),
          targetlanenumber(targetlanenumber > 0 ? targetlanenumber : getrandomtargetlanenumber()),
          speed(0.5f),
          isemergency(isemergency),
          isturning(false),
          turnprogress(0.0f),
          direction(getinitialdirection(lane)),
          lanechangingprogress(0.0f),
          ischanginglane(false),
          arrivaltime(std::time(nullptr)) {

        // assign a color based on emergency status or randomly
        if (isemergency) {
            color = {255, 0, 0, 255}; // red for emergency vehicles
        } else {
            std::uniform_int_distribution<int> dist(0, num_car_colors - 1);
            color = car_colors[dist(rng)];
        }

        // set initial position based on lane
        if (currentlane == 'a') {
            x = window_width/2 + (currentlanenumber-2)*50;
            y = 10; // start a bit inside the screen
        } else if (currentlane == 'b') {
            x = window_width/2 - (currentlanenumber-2)*50;
            y = window_height - 10;
        } else if (currentlane == 'c') {
            x = window_width - 10;
            y = window_height/2 + (currentlanenumber-2)*50;
        } else if (currentlane == 'd') {
            x = 10;
            y = window_height/2 - (currentlanenumber-2)*50;
        }

        startx = x;
        starty = y;

        // set turn path parameters for free lanes (lane 3)
        if (isfreelane()) {
            setupturnpath();
        }

        log_message("vehicle " + id + " created on lane " + currentlane + std::to_string(currentlanenumber) +
                    " with target " + targetlane + std::to_string(targetlanenumber));
    }

    // get initial direction based on entry lane
    direction getinitialdirection(char lane) {
        if (lane == 'a') return direction::south;
        if (lane == 'b') return direction::north;
        if (lane == 'c') return direction::west;
        return direction::east; // lane d
    }

    // get a random target lane (different from current)
    char getrandomtargetlane(char currentlane) {
        std::uniform_int_distribution<int> dist(0, 2);
        char lanes[] = {'a', 'b', 'c', 'd'};

        // find which of the 4 lanes is the current one
        int currentindex = 0;
        for (int i = 0; i < 4; i++) {
            if (lanes[i] == currentlane) {
                currentindex = i;
                break;
            }
        }

        // remove current lane and pick one of the remaining three
        char targetoptions[3];
        int index = 0;
        for (int i = 0; i < 4; i++) {
            if (i != currentindex) {
                targetoptions[index++] = lanes[i];
            }
        }

        return targetoptions[dist(rng)];
    }

    // get a random target lane number
    int getrandomtargetlanenumber() {
        std::uniform_int_distribution<int> dist(1, 3);
        return dist(rng);
    }

    // setup turn path for free lanes (lane 3)
    void setupturnpath() {
        // in a free lane, the vehicle should turn left at the intersection
        const int center_x = window_width / 2;
        const int center_y = window_height / 2;

        // set target position based on turning left
        if (currentlane == 'a') { // going south, turn east
            endx = window_width;
            endy = center_y - 50;
            controlx = center_x + 50;
            controly = center_y - 150;
        } else if (currentlane == 'b') { // going north, turn west
            endx = 0;
            endy = center_y + 50;
            controlx = center_x - 50;
            controly = center_y + 150;
        } else if (currentlane == 'c') { // going west, turn south
            endx = center_x + 50;
            endy = window_height;
            controlx = center_x + 150;
            controly = center_y + 50;
        } else if (currentlane == 'd') { // going east, turn north
            endx = center_x - 50;
            endy = 0;
            controlx = center_x - 150;
            controly = center_y - 50;
        }
    }

    // easing function for smoother animations
    float easeinoutquad(float t) const {
        return t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }

    // setup lane changing path
    void setuplanechangepath() {
        if (!ischanginglane || currentlanenumber == targetlanenumber) {
            return;
        }

        lanechangestartx = x;
        lanechangestarty = y;

        // calculate target position based on current direction
        if (direction == direction::north || direction == direction::south) {
            // moving vertically, lane change is horizontal
            int lanediff = targetlanenumber - currentlanenumber;
            lanechangeendx = x + lanediff * 50;
            lanechangeendy = y;
        } else {
            // moving horizontally, lane change is vertical
            int lanediff = targetlanenumber - currentlanenumber;
            lanechangeendx = x;
            lanechangeendy = y + lanediff * 50;
        }
    }

    // check if approaching the intersection
    bool approachingintersection() const {
        const int center_x = window_width / 2;
        const int center_y = window_height / 2;
        const int stop_distance = 90;

        if (currentlane == 'a' && y >= center_y - stop_distance) return true;
        if (currentlane == 'b' && y <= center_y + stop_distance) return true;
        if (currentlane == 'c' && x <= center_x + stop_distance) return true;
        if (currentlane == 'd' && x >= center_x - stop_distance) return true;

        return false;
    }

    // basic forward movement
    void moveforward(float scaledspeed) {
        if (direction == direction::south) y += scaledspeed;
        else if (direction == direction::north) y -= scaledspeed;
        else if (direction == direction::west) x -= scaledspeed;
        else if (direction == direction::east) x += scaledspeed;
    }

    // processes turning for lane 3 vehicles (left turn logic from c code)
    void updatefreelanevehicle(float scaledspeed) {
        const int center_x = window_width / 2;
        const int center_y = window_height / 2;

        // if vehicle is approaching the intersection, move normally
        if (!isturning) {
            // check if at turning point (same thresholds as in c code)
            bool atturningpoint = false;

            if (currentlane == 'a' && y >= center_y - 90) atturningpoint = true;
            else if (currentlane == 'b' && y <= center_y + 90) atturningpoint = true;
            else if (currentlane == 'c' && x <= center_x + 90) atturningpoint = true;
            else if (currentlane == 'd' && x >= center_x - 90) atturningpoint = true;

            if (atturningpoint) {
                // reached turning point, start turning
                isturning = true;
                turnprogress = 0.0f;
                log_message("vehicle " + id + " at lane " + currentlane + std::to_string(currentlanenumber) + " starting left turn");
            } else {
                // keep moving forward until reaching turning point
                moveforward(scaledspeed);
            }
            return;
        }

        // update turn progress - matching the c algorithm's turning speed
        turnprogress += scaledspeed * 0.01f;
        if (turnprogress > 1.0f) turnprogress = 1.0f;

        // calculate position using quadratic bezier curve (same as c code)
        float t = easeinoutquad(turnprogress);
        float oneminust = 1.0f - t;

        // compute current position based on bezier curve
        x = oneminust*oneminust*startx + 2*oneminust*t*controlx + t*t*endx;
        y = oneminust*oneminust*starty + 2*oneminust*t*controly + t*t*endy;

        // update direction based on where we are in the turn
        if (turnprogress > 0.5f) {
            // update direction based on target lane (after halfway through turn)
            if (currentlane == 'a') direction = direction::east;      // a turning to c
            else if (currentlane == 'b') direction = direction::west; // b turning to d
            else if (currentlane == 'c') direction = direction::south; // c turning to b
            else if (currentlane == 'd') direction = direction::north; // d turning to a
        }
    }

    // update lane changing progress
    void updatelanechanging(float scaledspeed) {
        lanechangingprogress += scaledspeed * 0.01f;

        if (lanechangingprogress >= 1.0f) {
            // lane change complete
            x = lanechangeendx;
            y = lanechangeendy;
            currentlanenumber = targetlanenumber;
            ischanginglane = false;
            return;
        }

        // smooth lane change using easing function (as in c code)
        float t = easeinoutquad(lanechangingprogress);
        x = lanechangestartx + (lanechangeendx - lanechangestartx) * t;
        y = lanechangestarty + (lanechangeendy - lanechangestarty) * t;

        // continue forward movement while changing lanes (same behavior as c code)
        if (direction == direction::south) y += scaledspeed * 0.5f;
        else if (direction == direction::north) y -= scaledspeed * 0.5f;
        else if (direction == direction::west) x -= scaledspeed * 0.5f;
        else if (direction == direction::east) x += scaledspeed * 0.5f;
    }

    // update method based on the c code logic but with cleaner structure
    void update(uint32_t delta, bool isgreenlight, float vehicleahead = -1.0f) {
        float scaledspeed = speed * (delta / 16.0f);

        // free lanes handle turning separately
        if (isfreelane()) {
            updatefreelanevehicle(scaledspeed);
            return;
        }

        // if we need to change lanes and haven't started yet
        if (currentlanenumber != targetlanenumber && !ischanginglane && !approachingintersection()) {
            ischanginglane = true;
            setuplanechangepath();
            lanechangingprogress = 0.0f;
        }

        // if currently changing lanes
        if (ischanginglane) {
            updatelanechanging(scaledspeed);
            return;
        }

        // regular movement logic based on traffic light and position
        if (isgreenlight || !approachingintersection()) {
            // check for a vehicle ahead if specified
            if (vehicleahead > 0) {
                float nextpos = 0;
                if (direction == direction::south) nextpos = y + scaledspeed;
                else if (direction == direction::north) nextpos = y - scaledspeed;
                else if (direction == direction::west) nextpos = x - scaledspeed;
                else if (direction == direction::east) nextpos = x + scaledspeed;

                // calculate distance based on direction
                float distance = 0;
                if (direction == direction::south || direction == direction::north) {
                    distance = std::abs(nextpos - vehicleahead);
                } else {
                    distance = std::abs(nextpos - vehicleahead);
                }

                // if too close to vehicle ahead, don't move
                if (distance < (vehicle_length + vehicle_gap)) {
                    return;
                }
            }

            moveforward(scaledspeed);
        }
    }

    // check if vehicle is off-screen
    bool isoffscreen() const {
        return x < -30 || x > window_width + 30 || y < -30 || y > window_height + 30;
    }

    // check if this is a free lane (lane 3 always has green light)
    bool isfreelane() const {
        return currentlanenumber == 3;
    }

    // check if turning is complete
    bool isturncomplete() const {
        return isfreelane() && isturning && turnprogress >= 1.0f;
    }

    // get vehicle position as a pair
    std::pair<float, float> getposition() const {
        return {x, y};
    }

    // draw the vehicle (car shape) - adapting the c drawing function
    void render(SDL_Renderer* renderer) const {
        // base car dimensions - like in c code
        const int car_length = 24;
        const int car_width = 12;

        // set color for vehicle - using the emergency or assigned color
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        // draw vehicle body based on direction or turning state
        SDL_FRect carbody;

        // determine orientation based on direction
        if (direction == direction::north || direction == direction::south) {
            // vertical orientation
            carbody = {x - car_width/2, y - car_length/2, (float)car_width, (float)car_length};
        } else {
            // horizontal orientation
            carbody = {x - car_length/2, y - car_width/2, (float)car_length, (float)car_width};
        }

        SDL_RenderFillRect(renderer, &carbody);

        // draw car roof (slightly darker)
        SDL_SetRenderDrawColor(renderer,
            (Uint8)(color.r * 0.7f),
            (Uint8)(color.g * 0.7f),
            (Uint8)(color.b * 0.7f),
            color.a);

        SDL_FRect carroof;
        if (direction == direction::north || direction == direction::south) {
            // roof for vertical cars - adjust for direction
            float roofoffset = (direction == direction::south) ? 5 : 2;
            carroof = {x - car_width/2 + 2, y - car_length/2 + roofoffset,
                     (float)(car_width - 4), (float)(car_length/2)};
        } else {
            // roof for horizontal cars - adjust for direction
            float roofoffset = (direction == direction::west) ? 2 : 5;
            carroof = {x - car_length/2 + roofoffset, y - car_width/2 + 2,
                     (float)(car_length/2), (float)(car_width - 4)};
        }

        SDL_RenderFillRect(renderer, &carroof);

        // add headlights (small yellow rectangles) - enhancement over c version
        SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255); // yellow headlights

        float headlightsize = 3.0f;
        SDL_FRect leftlight, rightlight;

        if (direction == direction::north || direction == direction::south) {
            // vertical orientation - headlights at front based on direction
            float heady = (direction == direction::south) ?
                         y + car_length/2 - headlightsize/2 - 2 : // south - lights at bottom
                         y - car_length/2 + headlightsize/2 + 2;  // north - lights at top

            leftlight = {x - car_width/3 - headlightsize/2, heady - headlightsize/2, headlightsize, headlightsize};
            rightlight = {x + car_width/3 - headlightsize/2, heady - headlightsize/2, headlightsize, headlightsize};
        } else {
            // horizontal orientation - headlights at front based on direction
            float headx = (direction == direction::west) ?
                         x - car_length/2 + headlightsize/2 + 2 : // west - lights at left
                         x + car_length/2 - headlightsize/2 - 2;  // east - lights at right

            leftlight = {headx - headlightsize/2, y - car_width/3 - headlightsize/2, headlightsize, headlightsize};
            rightlight = {headx - headlightsize/2, y + car_width/3 - headlightsize/2, headlightsize, headlightsize};
        }

        SDL_RenderFillRect(renderer, &leftlight);
        SDL_RenderFillRect(renderer, &rightlight);
    }

    // getter methods
    std::string getid() const { return id; }
    char getlane() const { return currentlane; }
    int getlanenumber() const { return currentlanenumber; }
    bool getisemergency() const { return isemergency; }
    time_t getarrivaltime() const { return arrivaltime; }
};

// queue implementation for vehicle management (adapting from c code)
class vehiclequeue {
private:
    std::vector<vehicle*> vehicles;
    mutable std::mutex queuemutex;
    int maxsize;
    char laneid;
    int lanenumber;
    bool ispriority;
    int priority;

public:
    vehiclequeue(char laneid, int lanenumber, int maxsize = 100)
        : maxsize(maxsize), laneid(laneid), lanenumber(lanenumber),
          ispriority(laneid == 'a' && lanenumber == 2), priority(0) {
        log_message("created queue for lane " + std::string(1, laneid) + std::to_string(lanenumber));
    }

    ~vehiclequeue() {
        std::lock_guard<std::mutex> lock(queuemutex);
        for (auto* vehicle : vehicles) {
            delete vehicle;
        }
        vehicles.clear();
    }

    // add vehicle to queue
    void enqueue(vehicle* vehicle) {
        std::lock_guard<std::mutex> lock(queuemutex);
        if (vehicles.size() < maxsize) {
            vehicles.push_back(vehicle);
            log_message("enqueued vehicle " + vehicle->getid() + " to lane " +
                       std::string(1, laneid) + std::to_string(lanenumber) +
                       " (size: " + std::to_string(vehicles.size()) + ")");
        } else {
            log_message("queue for lane " + std::string(1, laneid) + std::to_string(lanenumber) + " is full!");
            delete vehicle;
        }
    }

    // remove and return front vehicle
    vehicle* dequeue() {
        std::lock_guard<std::mutex> lock(queuemutex);
        if (!vehicles.empty()) {
            vehicle* vehicle = vehicles.front();
            vehicles.erase(vehicles.begin());
            log_message("dequeued vehicle " + vehicle->getid() + " from lane " +
                       std::string(1, laneid) + std::to_string(lanenumber) +
                       " (size: " + std::to_string(vehicles.size()) + ")");
            return vehicle;
        }
        return nullptr;
    }

    // check if queue is empty
    bool isempty() const {
        std::lock_guard<std::mutex> lock(queuemutex);
        return vehicles.empty();
    }

    // check if queue is full
    bool isfull() const {
        std::lock_guard<std::mutex> lock(queuemutex);
        return vehicles.size() >= maxsize;
    }

    // get queue size
    int size() const {
        std::lock_guard<std::mutex> lock(queuemutex);
        return vehicles.size();
    }

    // get priority value
    int getpriority() const {
        return priority;
    }

    // update priority based on vehicle count (same threshold logic as c version)
    void updatepriority() {
        if (ispriority) {
            int count = size();

            if (count > priority_threshold_high && priority == 0) {
                priority = 100;
                log_message("lane " + std::string(1, laneid) + std::to_string(lanenumber) +
                           " priority increased (vehicles: " + std::to_string(count) + ")");
            }
            else if (count < priority_threshold_low && priority > 0) {
                priority = 0;
                log_message("lane " + std::string(1, laneid) + std::to_string(lanenumber) +
                           " priority reset to normal (vehicles: " + std::to_string(count) + ")");
            }
        }
    }

    // update all vehicles in the queue
    void updatevehicles(uint32_t delta, bool isgreenlight) {
        std::lock_guard<std::mutex> lock(queuemutex);

        // go through each vehicle in the queue
        for (size_t i = 0; i < vehicles.size(); i++) {
            // get vehicle ahead position if there is one
            float vehicleahead = -1.0f;
            if (i > 0) {
                std::pair<float, float> aheadpos = vehicles[i-1]->getposition();
                vehicleahead = (laneid == 'a' || laneid == 'b') ? aheadpos.second : aheadpos.first;
            }

            // update vehicle position
            vehicles[i]->update(delta, isgreenlight, vehicleahead);
        }

        // remove vehicles that have left the screen
        vehicles.erase(
            std::remove_if(vehicles.begin(), vehicles.end(),
                [this](vehicle* v) {
                    if (v->isoffscreen() || v->isturncomplete()) {
                        log_message("vehicle " + v->getid() + " left lane " +
                                   std::string(1, laneid) + std::to_string(lanenumber));
                        delete v;
                        return true;
                    }
                    return false;
                }),
            vehicles.end()
        );
    }

    // get all vehicles for rendering
    std::vector<vehicle*> getvehicles() const {
        std::lock_guard<std::mutex> lock(queuemutex);
        return vehicles;
    }

    // check if this is a priority lane
    bool isprioritylane() const {
        return ispriority;
    }

    // get lane id
    char getlaneid() const {
        return laneid;
    }

    // get lane number
    int getlanenumber() const {
        return lanenumber;
    }
};

// trafficlight states (matching c code)
enum class trafficlightstate {
    all_red = 0,
    a_green = 1,
    b_green = 2,
    c_green = 3,
    d_green = 4
};

// trafficlight class (adapted from c code)
class trafficlight {
private:
    trafficlightstate currentstate;
    trafficlightstate nextstate;
    uint32_t lastchangetime;
    bool prioritymode;

public:
    trafficlight()
        : currentstate(trafficlightstate::all_red),
          nextstate(trafficlightstate::a_green),
          lastchangetime(SDL_GetTicks()),
          prioritymode(false) {
    }

    // update traffic light state (using c code logic)
    void update(const std::vector<vehiclequeue*>& queues) {
        uint32_t currenttime = SDL_GetTicks();
        uint32_t elapsedtime = currenttime - lastchangetime;

        // find priority lane (a2)
        vehiclequeue* prioritylane = nullptr;
        for (auto* queue : queues) {
            if (queue->getlaneid() == 'a' && queue->getlanenumber() == 2) {
                prioritylane = queue;
                break;
            }
        }

        // update priority mode if needed
        if (prioritylane) {
            int vehiclecount = prioritylane->size();
            int priorityvalue = prioritylane->getpriority();

            // check for priority mode activation/deactivation
            if (!prioritymode && priorityvalue > 0) {
                prioritymode = true;
                log_message("priority mode activated: a2 has " + std::to_string(vehiclecount) + " vehicles");

                // if not already serving a, switch to it after all_red
                if (currentstate != trafficlightstate::a_green) {
                    nextstate = trafficlightstate::all_red;
                }
            }
            else if (prioritymode && priorityvalue == 0) {
                prioritymode = false;
                log_message("priority mode deactivated: a2 now has " + std::to_string(vehiclecount) + " vehicles");
            }
        }

        // calculate appropriate duration (same formula as c code)
        int stateduration;
        if (currentstate == trafficlightstate::all_red) {
            stateduration = 2000; // 2 seconds for all red
        } else {
            // identify active lane
            char activelane = ' ';
            int totalvehicles = 0;
            int lanecount = 0;

            switch (currentstate) {
                case trafficlightstate::a_green: activelane = 'a'; break;
                case trafficlightstate::b_green: activelane = 'b'; break;
                case trafficlightstate::c_green: activelane = 'c'; break;
                case trafficlightstate::d_green: activelane = 'd'; break;
                default: break;
            }

            // count vehicles in normal lanes (not lane 3)
            for (auto* queue : queues) {
                if (queue->getlaneid() == activelane && queue->getlanenumber() != 3) {
                    totalvehicles += queue->size();
                    lanecount++;
                }
            }

            // calculate average vehicles (same formula as in assignment)
            float avgvehicles = lanecount > 0 ? static_cast<float>(totalvehicles) / lanecount : 0;

            // total time = |v| * t where t = 2 seconds (same formula as in assignment)
            stateduration = static_cast<int>(avgvehicles * 2000);

            // enforce minimum and maximum durations
            stateduration = std::max(stateduration, 3000);  // min 3 seconds
            stateduration = std::min(stateduration, 15000); // max 15 seconds
        }

        // state transition logic (matching c code)
        if (elapsedtime >= stateduration) {
            // change to next state
            currentstate = nextstate;

            // determine next state
            if (prioritymode) {
                // in priority mode, alternate between a_green and all_red
                if (currentstate == trafficlightstate::all_red) {
                    nextstate = trafficlightstate::a_green;
                } else {
                    nextstate = trafficlightstate::all_red;
                }
            } else {
                // in normal mode, rotate through all lanes
                if (currentstate == trafficlightstate::all_red) {
                    // choose the next green light in sequence
                    switch (nextstate) {
                        case trafficlightstate::a_green: nextstate = trafficlightstate::b_green; break;
                        case trafficlightstate::b_green: nextstate = trafficlightstate::c_green; break;
                        case trafficlightstate::c_green: nextstate = trafficlightstate::d_green; break;
                        case trafficlightstate::d_green: nextstate = trafficlightstate::a_green; break;
                        default: nextstate = trafficlightstate::a_green; break;
                    }
                } else {
                    // any green state goes to all_red next
                    nextstate = trafficlightstate::all_red;
                }
            }

            // log state change
            log_message("traffic light changed state. duration: " + std::to_string(stateduration) + "ms");
            lastchangetime = currenttime;
        }
    }

    // check if a specific lane has a green light
    bool isgreen(char laneid) const {
        switch (laneid) {
            case 'a': return currentstate == trafficlightstate::a_green;
            case 'b': return currentstate == trafficlightstate::b_green;
            case 'c': return currentstate == trafficlightstate::c_green;
            case 'd': return currentstate == trafficlightstate::d_green;
            default: return false;
        }
    }

    // draw traffic lights (adapted from c code)
    void render(SDL_Renderer* renderer) const {
        // draw traffic lights for each road
        drawlightfora(renderer, !isgreen('a'));
        drawlightforb(renderer, !isgreen('b'));
        drawlightforc(renderer, !isgreen('c'));
        drawlightford(renderer, !isgreen('d'));

        // show priority mode indicator if active
        if (prioritymode) {
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // orange
            SDL_FRect priorityindicator = {10, 10, 30, 30};
            SDL_RenderFillRect(renderer, &priorityindicator);

            // black border
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &priorityindicator);
        }
    }

    // draw traffic light for road a (matching placement in c code)
    void drawlightfora(SDL_Renderer* renderer, bool isred) const {
        // same dimensions and positions as in c code but with FRect
        SDL_FRect lightbox = {388.0f, 288.0f, 70.0f, 30.0f};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &lightbox);

        // background
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_FRect innerbox = {389.0f, 289.0f, 68.0f, 28.0f};
        SDL_RenderFillRect(renderer, &innerbox);

        // right turn light - always green for lane 3
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green
        SDL_FRect right_light = {433.0f, 293.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &right_light);

        // main light - controlled by traffic signal
        if(isred)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
        else
            SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green

        SDL_FRect straight_light = {393.0f, 293.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &straight_light);
    }

    // draw traffic light for road b (matching placement in c code)
    void drawlightforb(SDL_Renderer* renderer, bool isred) const {
        // same dimensions and positions as in c code but with FRect
        SDL_FRect lightbox = {325.0f, 488.0f, 80.0f, 30.0f};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &lightbox);

        // background
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_FRect innerbox = {326.0f, 489.0f, 78.0f, 28.0f};
        SDL_RenderFillRect(renderer, &innerbox);

        // left lane light -> always green for lane 3
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green
        SDL_FRect left_light = {330.0f, 493.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &left_light);

        // middle lane light -> controlled by traffic signal
        if(isred)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
        else
            SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green

        SDL_FRect middle_light = {380.0f, 493.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &middle_light);
    }

    // draw traffic light for road c (matching placement in c code)
    void drawlightforc(SDL_Renderer* renderer, bool isred) const {
        // same dimensions and positions as in c code but with FRect
        SDL_FRect lightbox = {488.0f, 388.0f, 30.0f, 70.0f};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &lightbox);

        // background
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_FRect innerbox = {489.0f, 389.0f, 28.0f, 68.0f};
        SDL_RenderFillRect(renderer, &innerbox);

        // right turn light - always green for lane 3
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green
        SDL_FRect right_light = {493.0f, 433.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &right_light);

        // straight light - controlled by traffic signal
        if(isred)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
        else
            SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green

        SDL_FRect straight_light = {493.0f, 393.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &straight_light);
    }

    // draw traffic light for road d (matching placement in c code)
    void drawlightford(SDL_Renderer* renderer, bool isred) const {
        // same dimensions and positions as in c code but with FRect
        SDL_FRect lightbox = {288.0f, 325.0f, 30.0f, 90.0f};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &lightbox);

        // background
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_FRect innerbox = {289.0f, 326.0f, 28.0f, 88.0f};
        SDL_RenderFillRect(renderer, &innerbox);

        // left turn light - always green for lane 3
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green
        SDL_FRect left_turn_light = {293.0f, 330.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &left_turn_light);

        // middle lane light - controlled by traffic signal
        if(isred)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
        else
            SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255); // green

        SDL_FRect middle_light = {293.0f, 380.0f, 20.0f, 20.0f};
        SDL_RenderFillRect(renderer, &middle_light);
    }

    // get current state
    trafficlightstate getcurrentstate() const {
        return currentstate;
    }

    // get next state
    trafficlightstate getnextstate() const {
        return nextstate;
    }

    // is in priority mode?
    bool isprioritymode() const {
        return prioritymode;
    }
};

// file handler for reading vehicle data from files
class filehandler {
private:
    std::string datapath;
    std::mutex filemutex;

public:
    filehandler(const std::string& path = data_dir) : datapath(path) {
        ensure_directories();
    }

    // initialize or reset all files
    bool initializefiles() {
        std::lock_guard<std::mutex> lock(filemutex);

        try {
            // create data directory if it doesn't exist
            if (!fs::exists(datapath)) {
                fs::create_directories(datapath);
            }

            // create or clear lane files
            for (char laneid : {'a', 'b', 'c', 'd'}) {
                std::string filepath = datapath + "/lane" + laneid + ".txt";
                std::ofstream file(filepath, std::ios::trunc);
                if (!file.is_open()) {
                    log_message("failed to create/clear file: " + filepath);
                    return false;
                }
                file.close();
            }

            // create/clear status file
            std::string statuspath = datapath + "/lane_status.txt";
            std::ofstream statusfile(statuspath, std::ios::trunc);
            if (!statusfile.is_open()) {
                log_message("failed to create status file: " + statuspath);
                return false;
            }
            statusfile << "=== lane status log ===" << std::endl;
            statusfile.close();

            log_message("files initialized successfully");
            return true;
        } catch (const std::exception& e) {
            log_message("error initializing files: " + std::string(e.what()));
            return false;
        }
    }

    // read vehicles from files (similar to c version)
    std::vector<vehicle*> readvehiclesfromfiles() {
        std::lock_guard<std::mutex> lock(filemutex);
        std::vector<vehicle*> vehicles;

        for (char laneid : {'a', 'b', 'c', 'd'}) {
            std::string filepath = datapath + "/lane" + laneid + ".txt";
            std::ifstream file(filepath);

            if (!file.is_open()) {
                continue;
            }

            std::vector<std::string> lines;
            std::string line;

            // read all lines from file
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            file.close();

            // clear the file after reading
            std::ofstream clearfile(filepath, std::ios::trunc);
            clearfile.close();

            // process each line
            for (const auto& line : lines) {
                size_t pos = line.find(":");
                if (pos == std::string::npos) continue;

                std::string vehicleid = line.substr(0, pos);
                char lane = line[pos + 1];

                // extract lane number from id (same as c code)
                int lanenumber = 2; // default
                if (vehicleid.find("L1") != std::string::npos) {
                    lanenumber = 1;
                } else if (vehicleid.find("L3") != std::string::npos) {
                    lanenumber = 3;
                }

                // check if emergency vehicle
                bool isemergency = (vehicleid.find("EMG") != std::string::npos);

                // create vehicle with parameters matching c code
                vehicles.push_back(new vehicle(vehicleid, lane, lanenumber, '\0', 0, isemergency));
            }
        }

        // log how many vehicles were read
        if (!vehicles.empty()) {
            log_message("read " + std::to_string(vehicles.size()) + " vehicles from files");
        }

        return vehicles;
    }

    // write lane status to file
    void writelanestatus(char laneid, int lanenumber, int vehiclecount, bool ispriority) {
        std::lock_guard<std::mutex> lock(filemutex);
        std::string statuspath = datapath + "/lane_status.txt";

        std::ofstream file(statuspath, std::ios::app);
        if (file.is_open()) {
            file << laneid << lanenumber << ": " << vehiclecount << " vehicles"
                 << (ispriority ? " (priority)" : "") << std::endl;
            file.close();
        }
    }
};

// traffic manager for lane queues and simulation logic
class trafficmanager {
private:
    std::vector<vehiclequeue*> queues;
    trafficlight* trafficlight;
    filehandler* filehandler;
    uint32_t lastfilechecktime;
    uint32_t lastpriorityupdatetime;
    bool running;

public:
    trafficmanager()
        : trafficlight(nullptr),
          filehandler(nullptr),
          lastfilechecktime(0),
          lastpriorityupdatetime(0),
          running(false) {
    }

    ~trafficmanager() {
        // clean up resources
        for (auto* queue : queues) {
            delete queue;
        }
        queues.clear();

        if (trafficlight) {
            delete trafficlight;
            trafficlight = nullptr;
        }

        if (filehandler) {
            delete filehandler;
            filehandler = nullptr;
        }
    }

    // initialize the traffic manager
    bool initialize() {
        // create file handler
        filehandler = new filehandler(data_dir);
        if (!filehandler->initializefiles()) {
            log_message("failed to initialize files");
            return false;
        }

        // create queues for each lane (12 total - 4 roads x 3 lanes each)
        for (char road : {'a', 'b', 'c', 'd'}) {
            for (int lanenum = 1; lanenum <= 3; lanenum++) {
                queues.push_back(new vehiclequeue(road, lanenum));
            }
        }

        // create traffic light controller
        trafficlight = new trafficlight();

        log_message("trafficmanager initialized with " + std::to_string(queues.size()) + " lanes");
        return true;
    }

    // start the traffic manager
    void start() {
        running = true;
        log_message("trafficmanager started");
    }

    // stop the traffic manager
    void stop() {
        running = false;
        log_message("trafficmanager stopped");
    }

    // update simulation state
    void update(uint32_t delta) {
        if (!running) return;

        uint32_t currenttime = SDL_GetTicks();

        // check for new vehicles periodically
        if (currenttime - lastfilechecktime >= 1000) { // every second
            checkfiles();
            lastfilechecktime = currenttime;
        }

        // update priorities periodically
        if (currenttime - lastpriorityupdatetime >= 500) { // every half second
            updatepriorities();
            lastpriorityupdatetime = currenttime;
        }

        // update traffic light state
        if (trafficlight) {
            trafficlight->update(queues);
        }

        // update vehicles in each queue
        for (auto* queue : queues) {
            bool isgreenlight = false;

            // free lanes (lane 3) always have green light
            if (queue->getlanenumber() == 3) {
                isgreenlight = true;
            } else if (trafficlight) {
                // check traffic light state for other lanes
                isgreenlight = trafficlight->isgreen(queue->getlaneid());
            }

            // update all vehicles in this queue
            queue->updatevehicles(delta, isgreenlight);
        }
    }

    // check files for new vehicles
    void checkfiles() {
        if (!filehandler) return;

        // read vehicles from files
        std::vector<vehicle*> newvehicles = filehandler->readvehiclesfromfiles();

        // add vehicles to appropriate queues
        for (auto* vehicle : newvehicles) {
            vehiclequeue* targetqueue = findqueue(vehicle->getlane(), vehicle->getlanenumber());
            if (targetqueue) {
                targetqueue->enqueue(vehicle);
            } else {
                log_message("failed to find queue for vehicle " + vehicle->getid());
                delete vehicle;
            }
        }
    }

    // update queue priorities
    void updatepriorities() {
        for (auto* queue : queues) {
            queue->updatepriority();

            // log priority lane status
            if (queue->isprioritylane() && filehandler) {
                filehandler->writelanestatus(
                    queue->getlaneid(),
                    queue->getlanenumber(),
                    queue->size(),
                    queue->getpriority() > 0
                );
            }
        }
    }

    // find a queue by lane id and number
    vehiclequeue* findqueue(char laneid, int lanenumber) const {
        for (auto* queue : queues) {
            if (queue->getlaneid() == laneid && queue->getlanenumber() == lanenumber) {
                return queue;
            }
        }
        return nullptr;
    }

    // get all queues
    const std::vector<vehiclequeue*>& getqueues() const {
        return queues;
    }

    // get traffic light
    trafficlight* gettrafficlight() const {
        return trafficlight;
    }

    // get statistics for display
    std::string getstatistics() const {
        std::string stats = "lane statistics:\n";
        int totalvehicles = 0;

        for (auto* queue : queues) {
            int count = queue->size();
            totalvehicles += count;

            stats += queue->getlaneid() + std::to_string(queue->getlanenumber()) +
                    ": " + std::to_string(count) + " vehicles";

            if (queue->isprioritylane() && queue->getpriority() > 0) {
                stats += " (priority)";
            }

            stats += "\n";
        }

        stats += "total vehicles: " + std::to_string(totalvehicles) + "\n";
        return stats;
    }
};

// renderer for drawing the simulation
class renderer {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    int windowwidth;
    int windowheight;
    bool active;
    bool showdebug;
    trafficmanager* trafficmanager;

public:
    renderer()
        : window(nullptr),
          renderer(nullptr),
          windowwidth(800),
          windowheight(800),
          active(false),
          showdebug(false),  // debug off by default
          trafficmanager(nullptr) {}

    ~renderer() {
        cleanup();
    }

    // initialize the renderer
    bool initialize(int width, int height, const std::string& title) {
        windowwidth = width;
        windowheight = height;

        // create window
        window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
        if (!window) {
            log_message("failed to create window: " + std::string(SDL_GetError()));
            return false;
        }

        // create renderer
        renderer = SDL_CreateRenderer(window, NULL);
        if (!renderer) {
            log_message("failed to create renderer: " + std::string(SDL_GetError()));
            return false;
        }

        active = true;
        log_message("renderer initialized successfully");
        return true;
    }

    // set the traffic manager
    void settrafficmanager(trafficmanager* manager) {
        trafficmanager = manager;
    }

    // draw road layout (matching c code road design)
    void drawroadlayout() {
        const int road_width = 150;
        const int lane_width = 50;
        const int sidewalk_width = 20;

        // draw grass background
        SDL_SetRenderDrawColor(renderer, grass_color.r, grass_color.g, grass_color.b, grass_color.a);
        SDL_RenderClear(renderer);

        // draw sidewalks
        SDL_SetRenderDrawColor(renderer, sidewalk_color.r, sidewalk_color.g, sidewalk_color.b, sidewalk_color.a);

        // horizontal sidewalks
        SDL_FRect hsidewalk1 = {0, (float)(windowheight/2 - road_width/2 - sidewalk_width),
                               (float)windowwidth, (float)sidewalk_width};
        SDL_FRect hsidewalk2 = {0, (float)(windowheight/2 + road_width/2),
                               (float)windowwidth, (float)sidewalk_width};
        SDL_RenderFillRect(renderer, &hsidewalk1);
        SDL_RenderFillRect(renderer, &hsidewalk2);

        // vertical sidewalks
        SDL_FRect vsidewalk1 = {(float)(windowwidth/2 - road_width/2 - sidewalk_width), 0,
                               (float)sidewalk_width, (float)windowheight};
        SDL_FRect vsidewalk2 = {(float)(windowwidth/2 + road_width/2), 0,
                               (float)sidewalk_width, (float)windowheight};
        SDL_RenderFillRect(renderer, &vsidewalk1);
        SDL_RenderFillRect(renderer, &vsidewalk2);

        // draw main roads (dark gray)
        SDL_SetRenderDrawColor(renderer, road_color.r, road_color.g, road_color.b, road_color.a);

        // horizontal road
        SDL_FRect hroad = {0, (float)(windowheight/2 - road_width/2),
                          (float)windowwidth, (float)road_width};
        SDL_RenderFillRect(renderer, &hroad);

        // vertical road
        SDL_FRect vroad = {(float)(windowwidth/2 - road_width/2), 0,
                          (float)road_width, (float)windowheight};
        SDL_RenderFillRect(renderer, &vroad);

        // draw intersection (slightly darker)
        SDL_SetRenderDrawColor(renderer, intersection_color.r, intersection_color.g, intersection_color.b, intersection_color.a);
        SDL_FRect intersection = {(float)(windowwidth/2 - road_width/2), (float)(windowheight/2 - road_width/2),
                                 (float)road_width, (float)road_width};
        SDL_RenderFillRect(renderer, &intersection);

        // draw lane dividers - same style as c code
        // horizontal lane dividers
        for (int i = 1; i < 3; i++) {
            int y = windowheight/2 - road_width/2 + i * lane_width;

            if (i == 1) {
                // center line (yellow)
                SDL_SetRenderDrawColor(renderer, yellow_marker_color.r, yellow_marker_color.g,
                                     yellow_marker_color.b, yellow_marker_color.a);
            } else {
                // other lane dividers (white)
                SDL_SetRenderDrawColor(renderer, lane_marker_color.r, lane_marker_color.g,
                                     lane_marker_color.b, lane_marker_color.a);
            }

            // draw dashed lines outside intersection
            for (int x = 0; x < windowwidth; x += 30) {
                if (x < windowwidth/2 - road_width/2 || x > windowwidth/2 + road_width/2) {
                    SDL_FRect line = {(float)x, (float)y - 2, 15, 4};
                    SDL_RenderFillRect(renderer, &line);
                }
            }
        }

        // vertical lane dividers
        for (int i = 1; i < 3; i++) {
            int x = windowwidth/2 - road_width/2 + i * lane_width;

            if (i == 1) {
                // center line (yellow)
                SDL_SetRenderDrawColor(renderer, yellow_marker_color.r, yellow_marker_color.g,
                                     yellow_marker_color.b, yellow_marker_color.a);
            } else {
                // other lane dividers (white)
                SDL_SetRenderDrawColor(renderer, lane_marker_color.r, lane_marker_color.g,
                                     lane_marker_color.b, lane_marker_color.a);
            }

            // draw dashed lines outside intersection
            for (int y = 0; y < windowheight; y += 30) {
                if (y < windowheight/2 - road_width/2 || y > windowheight/2 + road_width/2) {
                    SDL_FRect line = {(float)x - 2, (float)y, 4, 15};
                    SDL_RenderFillRect(renderer, &line);
                }
            }
        }

        // draw crosswalks (matching c code)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // north crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowwidth/2 - road_width/2 + 15*i),
                               (float)(windowheight/2 - road_width/2 - 15), 10, 15};
            SDL_RenderFillRect(renderer, &stripe);
        }

        // south crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowwidth/2 - road_width/2 + 15*i),
                               (float)(windowheight/2 + road_width/2), 10, 15};
            SDL_RenderFillRect(renderer, &stripe);
        }

        // east crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowwidth/2 + road_width/2),
                               (float)(windowheight/2 - road_width/2 + 15*i), 15, 10};
            SDL_RenderFillRect(renderer, &stripe);
        }

        // west crosswalk
        for (int i = 0; i < 10; i++) {
            SDL_FRect stripe = {(float)(windowwidth/2 - road_width/2 - 15),
                               (float)(windowheight/2 - road_width/2 + 15*i), 15, 10};
            SDL_RenderFillRect(renderer, &stripe);
        }

        // draw turn arrows for free lanes
        drawturnarrows();

        // draw lane labels
        drawlanelabels();
    }

    // draw lane labels for better visibility
    void drawlanelabels() {
        // road a labels
        drawtext("a1", window_width/2 - 40, 50, {255, 255, 255, 255});
        drawtext("a2", window_width/2, 50, {255, 165, 0, 255}); // highlight priority lane
        drawtext("a3", window_width/2 + 40, 50, {100, 255, 100, 255}); // highlight free lane

        // road b labels
        drawtext("b1", window_width/2 + 40, window_height - 50, {255, 255, 255, 255});
        drawtext("b2", window_width/2, window_height - 50, {255, 255, 255, 255});
        drawtext("b3", window_width/2 - 40, window_height - 50, {100, 255, 100, 255});

        // road c labels
        drawtext("c1", window_width - 50, window_height/2 - 40, {255, 255, 255, 255});
        drawtext("c2", window_width - 50, window_height/2, {255, 255, 255, 255});
        drawtext("c3", window_width - 50, window_height/2 + 40, {100, 255, 100, 255});

        // road d labels
        drawtext("d1", 50, window_height/2 + 40, {255, 255, 255, 255});
        drawtext("d2", 50, window_height/2, {255, 255, 255, 255});
        drawtext("d3", 50, window_height/2 - 40, {100, 255, 100, 255});
    }

    // draw arrows for indicating turn direction for free lanes
    void drawturnarrows() {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // arrow from a to c
        drawarrow(window_width/2 + 125, window_height/2 - 100,
                 window_width/2 + 100, window_height/2 - 100,
                 window_width/2 + 110, window_height/2 - 90);

        // arrow from b to d
        drawarrow(window_width/2 - 125, window_height/2 + 100,
                 window_width/2 - 100, window_width/2 + 100,
                 window_width/2 - 110, window_height/2 + 90);

        // arrow from c to b
        drawarrow(window_width/2 + 100, window_height/2 + 125,
                 window_width/2 + 100, window_height/2 + 100,
                 window_width/2 + 90, window_height/2 + 110);

        // arrow from d to a
        drawarrow(window_width/2 - 100, window_height/2 - 125,
                 window_width/2 - 100, window_height/2 - 100,
                 window_width/2 - 90, window_height/2 - 110);
    }

    // draw an arrow (adapting c code function)
    void drawarrow(int x1, int y1, int x2, int y2, int x3, int y3) {
        // draw arrow line
        SDL_RenderLine(renderer, x1, y1, x2, y2);

        // draw arrowhead
        SDL_RenderLine(renderer, x2, y2, x3, y3);

        // calculate the other point of the arrowhead
        int dx = x3 - x2;
        int dy = y3 - y2;
        int x4 = x2 - dy;
        int y4 = y2 + dx;

        SDL_RenderLine(renderer, x2, y2, x4, y4);
    }

    // render a frame
    void renderframe() {
        if (!active || !renderer || !trafficmanager) {
            return;
        }

        // draw the road layout
        drawroadlayout();

        // draw the traffic lights
        if (trafficmanager->gettrafficlight()) {
            trafficmanager->gettrafficlight()->render(renderer);
        }

        // draw all vehicles
        for (auto* queue : trafficmanager->getqueues()) {
            for (auto* vehicle : queue->getvehicles()) {
                vehicle->render(renderer);
            }
        }

        // draw debug overlay if enabled
        if (showdebug) {
            drawdebugoverlay();
        }

        // present render
        SDL_RenderPresent(renderer);
    }

    // start render loop
    void startrenderloop() {
        if (!active) {
            return;
        }

        log_message("starting render loop");

        bool running = true;
        uint32_t lastupdatetime = SDL_GetTicks();

        while (running) {
            // process events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN) {
                    // handle key presses
                    int key = event.key.which;

                    if (key == SDL_SCANCODE_D) {
                        showdebug = !showdebug;
                        log_message("debug overlay " + std::string(showdebug ? "enabled" : "disabled"));
                    } else if (key == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                }
            }

            // calculate delta time
            uint32_t currenttime = SDL_GetTicks();
            uint32_t deltatime = currenttime - lastupdatetime;

            // update traffic manager
            if (trafficmanager) {
                trafficmanager->update(deltatime);
            }

            // render frame
            renderframe();

            // limit frame rate
            SDL_Delay(16); // ~60 fps

            lastupdatetime = currenttime;
        }
    }

    // draw debug overlay
    void drawdebugoverlay() {
        if (!trafficmanager) return;

        // draw semi-transparent background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect overlayrect = {10, 10, 280, 300};
        SDL_RenderFillRect(renderer, &overlayrect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // draw border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &overlayrect);

        // title
        drawtext("traffic junction simulator", 20, 20, {255, 255, 255, 255});

        // traffic light state
        std::string statestr = "traffic light: ";
        SDL_Color statecolor = {255, 255, 255, 255};

        trafficlight* light = trafficmanager->gettrafficlight();
        if (light) {
            switch (light->getcurrentstate()) {
                case trafficlightstate::all_red:
                    statestr += "all red";
                    statecolor = {255, 100, 100, 255};
                    break;
                case trafficlightstate::a_green:
                    statestr += "a green";
                    statecolor = {100, 255, 100, 255};
                    break;
                case trafficlightstate::b_green:
                    statestr += "b green";
                    statecolor = {100, 255, 100, 255};
                    break;
                case trafficlightstate::c_green:
                    statestr += "c green";
                    statecolor = {100, 255, 100, 255};
                    break;
                case trafficlightstate::d_green:
                    statestr += "d green";
                    statecolor = {100, 255, 100, 255};
                    break;
            }

            if (light->isprioritymode()) {
                statestr += " (priority mode)";
                statecolor = {255, 165, 0, 255};
            }
        }

        drawtext(statestr, 20, 45, statecolor);

        // lane statistics
        drawtext("lane statistics:", 20, 70, {255, 255, 255, 255});

        int y = 95;
        for (auto* queue : trafficmanager->getqueues()) {
            std::string laneinfo = "lane " + std::string(1, queue->getlaneid()) +
                                   std::to_string(queue->getlanenumber()) + ": " +
                                   std::to_string(queue->size()) + " vehicles";

            SDL_Color color = {255, 255, 255, 255};

            // highlight priority lane
            if (queue->isprioritylane() && queue->getpriority() > 0) {
                laneinfo += " (priority)";
                color = {255, 165, 0, 255};
            }
            // highlight free lane
            else if (queue->getlanenumber() == 3) {
                laneinfo += " (free)";
                color = {100, 255, 100, 255};
            }

            drawtext(laneinfo, 30, y, color);
            y += 20;

            // avoid overflow
            if (y > 250) break;
        }

        // controls info
        drawtext("press d to toggle debug overlay", 20, 275, {200, 200, 200, 255});
        drawtext("press esc to exit", 20, 295, {200, 200, 200, 255});
    }

    // draw text using simplest method
    void drawtext(const std::string& text, int x, int y, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_FRect textrect = {(float)x, (float)y, (float)(text.length() * 7), 16.0f};
        SDL_RenderFillRect(renderer, &textrect);

        // add border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &textrect);
    }

    // clean up resources
    void cleanup() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        active = false;
    }
};

// main function
int main(int argc, char* argv[]) {
    try {
        // initialize debug logger
        DebugLogger::initialize();
        log_message("starting traffic junction simulator");

        // initialize sdl
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            log_message("sdl initialization failed: " + std::string(SDL_GetError()));
            return 1;
        }

        // create traffic manager
        trafficmanager trafficmanager;
        if (!trafficmanager.initialize()) {
            log_message("failed to initialize traffic manager");
            SDL_Quit();
            return 1;
        }

        // create renderer
        renderer renderer;
        if (!renderer.initialize(window_width, window_height, "traffic junction simulator")) {
            log_message("failed to initialize renderer");
            SDL_Quit();
            return 1;
        }

        // connect traffic manager to renderer
        renderer.settrafficmanager(&trafficmanager);

        // start traffic manager
        trafficmanager.start();

        // start render loop
        renderer.startrenderloop();

        // cleanup
        trafficmanager.stop();
        renderer.cleanup();
        SDL_Quit();

        log_message("simulator shutdown complete");
        return 0;
    }
    catch (const std::exception& e) {
        log_message("unhandled exception: " + std::string(e.what()));
        SDL_Quit();
        return 1;
    }
}
