add_llvm_library(LLVMObfuscation
  CryptoUtils.cpp
  DiverseFunction.cpp
  DupFunction.cpp
  SplitFunction.cpp
  ArrayObfs.cpp
  Substitution.cpp
  BogusControlFlow.cpp
  Utils.cpp
  SplitBasicBlocks.cpp
  Flattening.cpp
  )

add_dependencies(LLVMObfuscation intrinsics_gen)
