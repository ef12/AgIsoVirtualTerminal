if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG 1eb0a89f21e0c2ea57a2c63b93a7d3b2218bc66f)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
