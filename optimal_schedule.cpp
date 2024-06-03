#include <iostream>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <boost/multiprecision/cpp_int.hpp>

#include "thread_pool.h"
#include "activity.h"
#include "facilitator.h"
#include "session.h"
#include "schedule.h"

// Helper to create arrays without needing provide an explicit size
template<typename T, typename... N>
auto make_array(N&&... args) -> std::array<T,sizeof...(args)>
{
    return {std::forward<N>(args)...};
}

// ------------------------ Global variables -------------------------

// List of facilitators
const auto facilitators = make_array<Facilitator>(
    Facilitator("Adam Apples", Position::junior),
    Facilitator("Betty Blues", Position::junior),
    Facilitator("Charles Chapman", Position::junior),
    Facilitator("Daisy Duke", Position::junior),
    Facilitator("Earl Eastman", Position::junior),
    Facilitator("Fred Flinstone", Position::junior),
    Facilitator("Gabriella Gabon", Position::senior),
    Facilitator("Henrik Hanson", Position::senior),
    Facilitator("Inge Ingram", Position::senior),
    Facilitator("John Jones", Position::senior)
);

const int num_activities = activities.size();
// Will measure the start time of the algorithm
std::chrono::high_resolution_clock::time_point start_time;
// Will be used to measure the time that an interval began
std::chrono::high_resolution_clock::time_point t1;
// Contains every possible permutation of a session given the possible pairings
std::unordered_set<Session> session_permutations;
// Minimum schedule found out of all the schedule permutations. Initialize it with the
// maximum conflicts you wish a schedule to have. The algorithm below will start looking for
// schedules that are less than this maximum.
//
// Uncomment the following if you want to initially set maximum conflicts to a really high number
//Schedule min_schedule{UINT_MAX};
Schedule min_schedule{7};
// Initialize the thread pool
ThreadPool threadPool(std::thread::hardware_concurrency());
// ------------------------ End of Global variables section -------------------------


// ------------------------ Main algorithm -------------------------

// Print the Schedule into a file
void print_schedule(const Schedule &schedule) {
    // Create an ofstream object and open the file for writing (default mode is std::ios::out)
    std::ofstream outFile("min_schedule.txt");
    // Check if the file opened successfully
    if (!outFile) {
        std::cerr << "Error: Could not open the file." << std::endl;
        return; // Return a non-zero value to indicate an error
    }

    int session_idx = 0;
    for (const Session &session : schedule ) {
        outFile << "=======================\n";
        outFile << " Session " << session_idx << "\n";
        outFile << "=======================\n";
        for (const auto& [activity, pair] : session) {
            outFile << activity << " - " << pair.p.first.name << " + " << pair.p.second.name << "\n";
        }
        outFile << "\n";
        session_idx++;
    }
    outFile << "Schedule Conflicts: " << schedule.conflicts << "\n\n";

    // Close the file
    outFile.close();

    // Inform the user that the operation was successful
    std::cout << "Schedule with " << schedule.conflicts << " conflicts has been found!" << std::endl;
}

// Given a set of possible pairings and one selected pairing, generate a new set of possible pairings
// by only including pairings from the list of possible pairings that are still valid to select
std::unordered_set<Pair> generate_possible_pairings(
    const Pair &selected_pairing,
    const std::unordered_set<Pair> &possible_pairings) {
    // Create the generated set of new possible pairings
    std::unordered_set<Pair> remaining_pairings;
    for (const Pair &p : possible_pairings) {
        // If the current pairing has a Facilitator from the selected pairing, skip adding it to the
        // new set
        if (p.contains(selected_pairing.p.first) || p.contains(selected_pairing.p.second)) {
            continue;
        }
        // If the current pairing is a junior pairing and the selected pairing is also a junior
        // pairing, skip adding it to the new set - we can only have one junior pairing in a session
        if (selected_pairing.is_junior_pairing() && p.is_junior_pairing()) {
            continue;
        }
        // Otherwise, add the current pairing to the new set of possible pairings
        remaining_pairings.insert(p);
    }
    return remaining_pairings;
}

// Recursively generate all possible permutations of a Session.
void generate_sessions(
    const std::unordered_set<Pair> &possible_pairings,
    Session &session
) {
    // If the Session is complete (ie. we have a pairing for each activity) then add it to the
    // set of Session permutations
    if (session.complete()) {
        // A full session permutation has been generated
        session_permutations.insert(session);
        return;
    }
    // Iterate over each possible pairing, and add it to the next activity in the Session
    // and then recurse further to complete the Session
    std::cout << "Starting loop" << std::endl;
    for (const Pair &selected_pairing : possible_pairings) {
        std::unordered_set<Pair> remaining_available_pairings = generate_possible_pairings(selected_pairing, possible_pairings);
        std::cout << remaining_available_pairings.size() << std::endl;
        const Activity &activity = session.assign_pair(selected_pairing);
        generate_sessions(remaining_available_pairings, session);
        session.free_activity(activity);
    }
}

// Update stats on the iterations performed so far
void update_iterations(
    const boost::multiprecision::uint128_t &new_full_iterations,
    const boost::multiprecision::uint128_t &new_skipped_iterations
) {
    // Define a mutex to protect access to iteration counters
    static std::mutex iteration_mutex;

    // Lock the mutex to ensure exclusive access to iteration counters
    std::lock_guard<std::mutex> lock(iteration_mutex);

    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    // Total number of schedule permutations iterated over (includes skipped permutations)
    static boost::multiprecision::uint128_t total_iterations = 0;
    // Total number of schedule permutations that were fully iterated over
    static boost::multiprecision::uint128_t total_full_iterations = 0;
    // Total number of schedule permutations skipped over
    static boost::multiprecision::uint128_t total_skipped_iterations = 0;
    // Last iteration count that we printed for, rounded down to the nearest trillion
    static boost::multiprecision::uint128_t last_iteration_count_printed = 0;
    // Print the iteration count every trillion iterations
    static const boost::multiprecision::uint128_t granularity = 1000000000000;

    total_full_iterations += new_full_iterations;
    total_skipped_iterations += new_skipped_iterations;
    total_iterations += ( new_full_iterations + new_skipped_iterations );
    assert(total_full_iterations + total_skipped_iterations == total_iterations);

    if (total_iterations - last_iteration_count_printed >= granularity) {
        auto t2 = high_resolution_clock::now();
        auto interval_ms_int = duration_cast<milliseconds>(t2 - t1);
        auto total_s_int = duration_cast<seconds>(t2 - start_time);
        // Save the last iteration count rounded down to the nearest trillion. For example, if the total
        // iteration count is 4,765,432,000,000 the last iteration count will be 4,000,000,000,000.
        // We do this to be able to print every 1T iterations.
        last_iteration_count_printed = (total_iterations / granularity) * granularity;
        std::stringstream out;
        out << "Iteration count (in trillions): " << (last_iteration_count_printed / granularity) << "T, "<<
                     "total time (s): " << total_s_int.count() << ", " <<
                     "interval time (ms): " << interval_ms_int.count() << "\n";
        out << "Full iterations: " << total_full_iterations << ", " <<
                     "Skipped iterations: " << total_skipped_iterations << ", " <<
                     "Total iterations: " << total_iterations << "\n\n";
        std::cout << out.str() << std::endl;
        // Reset the clock for the interval
        t1 = high_resolution_clock::now();
    }
}

// Main algorithm to iterate over possible schedule permutations, calculate their conflict score,
// and compare that score to the conflict score of the schedule with the fewest number of conflicts
// found so far
void generate_schedules(Schedule &schedule) {
    static const int session_permutations_size = session_permutations.size();
    // Define a mutex to protect access to min_schedule
    static std::mutex min_schedule_mutex;

    {
        // Lock the mutex to ensure exclusive access to min_schedule
        std::lock_guard<std::mutex> lock(min_schedule_mutex);
        if (schedule.conflicts >= min_schedule.conflicts) {
            // Figure out how many schedule iterations were skipped and add that to the
            // iteration count. Even if we skipped iterations, we assume they were performed
            // for the purposes of printing the number of iterations performed.
            const int remaining_sessions = NUM_SESSIONS - schedule.size();
            const boost::multiprecision::uint128_t iterations_skipped(
                pow(session_permutations_size, remaining_sessions));
            update_iterations(0, iterations_skipped);
            return;
        }
        else if (schedule.complete()) {
            // We've completed building a schedule and it has the fewest conflicts we've
            // encountered so far - save it as such
            min_schedule = schedule;
            print_schedule(min_schedule);
            update_iterations(1, 0);
            return;
        }
    }

    // Iterate over each possible session, and add it to the schedule and recurse down further
    // to build the schedule
    for (const auto &session : session_permutations) {
        // Schedule the recursive function to run as a task
        threadPool.enqueue([schedule, session]() mutable {
            schedule.add_session(session);
            generate_schedules(schedule);
        });
    }
}

int main() {
    // Generate a set of all seniors and all juniors
    std::unordered_set<Facilitator> seniors, juniors;
    for (const Facilitator &facilitator : facilitators) {
        if (facilitator.position.value() == Position::senior) seniors.insert(facilitator);
        else juniors.insert(facilitator);
    }

    // Generate senior <--> junior pairings and junior <--> junior pairings
    std::unordered_set<Pair> senior_junior_pairings, junior_junior_pairings;
    for (const Facilitator &senior : seniors) {
        for (const auto &junior : juniors) {
            senior_junior_pairings.insert(Pair(senior, junior));
        }
    }
    for (const Facilitator &juniorA : juniors) {
        for (const Facilitator &juniorB : juniors) {
            // You can't pair a junior with himself/herself
            if (juniorA == juniorB) {
                continue;
            }
            junior_junior_pairings.insert(Pair(juniorA, juniorB));
        }
    }

    // Insert the senior - junior pairings into the set of all possible pairings
    std::unordered_set<Pair> pairings = senior_junior_pairings;
    // Insert the junior - junior pairings into the set of all possible pairings
    pairings.insert(junior_junior_pairings.begin(), junior_junior_pairings.end());
    // Insert the empty pair into the set of all possible pairings
    pairings.insert(Pair());

    // Generate a set of all possible session permutations using the available pairings
    Session session{};
    generate_sessions(pairings, session);

    std::cout << "Number of possible session permutations: " << session_permutations.size() << std::endl;
    std::cout << "Number of possible iterations: " << pow(session_permutations.size(), NUM_SESSIONS) << "\n\n";

    try {
        Schedule schedule;
        // Start the clock now for when the algorithm starts
        start_time = std::chrono::high_resolution_clock::now();
        threadPool.enqueue([schedule]() mutable {
            generate_schedules(schedule);
        });
        threadPool.wait_finished();
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception caught!" << std::endl;
    }

    std::cout << "Exiting" << std::endl;
    return EXIT_SUCCESS;
}

// ------------------------ End of Main algorithm section -------------------------