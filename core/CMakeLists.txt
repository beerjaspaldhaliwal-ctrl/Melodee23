add_library(melodee_core STATIC
    src/Theory.cpp
    src/Scales.cpp
    src/Chords.cpp
    src/Progression.cpp
    src/ProgressionLibrary.cpp
    src/Harmony.cpp
    src/MelodyStyles.cpp
    src/MelodyGenerator.cpp
    src/MelodyTransforms.cpp
    src/MelodyEmbellish.cpp
    src/Accompaniment.cpp
    src/MidiFile.cpp
)

target_include_directories(melodee_core PUBLIC include)
target_compile_features(melodee_core PUBLIC cxx_std_17)
set_target_properties(melodee_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(melodee_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(melodee_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
