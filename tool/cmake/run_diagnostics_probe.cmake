if(NOT DEFINED PROBE OR NOT DEFINED MODE OR NOT DEFINED WORK_DIR OR
   NOT DEFINED EXPECT_EXIT)
    message(FATAL_ERROR "PROBE, MODE, WORK_DIR and EXPECT_EXIT are required")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
file(GLOB _old_logs "${WORK_DIR}/lcu_diagnostics*.log")
if(_old_logs)
    file(REMOVE ${_old_logs})
endif()

execute_process(
    COMMAND "${PROBE}" "${MODE}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
    TIMEOUT 5)

if("${_result}" MATCHES "timeout")
    message(FATAL_ERROR "probe timed out, likely blocked by a modal dialog")
endif()

if(EXPECT_EXIT STREQUAL "zero" AND NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR "probe exited with ${_result}:\n${_stderr}")
endif()

if(EXPECT_EXIT STREQUAL "nonzero" AND "${_result}" STREQUAL "0")
    message(FATAL_ERROR "probe unexpectedly exited successfully")
endif()

if(DEFINED EXPECT_STDERR AND NOT _stderr MATCHES "${EXPECT_STDERR}")
    message(FATAL_ERROR "stderr did not contain '${EXPECT_STDERR}':\n${_stderr}")
endif()

if(DEFINED EXPECT_STDOUT AND NOT _stdout MATCHES "${EXPECT_STDOUT}")
    message(FATAL_ERROR "stdout did not contain '${EXPECT_STDOUT}':\n${_stdout}")
endif()

if(DEFINED EXPECT_LOG_COUNT)
    file(GLOB _logs "${WORK_DIR}/lcu_diagnostics*.log")
    list(LENGTH _logs _log_count)
    if(NOT _log_count EQUAL EXPECT_LOG_COUNT)
        message(FATAL_ERROR "expected ${EXPECT_LOG_COUNT} diagnostics logs, found ${_log_count}")
    endif()

    if(DEFINED EXPECT_LOG)
        set(_matched_log FALSE)
        foreach(_log ${_logs})
            file(READ "${_log}" _log_content)
            if(_log_content MATCHES "${EXPECT_LOG}")
                set(_matched_log TRUE)
            endif()
        endforeach()
        if(NOT _matched_log)
            message(FATAL_ERROR "no diagnostics log contained '${EXPECT_LOG}'")
        endif()
    endif()
endif()
