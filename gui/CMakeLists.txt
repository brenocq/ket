include(FetchContent)

find_package(OpenGL REQUIRED)

# --- Pinned third-party UI dependencies ------------------------------------
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4)
set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(GLFW_INSTALL
    OFF
    CACHE BOOL "" FORCE)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.8-docking)
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v1.0)
FetchContent_Declare(
  implot3d
  GIT_REPOSITORY https://github.com/brenocq/implot3d.git
  GIT_TAG v0.4)

# glfw ships CMake (add_subdirectory); the others are source-only and are just
# populated, then compiled into static libraries below.
FetchContent_MakeAvailable(glfw imgui implot implot3d)

# Dear ImGui: core + GLFW/OpenGL3 backends. Headers are marked SYSTEM so their
# warnings do not leak into our targets.
add_library(
  imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp)
target_include_directories(
  imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends
                      ${imgui_SOURCE_DIR}/misc/cpp)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# ImPlot (2D) and ImPlot3D, both built against ImGui.
add_library(
  implot STATIC
  ${implot_SOURCE_DIR}/implot.cpp ${implot_SOURCE_DIR}/implot_items.cpp
  ${implot_SOURCE_DIR}/implot_demo.cpp)
target_include_directories(implot SYSTEM PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

add_library(
  implot3d STATIC
  ${implot3d_SOURCE_DIR}/implot3d.cpp ${implot3d_SOURCE_DIR}/implot3d_items.cpp
  ${implot3d_SOURCE_DIR}/implot3d_demo.cpp
  ${implot3d_SOURCE_DIR}/implot3d_meshes.cpp)
target_include_directories(implot3d SYSTEM PUBLIC ${implot3d_SOURCE_DIR})
target_link_libraries(implot3d PUBLIC imgui)

# --- The ket-gui executable ------------------------------------------------
# The UI lives in a small static library (mirrors cli/), so the executable
# target stays a thin entry point.
add_library(ket_gui_core STATIC app.cpp)
target_link_libraries(ket_gui_core PUBLIC ket imgui implot implot3d)
target_include_directories(ket_gui_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Absolute paths to the font assets, resolved at runtime: the in-tree copy for
# dev builds, and the installed copy (share/ket) for an installed binary.
target_compile_definitions(
  ket_gui_core
  PRIVATE KET_GUI_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
          KET_GUI_INSTALL_ASSETS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/ket")

add_executable(ket_gui main.cpp)
set_target_properties(ket_gui PROPERTIES OUTPUT_NAME ket-gui)
target_link_libraries(ket_gui PRIVATE ket_gui_core)

install(TARGETS ket_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/Inter-Regular.ttf
              ${CMAKE_CURRENT_SOURCE_DIR}/assets/Inter-Bold.ttf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/ket)
