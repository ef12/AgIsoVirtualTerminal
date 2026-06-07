if(NOT TARGET JUCE::JUCE)
  set(JUCE_SUBMODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/../external/JUCE")

  if(EXISTS "${JUCE_SUBMODULE_DIR}/CMakeLists.txt")
    message(STATUS "Using JUCE from submodule: ${JUCE_SUBMODULE_DIR}")
    add_subdirectory("${JUCE_SUBMODULE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
  else()
    message(STATUS "JUCE submodule not found, using FetchContent to download")
    include(FetchContent)
    FetchContent_Declare(
      JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.8
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()
endif()
