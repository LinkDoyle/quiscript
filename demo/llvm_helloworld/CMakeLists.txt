cmake_minimum_required (VERSION 3.10)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(llvm_helloworld_SRCS
    main.cpp
)

llvm_map_components_to_libnames(llvm_libs
  Core
  ExecutionEngine
  Interpreter
  orcjit
  Support
  nativecodegen
  )

add_executable(llvm_helloworld ${llvm_helloworld_SRCS})
target_link_libraries(llvm_helloworld ${llvm_libs})

set_target_properties(llvm_helloworld PROPERTIES FOLDER "Demo")
