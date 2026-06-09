/* Compat shims to keep the binary linkable against EmuELEC v4.6 (aarch64,
 * glibc 2.36). GCC 14 / Ubuntu 26.04 emits references to the C23-named
 * wrappers `__isoc23_strto{l,ul,ll,ull}` which only exist in glibc 2.38+.
 * Static libstdc++ pulls these in via std::from_chars and locale code.
 *
 * Providing strong local definitions forces the linker to satisfy those
 * references from this TU (no dynamic GLIBC_2.38 reference produced).
 *
 * Only relevant for aarch64 Linux cross builds.
 */

#if defined(__linux__) && defined(__aarch64__)

#include <stdlib.h>

long __isoc23_strtol(const char* nptr, char** endptr, int base) {
    return strtol(nptr, endptr, base);
}

unsigned long __isoc23_strtoul(const char* nptr, char** endptr, int base) {
    return strtoul(nptr, endptr, base);
}

long long __isoc23_strtoll(const char* nptr, char** endptr, int base) {
    return strtoll(nptr, endptr, base);
}

unsigned long long __isoc23_strtoull(const char* nptr, char** endptr, int base) {
    return strtoull(nptr, endptr, base);
}

#endif
