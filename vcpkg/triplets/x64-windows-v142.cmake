set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Build the dependencies with the VS 2019 toolset, so that they are compiled against the same
# standard library as sqlite_orm itself. Consuming v143 binaries from a v142 build is not covered
# by the binary compatibility guarantee, which only extends from older toolsets to newer ones.
set(VCPKG_PLATFORM_TOOLSET v142)
