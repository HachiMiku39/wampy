#define IMGUI_IMPL_OPENGL_ES2 true
// Song names may contain supplementary CJK characters such as 𠮷 (U+20BB7).
#define IMGUI_USE_WCHAR32
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
// int breaks rendering on device
// GL_OES_element_index_uint is not supported
// https://registry.khronos.org/OpenGL-Refpages/es2.0/xhtml/glDrawElements.xml
#define ImDrawIdx unsigned short
