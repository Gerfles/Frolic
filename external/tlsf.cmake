# tlsf

project(tlsf)

# add_library(tlsf OBJECT)

# file(GLOB TLSF_FILES tlsf/tlsf.c tlsf/tlsf.h)

# target_sources(tlsf PRIVATE ${TLSF_FILES})

# target_include_directories(tlsf SYSTEM PUBLIC tlsf)

#######

set (TLSF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tlsf)

set (TLSF_SOURCES ${TLSF_DIR}/tlsf.h ${TLSF_DIR}/tlsf.c)

add_library(tlsf STATIC ${TLSF_SOURCES})

target_include_directories(tlsf SYSTEM PUBLIC ${TLSF_DIR})
