if(NOT TARGET sil_lib)
  include(FetchContent)
  FetchContent_Declare(
    SIL
    GIT_REPOSITORY https://github.com/ef12/SIL.git
    GIT_TAG main)
  FetchContent_MakeAvailable(SIL)
endif()
