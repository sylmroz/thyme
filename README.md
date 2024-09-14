# Goal
The main porpouse of the project is to learn about various topics:
- The newest C++ standard (C++ 23 for now) like constexpr, coroutines, the newest std library features
- Gathering knowledge about Physical Based Rendering from sources like:
  * https://raytracing.github.io/books
  * https://www.scratchapixel.com/
  * https://www.pbr-book.org/
- Taking knowlege about GPU computing
- Learning Vulkan API
- Learing about cmake, clang, vckpg, github actions and other helpfull tools which can succesfully support development process

The project should be:
- Code should be easy to read
- Multplatform
- Good knowledge source
- Fun :)

## How to build
The configuration is defined in `CMakePresets.json`. This file can be easly modified to meet individual requirements.

1. **Clone the repository** using the following command command:
```git clone --recurse-submodules https://github.com/sylmroz/thyme.git```

2. **Bootstrap vcpkg** by navigating to the repository directory:
```
cd thyme
./thirdparty/vcpkg/bootstrap-vcpkg.sh # for non-Windows system

./thirdparty/vcpkg/bootstrap-vcpkg.bat # for Windows
```

3. To build this example, execute the following command in the repository directory:
```cmake --preset <presetName>
cmake --build --<presetName>
```
