#ifndef POSITION_H
#define POSITION_H

#include <string>

// Represents a Facilitator's position
enum class Position {
    senior,
    junior
};

// Convert a Position enum value into a string representation
std::string positionToStr(const Position &p) {
    switch (p) {
        case Position::senior:
            return "senior";
        case Position::junior:
            return "junior";
    }
    throw std::invalid_argument("Unexpected value");
}

namespace std {
    template<>
    struct hash<std::optional<Position>> {
        std::size_t operator()(const std::optional<Position> &opt) const {
            if (opt.has_value()) {
                return static_cast<size_t>(opt.value());
            }
            return 0; // Return 0 for empty optional
        }
    };
}

#endif // POSITION_H