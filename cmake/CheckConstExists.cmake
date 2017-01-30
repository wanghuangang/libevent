include(CheckCSourceCompiles)

macro(check_const_exists CONST FILES VARIABLE)
  set(check_const_exists_source "")
  foreach(file ${FILES})
    string(APPEND check_const_exists_source "#include <${file}>\n")
  endforeach()
  string(APPEND check_const_exists_source "\nint main() { (void)${CONST}; return 0; }")

  check_c_source_compiles("${check_const_exists_source}" ${VARIABLE})

  if (${${VARIABLE}})
    set(${VARIABLE} 1 CACHE INTERNAL "Have const ${CONST}")
    message(STATUS "Looking for ${CONST} - found")
  else()
    set(${VARIABLE} 0 CACHE INTERNAL "Have const ${CONST}")
    message(STATUS "Looking for ${CONST} - not found")
  endif()
endmacro(check_const_exists)
