#ifndef FACILITATOR_H
#define FACILITATOR_H

#include <string>

#include "position.h"

// Represents a person, that as part of a pair, leads a given activity
class Facilitator {
public:
    // Member variables
    std::string name;
    std::optional<Position> position;

public:
    // Constructors
    Facilitator(): name("") {}
    Facilitator(std::string n, Position p) : name(n), position(p) {
        if (name.empty()) {
            throw std::invalid_argument("Name cannot be an empty string");
        }
    }
    Facilitator(const Facilitator &other) : name(other.name), position(other.position) {}

    Facilitator& operator=(const Facilitator &other) {
        if (this != &other) {
            name = other.name;
            position = other.position;
        }
        return *this;
    }

    bool operator==(const Facilitator &other) const {
        return name == other.name && position == other.position;
    }

    // Returns if the Facilitator represents an "empty" Facilitator.
    // Empty Facilitators are used to create an "empty" pair.
    bool is_empty() const {
        return !position;
    }

    bool is_junior() const {
        return position && position.value() == Position::junior;
    }

    std::string to_string() const {
        if (is_empty()) {
            return "Facilitator()";
        }
        return "Facilitator( name: " + name + " - " + positionToStr(position.value()) + " )";
    }
};

namespace std {
    template<>
    struct hash<Facilitator> {
        std::size_t operator()(const Facilitator &f) const {
            std::size_t hashValue = std::hash<std::string>()(f.name);
            hashValue ^= std::hash<std::optional<Position>>()(f.position); // Combine hash values
            return hashValue;
        }
    };
}

#endif // FACILITATOR_H