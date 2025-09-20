# Goal
The main porpoise of the project is to learn about various topics:
- The newest C++ standard (C++ 23 for now) like constexpr, coroutines, the newest std library features
- Gathering knowledge about Physical Based Rendering from sources like:
  * https://raytracing.github.io/books
  * https://www.scratchapixel.com/
  * https://www.pbr-book.org/
- Taking knowledge about GPU computing
- Learning Vulkan API
- Learning about cmake, clang, vckpg, GitHub actions and other helpfully tools which can successfully support development process

The project should be:
- Code should be easy to read
- Multiplatform
- Good knowledge source
- Fun :)

## How to build
The configuration is defined in `CMakePresets.json`. This file can be easily modified to meet individual requirements.

1. **Clone the repository** using the following command:
```git clone --recurse-submodules https://github.com/sylmroz/thyme.git```

2. **Bootstrap vcpkg** by navigating to the repository directory:
```
cd thyme
./thirdparty/vcpkg/bootstrap-vcpkg.sh # for non-Windows system

./thirdparty/vcpkg/bootstrap-vcpkg.bat # for Windows
```

3. To build this example, execute the following command in the repository directory:
```
cmake --preset <presetName>
cmake --build --preset <presetName>
```
