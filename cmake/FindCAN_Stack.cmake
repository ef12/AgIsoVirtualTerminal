if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG a9b51d5e60e69c273cfac4362e1fe0ff593d3331)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
