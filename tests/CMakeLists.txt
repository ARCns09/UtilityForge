function(utilityforge_add_core_test test_name source_file)
    add_executable("${test_name}" "${source_file}")
    target_link_libraries("${test_name}" PRIVATE utilityforge_core Qt6::Test)
    utilityforge_enable_warnings("${test_name}")
    add_test(NAME "${test_name}" COMMAND "${test_name}")
endfunction()

utilityforge_add_core_test(tst_AppError core/tst_AppError.cpp)
utilityforge_add_core_test(tst_AppPaths core/tst_AppPaths.cpp)
utilityforge_add_core_test(tst_SettingsService core/tst_SettingsService.cpp)
utilityforge_add_core_test(tst_CancellationToken core/tst_CancellationToken.cpp)
utilityforge_add_core_test(tst_TaskManager core/tst_TaskManager.cpp)

add_executable(utilityforge_fake_process fake-tools/FakeProcess.cpp)
target_link_libraries(utilityforge_fake_process PRIVATE Qt6::Core)
utilityforge_enable_warnings(utilityforge_fake_process)

utilityforge_add_core_test(tst_ProcessRunner core/tst_ProcessRunner.cpp)
target_compile_definitions(
    tst_ProcessRunner
    PRIVATE UTILITYFORGE_FAKE_PROCESS_PATH="$<TARGET_FILE:utilityforge_fake_process>"
)
add_dependencies(tst_ProcessRunner utilityforge_fake_process)

utilityforge_add_core_test(tst_ExternalToolRegistry core/tst_ExternalToolRegistry.cpp)
target_compile_definitions(
    tst_ExternalToolRegistry
    PRIVATE UTILITYFORGE_FAKE_PROCESS_PATH="$<TARGET_FILE:utilityforge_fake_process>"
)
add_dependencies(tst_ExternalToolRegistry utilityforge_fake_process)

function(utilityforge_add_widget_test test_name source_file)
    add_executable("${test_name}" "${source_file}")
    target_link_libraries(
        "${test_name}"
        PRIVATE utilityforge_app utilityforge_dropzone utilityforge_core Qt6::Test Qt6::Widgets
    )
    utilityforge_enable_warnings("${test_name}")
    add_test(NAME "${test_name}" COMMAND "${test_name}")
    set_tests_properties("${test_name}" PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

utilityforge_add_widget_test(tst_DropZonePage dropzone/tst_DropZonePage.cpp)
utilityforge_add_widget_test(tst_MainWindow app/tst_MainWindow.cpp)
