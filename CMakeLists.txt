cmake_minimum_required(VERSION 3.16) # Lowered for better FetchContent compatibility
project(MySFMLGame LANGUAGES CXX) # Changed project name for clarity, use your actual name

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# For static linking of SFML, you'd typically set SFML_USE_STATIC_LIBS before FetchContent_MakeAvailable
# option(BUILD_SHARED_LIBS "Build shared libraries" OFF) # This is for YOUR project, SFML controls its own
set(SFML_USE_STATIC_LIBS ON) # Tell SFML to prefer static linking for itself

include(FetchContent)

# SFML
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
)
# Optional: Control which SFML components are built if needed (usually defaults are fine)
# set(SFML_BUILD_GRAPHICS ON CACHE BOOL "" FORCE)
# ... other components ...
FetchContent_MakeAvailable(SFML)

# RapidJSON
FetchContent_Declare(rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson
    GIT_TAG master # Consider a specific release tag, e.g., v1.1.0
)
# RapidJSON specific settings (these prevent it from building its own docs/tests)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidjson)

# Your Executable
add_executable(main # Use your project name if it's not 'main'
    src/main.cpp
    src/PlatformBody.cpp
    src/Tile.cpp
    src/Player.cpp
    src/CollisionSystem.cpp
    src/Optimizer.cpp
    src/LevelManager.cpp
    src/SpriteManager.cpp
)
    
# Copy Assets to be next to your executable in the build/bin directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
target_link_libraries(main PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

target_compile_features(main PRIVATE cxx_std_17)

# Include directories
target_include_directories(main PUBLIC 
    ${PROJECT_SOURCE_DIR}/include   # For your own project's headers, if any
    ${rapidjson_SOURCE_DIR}/include # For RapidJSON headers
    # SFML include directories are automatically handled by linking SFML::xxx targets
)