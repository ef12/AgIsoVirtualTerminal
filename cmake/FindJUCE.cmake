if(NOT TARGET JUCE::JUCE)
  # First, try to use an external JUCE installation if JUCE_ROOT is set
  set(JUCE_ROOT_PATH "$ENV{JUCE_ROOT}" CACHE STRING "External JUCE installation path")
  
  if(JUCE_ROOT_PATH AND EXISTS "${JUCE_ROOT_PATH}/CMakeLists.txt")
    message(STATUS "Using external JUCE from: ${JUCE_ROOT_PATH}")
    add_subdirectory("${JUCE_ROOT_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
  else()
    if(JUCE_ROOT_PATH)
      message(WARNING "JUCE_ROOT set to '${JUCE_ROOT_PATH}' but CMakeLists.txt not found")
    endif()
    message(STATUS "Using FetchContent to download JUCE framework")
    include(FetchContent)
    FetchContent_Declare(
      JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.8
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()
endif()
