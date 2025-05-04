enable_testing()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
    ${CMAKE_CURRENT_SOURCE_DIR}/../net
    ${GTEST_INCLUDE_DIRS}
)


file(GLOB TEST_SRC_LIST "test_*.cc")
add_executable(example_test ${TEST_SRC_LIST})

# 链接测试目标
target_link_libraries(example_test
    ZFWTinyMuduo  # 链接主项目库
    rt  # 链接实时库
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
)

# 注册测试
add_test(NAME ExampleTest COMMAND example_test)