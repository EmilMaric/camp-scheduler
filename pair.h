#ifndef PAIR_H
#define PAIR_H

#include <cassert>

#include "facilitator.h"

// Represents a pair of Facilitators, that together lead an Activity
class Pair {
public:
    std::pair<Facilitator, Facilitator> p;

public:
    // Constructors
    Pair() : p(Facilitator(), Facilitator()) {}
    Pair(const Facilitator &f, const Facilitator &s) : p(f, s) {
        assert(f.is_empty() == s.is_empty() &&
               "Pair must consist of two non-empty Facilitators or two empty facilitators");
    }
    Pair(const Pair &other) : p(other.p) {}

    Pair& operator=(const Pair &other) {
        if (this != &other) {
            p = other.p;
        }
        return *this;
    }

    bool operator==(const Pair &other) const {
        return
            (p.first == other.p.first && p.second == other.p.second) ||
            (p.first == other.p.second && p.second == other.p.first);
    }

    bool operator!=(const Pair &other) const {
        return !(*this == other);
    }

    // Returns if the pair represents an "empty" pair, which consists of two default/empty
    // Facilitators
    bool is_empty_pair() const {
        return p.first.is_empty() && p.second.is_empty();
    }

    // Check if the passed-in facilitator is within this pair
    bool contains(const Facilitator & f) const {
        return p.first == f || p.second == f;
    }

    // Check if the pairing consists of two juniors
    bool is_junior_pairing() const {
        return p.first.is_junior() && p.second.is_junior();
    }

//    bool operator<(const Pair &other) const {
//        if (*this == other) {
//            return false;
//        }
//        if (other.contains(p.first)) {
//            // First element is found in the other pair, then comapre the
//            // non-matching elements
//            if (p.first == other.p.first) {
//                return p.second < other.p.second;
//            } else {
//                return p.second < other.p.first;
//            }
//        }
//        if (other.contains(p.second)) {
//            // Second element is found in the other pair, then compare the
//            // non-matching elements
//            if (p.second == other.p.second) {
//                return p.first < other.p.first;
//            } else {
//                return p.first < other.p.second;
//            }
//        }
//        // Pairs have no matching elements - see which pair has the "smallest" Facilitator
//        return (p.first < other.p.first && p.first < other.p.second) || (p.second < other.p.first && p.second < other.p.second);
//    }

    std::string to_string() const {
        return "Pair( " + p.first.to_string() + ", " + p.second.to_string() + " )";
    }
};

namespace std {
    template<>
    struct hash<Pair> {
        std::size_t operator()(const Pair &pair) const {
            std::size_t hashValue = std::hash<Facilitator>()(pair.p.first);
            hashValue ^= std::hash<Facilitator>()(pair.p.second);
            return hashValue;
        }
    };
}

#endif // PAIR_H