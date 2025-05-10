# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../base
    ${CMAKE_CURRENT_SOURCE_DIR}/../net
    ${GTEST_INCLUDE_DIRS}
)


# 使用 GLOB 收集所有测试文件
file(GLOB TEST_SRC_LIST "test_*.cc")
# 添加 main.cc 到源文件列表
list(APPEND TEST_SRC_LIST "main.cc")
# 添加可执行文件
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