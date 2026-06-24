if(NOT DEFINED BUILD_DIR OR NOT DEFINED RELEASE_TARGET OR
   NOT DEFINED DEBUG_TARGET OR NOT DEFINED PROBE OR NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "BUILD_DIR, RELEASE_TARGET, DEBUG_TARGET, PROBE and WORK_DIR are required")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config Release --target "${RELEASE_TARGET}"
    RESULT_VARIABLE _release_build_result
    OUTPUT_VARIABLE _release_build_stdout
    ERROR_VARIABLE _release_build_stderr
    TIMEOUT 120)
if(NOT _release_build_result EQUAL 0)
    message(FATAL_ERROR "Release library build failed with ${_release_build_result}:\n${_release_build_stdout}\n${_release_build_stderr}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config Debug --target "${DEBUG_TARGET}"
    RESULT_VARIABLE _debug_build_result
    OUTPUT_VARIABLE _debug_build_stdout
    ERROR_VARIABLE _debug_build_stderr
    TIMEOUT 120)
if(NOT _debug_build_result EQUAL 0)
    message(FATAL_ERROR "Debug probe build failed with ${_debug_build_result}:\n${_debug_build_stdout}\n${_debug_build_stderr}")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
file(GLOB _old_logs "${WORK_DIR}/lcu_diagnostics*.log")
if(_old_logs)
    file(REMOVE ${_old_logs})
endif()

execute_process(
    COMMAND "${PROBE}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 5)

if("${_result}" MATCHES "timeout")
    message(FATAL_ERROR "release-static Debug probe timed out, likely blocked by a modal dialog")
endif()
if(NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR "release-static Debug probe exited with ${_result}:\n${_stderr}")
endif()
if(NOT _stderr MATCHES "release static debug client CRT warning")
    message(FATAL_ERROR "stderr did not contain CRT warning:\n${_stderr}")
endif()
if(NOT _stderr MATCHES "Detected memory leaks")
    message(FATAL_ERROR "stderr did not contain leak report:\n${_stderr}")
endif()
if(NOT _stderr MATCHES "diagnostics_release_static_probe[.]c")
    message(FATAL_ERROR "stderr leak report did not contain client source file:\n${_stderr}")
endif()

file(GLOB _logs "${WORK_DIR}/lcu_diagnostics*.log")
list(LENGTH _logs _log_count)
if(NOT _log_count EQUAL 1)
    message(FATAL_ERROR "expected 1 diagnostics log, found ${_log_count}")
endif()

set(_matched_warning FALSE)
set(_matched_leak FALSE)
set(_matched_source FALSE)
foreach(_log ${_logs})
    file(READ "${_log}" _log_content)
    if(_log_content MATCHES "release static debug client CRT warning")
        set(_matched_warning TRUE)
    endif()
    if(_log_content MATCHES "Detected memory leaks")
        set(_matched_leak TRUE)
    endif()
    if(_log_content MATCHES "diagnostics_release_static_probe[.]c")
        set(_matched_source TRUE)
    endif()
endforeach()
if(NOT _matched_warning)
    message(FATAL_ERROR "diagnostics log did not contain CRT warning")
endif()
if(NOT _matched_leak)
    message(FATAL_ERROR "diagnostics log did not contain leak report")
endif()
if(NOT _matched_source)
    message(FATAL_ERROR "diagnostics log leak report did not contain client source file")
endif()
