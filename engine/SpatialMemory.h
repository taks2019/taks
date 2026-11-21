#pragma once
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

namespace taks {

// This is each character's personal, learned spatial knowledge, not a UI map.
// Landmark IDs: 0=camp, 1=cave entrance, 2..3=water,
// 4..15=resource sites, 16..18=trees. 0=player, 1..4=NPCs.
// The renderer and physics can know the whole scene; characters cannot.
struct SpatialMemory {
    static constexpr int people = 5;
    static constexpr int landmarkCount = 19;
    static constexpr int resourcesBegin = 4;
    std::array<std::uint32_t, people> known{};

    bool knows(int person, int landmark) const {
        return person >= 0 && person < people &&
               landmark >= 0 && landmark < landmarkCount &&
               (known[person] & (std::uint32_t{1} << landmark)) != 0;
    }
    bool learn(int person, int landmark) {
        if(person < 0 || person >= people || landmark < 0 || landmark >= landmarkCount)
            return false;
        const std::uint32_t bit = std::uint32_t{1} << landmark;
        const bool discovered = (known[person] & bit) == 0;
        known[person] |= bit;
        return discovered;
    }
    unsigned count(int person) const {
        if(person < 0 || person >= people) return 0;
        std::uint32_t bits = known[person];
        unsigned n = 0;
        while(bits) { n += bits & 1u; bits >>= 1; }
        return n;
    }
    // Knowledge moves only after an actual encounter of two nearby characters.
    bool exchange(int a, int b) {
        if(a < 0 || a >= people || b < 0 || b >= people || a == b) return false;
        const std::uint32_t combined = known[a] | known[b];
        const bool changed = combined != known[a] || combined != known[b];
        known[a] = known[b] = combined;
        return changed;
    }
    void write(std::ostream& out) const {
        out << " MEM1";
        for(std::uint32_t bits: known) out << ' ' << bits;
    }
    bool read(std::istream& in) {
        std::string tag;
        if(!(in >> tag) || tag != "MEM1") return false;
        SpatialMemory candidate;
        constexpr std::uint32_t allowed = (std::uint32_t{1} << landmarkCount) - 1;
        for(auto& bits: candidate.known) {
            unsigned long long raw = 0;
            if(!(in >> raw) || raw > allowed) return false;
            bits = static_cast<std::uint32_t>(raw);
        }
        *this = candidate;
        return true;
    }
};
}
