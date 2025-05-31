#pragma once
#include <unordered_map>
#include <chrono>

// A timer that measures the runtime of a scope. The timer starts on declaration,
// and ends when destroyed.
class scope_timer {
    float& elapsed_output;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

public:
    // Elapsed time is written to the specified location on destroy
    scope_timer(float& elapsed_output);

    /// @brief Elapsed time is saved into the map with the specified key on destroy.
    ///
    /// If the specified key doesn't exist, it'll be inserted automatically.
    scope_timer(std::unordered_map<const char*, float>& map, const char* name);
    ~scope_timer(); // Ends timer
};
