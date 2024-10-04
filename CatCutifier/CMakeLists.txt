# Gather all .cpp and .h files in the current directory
file(GLOB_RECURSE CATCUTIFIER_SOURCES "./*.cpp" "./*.h")

# Create the library
add_library(CatCutifier ${CATCUTIFIER_SOURCES})

# Set the include directories
target_include_directories(CatCutifier PUBLIC .)
