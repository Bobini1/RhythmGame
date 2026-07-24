#include "ExceptionBoundary.h"

#include <stdexcept>

int crossStaticLibraryBoundary()
{
    throw std::runtime_error{"wasm-native-exception"};
}
