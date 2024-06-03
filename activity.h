#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <string>
#include <array>

constexpr std::array activities = {
    "Hank's Planks",
    "Spider Web",
    "Lava Bridge",
    "Shepard",
    "Helium Sticks",
    "Balance Board"
};
constexpr unsigned int NUM_ACTIVITIES = activities.size();
using Activity = std::string;

#endif // ACTIVITY_H