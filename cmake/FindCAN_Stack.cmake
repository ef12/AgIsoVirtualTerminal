if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG 9a320161189c2015a762c5f81dd346f4c167905f)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
