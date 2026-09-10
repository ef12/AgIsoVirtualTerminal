if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG 86ed5e521b8d95433eb4688e2ded4d82c32cb383)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
