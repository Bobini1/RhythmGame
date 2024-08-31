//
// Created by PC on 11/08/2024.
//

#include "LehmerCode.h"

namespace support {

auto factorial(int64_t n) -> uint64_t {
    auto fact = uint64_t{ 1 };
    for (int i = 2; i <= n; i++) {
        fact *= i;
    }
    return fact;
}

auto encodePermutation(std::span<const int64_t> permutation) -> uint64_t {
    auto n = permutation.size();
    std::vector<bool> used(n, false);
    uint64_t lehmerCode = 0;

    for (int i = 0; i < n; i++) {
        int count = 0;
        for (int j = 0; j < permutation[i]; j++) {
            if (!used[j]) {
                count++;
            }
        }
        lehmerCode += count * factorial(n - i - 1);
        used[permutation[i]] = true;
    }

    return lehmerCode;
}

auto decodePermutation(uint64_t lehmerCode, int n) -> QList<int64_t> {
    QList<int64_t> permutation(n);
    std::vector<int64_t> availableElements(n);
    
    // Initialize the available elements
    for (int i = 0; i < n; i++) {
        availableElements[i] = i;
    }
    
    for (int i = 0; i < n; i++) {
        auto factorialValue = factorial(n - i - 1);
        auto index = lehmerCode / factorialValue;
        permutation[i] = availableElements[index];
        
        // Remove the used element
        availableElements.erase(availableElements.begin() + index);
        
        // Update the Lehmer code
        lehmerCode %= factorialValue;
    }
    
    return permutation;
}

} // namespace support