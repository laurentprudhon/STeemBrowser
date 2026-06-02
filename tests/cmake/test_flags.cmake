# Common compiler flags for test builds, matching the production build
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -Wfatal-errors -fpermissive -O2"
  CACHE STRING "" FORCE
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -Wfatal-errors"
  CACHE STRING "" FORCE
)
