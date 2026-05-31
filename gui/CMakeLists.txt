include(FetchContent)

# Native builds use a fetched GLFW + the system OpenGL. Emscripten instead uses
# its own GLFW port and WebGL/GLES (no fetch, no find_package).
if(NOT EMSCRIPTEN)
  find_package(OpenGL REQUIRED)

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
endif()

# --- Pinned third-party UI dependencies ------------------------------------
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
if(EMSCRIPTEN)
  FetchContent_MakeAvailable(imgui implot implot3d)
else()
  FetchContent_MakeAvailable(glfw imgui implot implot3d)
endif()

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
if(EMSCRIPTEN)
  # Activate Emscripten's GLFW port so <GLFW/glfw3.h> is available when compiling
  # the backends (and, propagated PUBLIC, app.cpp). WebGL/GLES come from sysroot.
  target_compile_options(imgui PUBLIC "-sUSE_GLFW=3")
else()
  target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
endif()

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
# Where the fonts are found at runtime. Native: the in-tree copy (dev) or the
# installed copy (share/ket). Emscripten: the preloaded virtual FS at /assets.
if(EMSCRIPTEN)
  target_compile_definitions(
    ket_gui_core PRIVATE KET_GUI_ASSETS_DIR="/assets"
                         KET_GUI_EXAMPLES_DIR="/examples")
else()
  target_compile_definitions(
    ket_gui_core
    PRIVATE
      KET_GUI_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
      KET_GUI_INSTALL_ASSETS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/ket"
      KET_GUI_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/examples"
      KET_GUI_INSTALL_EXAMPLES_DIR="${CMAKE_INSTALL_FULL_DATADIR}/ket/examples")
endif()

add_executable(ket_gui main.cpp)
set_target_properties(ket_gui PROPERTIES OUTPUT_NAME ket-gui)
target_link_libraries(ket_gui PRIVATE ket_gui_core)

if(EMSCRIPTEN)
  # Emit ket-gui.js (+ .wasm + .data) loaded by our own demo/index.html shell;
  # bundle the fonts and example circuits into the virtual FS so the existing
  # file lookups and the File dialog work in-browser.
  set_target_properties(ket_gui PROPERTIES SUFFIX ".js")
  target_link_options(
    ket_gui
    PRIVATE
    -sUSE_GLFW=3
    -sFULL_ES3
    -sMAX_WEBGL_VERSION=2
    -sALLOW_MEMORY_GROWTH=1
    "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
    "SHELL:--preload-file ${PROJECT_SOURCE_DIR}/examples@/examples")
else()
  install(TARGETS ket_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/Inter-Regular.ttf
          ${CMAKE_CURRENT_SOURCE_DIR}/assets/Inter-Bold.ttf
          ${CMAKE_CURRENT_SOURCE_DIR}/assets/Inter-OFL.txt
          ${CMAKE_CURRENT_SOURCE_DIR}/assets/JetBrainsMono-Regular.ttf
          ${CMAKE_CURRENT_SOURCE_DIR}/assets/JetBrainsMono-OFL.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ket)
  # The example circuits power the in-app Examples menu on an installed binary.
  file(GLOB _ket_example_qasm "${PROJECT_SOURCE_DIR}/examples/*.qasm")
  install(FILES ${_ket_example_qasm}
          DESTINATION ${CMAKE_INSTALL_DATADIR}/ket/examples)
endif()
