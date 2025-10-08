module;

#include <stb_image.h>

export module stb.image;

export {
using ::stbi_load;
using ::stbi_image_free;

using ::STBI_default;

using ::STBI_grey;
using ::STBI_grey_alpha;
using ::STBI_rgb;
using ::STBI_rgb_alpha;
}
