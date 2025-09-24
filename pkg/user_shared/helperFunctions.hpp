#pragma once
#include <string>   // For std::string
#include <cstdlib>  // For std::strtol

static u_int64_t get_id(char * s)
{
  std::string input = std::string(s);
  u_int64_t n = 0;

    // Find the position of '[' and ']'
    size_t start = input.find('[');
    size_t end = input.find(']');

    if (start != std::string::npos && end != std::string::npos && start < end) {
        // Extract the substring containing the number
        std::string number_str = input.substr(start + 1, end - start - 1);

        // Convert the string to u_int64_t
        n = static_cast<u_int64_t>(std::strtol(number_str.c_str(), nullptr, 10));
    }
  return n;
}