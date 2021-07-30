target_architecture(ARCHITECTURE)
#options for arch
if(ARCHITECTURE STREQUAL "i386" OR ARCHITECTURE STREQUAL "x86_64") 
    SET(X86 1)
    OPTION(X86_MMX "Enable MMX optimizations" ON)
    OPTION(X86_SSE2 "Enable SSE2 optimizations" ON)
    OPTION(X86_SSSE3 "Enable SSSE3 optimizations" OFF)
elseif(ARCHITECTURE STREQUAL "arm")
    SET(ARM 1)
    OPTION(ARM_IWMMXT "Enable IWMMXT compiler intrinsics" OFF)
    OPTION(ARM_NEON "Enable NEON optimizations" OFF)
    OPTION(ARM_SIMD "Enable SIMD optimizations" OFF)
elseif(ARCHITECTURE STREQUAL "ppc" OR ARCHITECTURE STREQUAL "ppc64")
    SET(PPC 1)
    OPTION(PPC_VMX "Enable VMX optimizations" OFF)
elseif(ARCHITECTURE MATCHES "mips*")
    SET(MIPS 1)
    OPTION(MIPS_DSPR2 "Enable DSPR2 optimizations" ON)
    OPTION(MIPS_LOONGSON_MMI  "Enable Loongson Multimedia Instructions" OFF)
endif(ARCHITECTURE STREQUAL "i386" OR ARCHITECTURE STREQUAL "x86_64")