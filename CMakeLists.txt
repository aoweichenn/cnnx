# 指定CMake最低版本要求
cmake_minimum_required(VERSION 3.20)

# 定义项目基本信息：项目名称为cnnx，版本1.0.0，使用C++语言
project(cnnx VERSION 1.0.0 LANGUAGES CXX)
# 添加预定宏
add_compile_definitions(GLOG_USE_GLOG_EXPORT)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 要求必须满足指定的C++标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建输出目录：
# 静态库文件(.a)输出到项目根目录/library/static目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library/static)
# 共享库文件(.so/.dll)输出到项目根目录/library/shared目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library/shared)
# 可执行文件输出到项目根目录/library/runtime目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/library/runtime)

# 添加头文件包含路径：
# 包含第三方库的头文件目录
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)
# 包含项目自身的头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 构建静态库配置：
# 创建名为cnnx_static的静态库，源文件为main.cpp
add_library(cnnx_static STATIC main.cpp
        include/ir/attribute.hpp
        include/ir/operand.hpp
        include/ir/operator.hpp
        include/ir/parameter.hpp
        include/utils/fp16_converter.hpp
        include/utils/storezip.hpp
        source/ir/attribute.cpp
        source/ir/operator.cpp
        source/ir/operand.cpp
        source/ir/graph.cpp)
# 为静态库链接依赖项：
target_link_libraries(cnnx_static PRIVATE
        pthread  # POSIX线程库
        ${PROJECT_SOURCE_DIR}/third_party/library/glog/libglog.a  # Google日志库
        ${PROJECT_SOURCE_DIR}/third_party/library/gflags/libgflags.a  # 命令行参数解析库
        ${PROJECT_SOURCE_DIR}/third_party/library/openblas/libopenblas.a  # 优化过的BLAS数学库
        ${PROJECT_SOURCE_DIR}/third_party/library/gtest/libgtest.a  # Google测试框架
        ${PROJECT_SOURCE_DIR}/third_party/library/gtest/libgtest_main.a  # GTest的main函数入口
)
# 显式设置静态库输出名称（虽然与目标名相同，但显式声明更明确）
set_target_properties(cnnx_static PROPERTIES OUTPUT_NAME "cnnx_static")

# 构建可执行文件配置：
# 创建名为cnnx的可执行文件，源文件为main.cpp
add_executable(cnnx main.cpp
        include/ir/attribute.hpp
        include/ir/graph.hpp
        include/ir/operand.hpp
        include/ir/operator.hpp
        include/ir/parameter.hpp
        include/utils/fp16_converter.hpp
        include/utils/storezip.hpp
        source/ir/attribute.cpp
        source/ir/operator.cpp
        source/ir/operand.cpp
        source/ir/graph.cpp)
# 为可执行文件链接依赖项（与静态库相同）
target_link_libraries(cnnx PRIVATE
        pthread
        ${PROJECT_SOURCE_DIR}/third_party/library/glog/libglog.a
        ${PROJECT_SOURCE_DIR}/third_party/library/gflags/libgflags.a
        ${PROJECT_SOURCE_DIR}/third_party/library/openblas/libopenblas.a
        ${PROJECT_SOURCE_DIR}/third_party/library/gtest/libgtest.a
        ${PROJECT_SOURCE_DIR}/third_party/library/gtest/libgtest_main.a
)
