#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "activity.h"
#include "session.h"

constexpr unsigned int NUM_SESSIONS = 6;

// Represents a collection of sessions
class Schedule : public std::vector<Session> {
public:
    // Number of conflicts in the schedule
    unsigned int conflicts;
    // Track which activities have been run by which facilitators from the sessions
    // chosen in this schedule
    std::unordered_map<Activity, std::unordered_set<Facilitator>> facilitator_activities;
    // Track which pairings have already been selected from the sessions chosen in this
    // schedule
    std::unordered_set<Pair> selected_pairings;

public:
    // Constructors
    Schedule() : std::vector<Session>(), conflicts(0) {}
    Schedule(unsigned int conflicts) : std::vector<Session>(), conflicts(conflicts) {}
    Schedule(const Schedule &other) :
        std::vector<Session>(other),
        conflicts(other.conflicts),
        facilitator_activities(other.facilitator_activities),
        selected_pairings(other.selected_pairings) {}

    Schedule& operator=(const Schedule &other) {
        if (this != &other) {
            // Call base class's copy assignment operator
            std::vector<Session>::operator=(other);
            conflicts = other.conflicts;
            facilitator_activities = other.facilitator_activities;
            selected_pairings = other.selected_pairings;
        }
        return *this;
    }

    bool operator==(const Schedule &other) {
        assert(size() == NUM_SESSIONS && "Schedule has too many sessions");
        assert(other.size() == NUM_SESSIONS && "Schedule has too many sessions");
        // Construct maps that map a Session to the number of times it appears in the Schedule
        // for both this Schedule and the one being compared. Then check if the mapping is the
        // same. Ex. A schedule with of [ A, B, C, C ] == [ C, B, C, A ]
        std::unordered_map<Session, int> session_count;
        std::unordered_map<Session, int> other_session_count;
        for (const auto& session : *this) {
            session_count[session]++;
        }
        for (const auto& session : other) {
            other_session_count[session]++;
        }
        if (session_count.size() != other_session_count.size()) {
            return false;
        }
        return std::equal(session_count.begin(), session_count.end(), other_session_count.begin());
    }

    bool complete() const {
        return size() == NUM_SESSIONS;
    }

    // Add a session to the schedule
    void add_session(const Session &session) {
        assert(size() < NUM_SESSIONS && "Schedule has too many sessions");
        // Iterate over each Activity -> Pair mapping and update the internal mappings
        for (const auto& [activity, pairing] : session) {
            // Ignore empty pairings, since they won't affect any of the internal mappings
            if (pairing.is_empty_pair()) continue;
            // If the pairing from the new session has already been seen before in this schedule,
            // add one to the conflict score
            if (selected_pairings.count(pairing)) ++conflicts;
            // For each facilitator in the pairing, if they've been scheduled before for the same
            // activity in another session, then add one to the conflict score
            if (facilitator_activities[activity].count(pairing.p.first)) ++conflicts;
            if (facilitator_activities[activity].count(pairing.p.second)) ++conflicts;
            // Finally, update the internal mappings
            facilitator_activities[activity].insert(pairing.p.first);
            facilitator_activities[activity].insert(pairing.p.second);
            selected_pairings.insert(pairing);
        }
        // Finally, add the session to the end of the schedule
        push_back(session);
    }
};

#endif // SCHEDULE_H