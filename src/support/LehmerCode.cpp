//
// Created by PC on 11/08/2024.
//

#include "LehmerCode.h"

namespace support {

uint64_t factorial(int n) {
    auto fact = uint64_t{ 1 };
    for (int i = 2; i <= n; i++) {
        fact *= i;
    }
    return fact;
}

uint64_t encodePermutation(std::span<const int> permutation) {
    int n = permutation.size();
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

QList<int> decodePermutation(uint64_t lehmerCode, int n) {
    QList<int> permutation(n);
    std::vector<int> availableElements(n);
    
    // Initialize the available elements
    for (int i = 0; i < n; i++) {
        availableElements[i] = i;
    }
    
    for (int i = 0; i < n; i++) {
        uint64_t factorialValue = factorial(n - i - 1);
        int index = lehmerCode / factorialValue;
        permutation[i] = availableElements[index];
        
        // Remove the used element
        availableElements.erase(availableElements.begin() + index);
        
        // Update the Lehmer code
        lehmerCode %= factorialValue;
    }
    
    return permutation;
}

} // namespace support