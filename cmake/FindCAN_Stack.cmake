if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG 49a5a642743dfd74b9584486ec540d0bc8c543a8)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
