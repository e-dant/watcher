file(REMOVE_RECURSE
  "snitch/snitch_all.hpp"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/snitch-header-only.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
