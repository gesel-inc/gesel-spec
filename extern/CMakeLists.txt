include(FetchContent)

FetchContent_Declare(
  byteme
  GIT_REPOSITORY https://github.com/LTLA/byteme
  GIT_TAG master # ^2.1.2
)

FetchContent_Declare(
  sanisizer
  GIT_REPOSITORY https://github.com/LTLA/sanisizer
  GIT_TAG master # ^0.2.0
)

FetchContent_MakeAvailable(byteme)
FetchContent_MakeAvailable(sanisizer)
