if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG 9db78b52afb6c6e0479df3c2d09d48a7eb307481)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
