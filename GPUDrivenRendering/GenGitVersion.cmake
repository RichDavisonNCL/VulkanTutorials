# Runs at build time (via a custom target) to capture the current git commit
# and dirty state, then fills GitVersion.h from GitVersion.h.in.
# SRCDIR = GPUDrivenRendering source dir, DSTDIR = build output dir.

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${SRCDIR}"
    OUTPUT_VARIABLE GIT_COMMIT
    ERROR_QUIET
)
execute_process(
    COMMAND git status --porcelain
    WORKING_DIRECTORY "${SRCDIR}"
    OUTPUT_VARIABLE GIT_STATUS
    ERROR_QUIET
)

string(STRIP "${GIT_COMMIT}" GIT_COMMIT)
string(STRIP "${GIT_STATUS}" GIT_STATUS)

if(GIT_STATUS STREQUAL "")
    set(GIT_DIRTY "false")
else()
    set(GIT_DIRTY "true")
endif()

if(GIT_COMMIT STREQUAL "")
    set(GIT_COMMIT "unknown")
endif()

configure_file("${SRCDIR}/GitVersion.h.in" "${DSTDIR}/GitVersion.h" @ONLY)
