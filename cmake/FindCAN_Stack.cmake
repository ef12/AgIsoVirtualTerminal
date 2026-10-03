if(NOT TARGET isobus::isobus)
  include(FetchContent)
  FetchContent_Declare(
    CAN_Stack
    GIT_REPOSITORY https://github.com/ef12/AgIsoStack-plus-plus.git
    GIT_TAG f2386d910e43f1ba25a14cb8b45adde6e3aaba28)
  FetchContent_MakeAvailable(CAN_Stack)
endif()
