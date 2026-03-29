#include "scope_timer.hxx"

#include <chrono>
#include <common/int.h>

scope_timer::scope_timer(float& elapsed_output) : elapsed_output(elapsed_output) {
    start_time = std::chrono::high_resolution_clock().now();
}

scope_timer::scope_timer(std::unordered_map<const char*, float>& map, const char* name, bool sum)
// I've never seen the "," operator used in the wild, so I think this warrants
// an explanation. "," does nothing but has the lowest operator precedence, so
// the last part after the "," is the result of the expression.
// So this initializer runs the map insertion, then assigns the reference from ".at()".
// - torph
    : elapsed_output((map.insert({name, 0}), map.at(name))), sum(sum)
{
    start_time = std::chrono::high_resolution_clock().now();
}

scope_timer::~scope_timer() {
    const std::chrono::high_resolution_clock clock;
    const std::chrono::duration elapsed = clock.now() - start_time;

    // We get the time in microsec then convert to millisec, to get 3 decimal
    // places of extra precision
    const auto us_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    const float ms_elapsed = us_elapsed.count() / 1000.0f;
    if (sum) {
        elapsed_output += ms_elapsed;
    } else {
        elapsed_output = ms_elapsed;
    }
}
