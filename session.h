#ifndef SESSION_H
#define SESSION_H

#include <unordered_map>

#include "activity.h"
#include "pair.h"

// Represents a set of activities and the pairings assigned to them
class Session : public std::unordered_map<Activity, Pair> {
public:
    // Which activity to assign a pair to next
    unsigned int free_activity_idx;

public:
    Session() : std::unordered_map<Activity, Pair>(), free_activity_idx(0) {}

    bool operator==(const Session &other) {
        assert(size() == other.size() && "Sessions must have the same size");
        assert(size() == NUM_ACTIVITIES && "Session does not have correct number of activities"); 
        // Check if the mappings between activity and pair are the same between the two sessions
        for (const auto& [activity, pair] : *this) {
            auto it = other.find(activity);
            assert(it != other.end() && "Activities must be the same between the two sessions");
            // Check if the Activity -> Pair mapping is the same
            if (it->second != pair) {
                return false;
            }
        }
        // All mappings are the same
        return true;
    }

    bool complete() const {
        return size() == NUM_ACTIVITIES;
    }

    // Assign a Pair to the next available Activity
    const Activity assign_pair(const Pair &pair) {
        assert(free_activity_idx < NUM_ACTIVITIES && "Too many pairings in the session");
        const Activity &activity = activities[free_activity_idx];
        (*this)[activity] = pair;
        // Increment the index to point to the next activity without an assigned Pair
        do {
            free_activity_idx++;
        } while (find(activities[free_activity_idx]) != end());
        return activity;
    }

    // Free up the given Activity so that it does not have an assigned Pair anymore
    void free_activity(const Activity &activity) {
        if (find(activity) == end()) {
            // Activity is already freed
            return;
        }
        // Get the index of the Activity in the activities array and assign it to the
        // free activity index
        free_activity_idx =
            std::find(activities.begin(), activities.end(), activity) - activities.begin();
        // Remove the activity from the map
        erase(activity);
    }
};

namespace std {
    template<>
    struct hash<Session> {
        size_t operator()(const Session& s) const {
            size_t hashVal = 0;
            for (const auto& [activity, pair] : s) {
                // Combine hashes of activity and pair using XOR
                hashVal ^= std::hash<Activity>()(activity) ^ std::hash<Pair>()(pair);
            }
            return hashVal;
        }
    };
}

#endif // SESSION_H