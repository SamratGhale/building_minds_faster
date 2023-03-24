#include <windows.h>
#include <dwmapi.h>
#include <xinput.h>
#include <gl/gl.h>
#include "platform_common.h"
#include "platform.h"
#include "intrinsics.h"

//Global variables
global_variable B32 running = 1;
global_variable OffscreenBuffer back_buffer;
global_variable GLuint global_tex_handle;
global_variable GLuint OpenGLDefaultInternalTextureFormat;

//file struffs
typedef size_t GLsizeiptr;
typedef U64 GLintptr;

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);

global_variable wgl_swap_interval_ext* wglSwapInterval;

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hdc, HGLRC hShareContext, const int* attribList);
typedef void   gl_attach_shader  (GLuint program, GLuint shader);
typedef void   gl_compile_shader (GLuint shader);
typedef GLuint gl_create_program (void);
typedef GLuint gl_create_shader  (GLenum type);
typedef void   gl_delete_program (GLuint program);
typedef void   gl_link_program   (GLuint program);
typedef char   GLchar;
typedef void   gl_shader_source    (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void   gl_use_program      (GLuint program);
typedef void   gl_validate_program (GLuint program);
typedef void   gl_get_programiv    (GLuint program, GLenum pname, GLint *params);
typedef void   gl_get_program_info_log (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void   gl_get_shader_info_log(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint  gl_get_uniform_location(GLuint program, const GLchar *name);
typedef void   gl_get_uniform_iv(GLuint program, GLint location, GLint *params);
typedef void   gl_uniform_1i(GLint location, GLint v0);
typedef void   gl_uniform_4f(GLint location, GLfloat v0, GLfloat v1, GLfloat V2_F32, GLfloat v3);
typedef void   gl_uniform_2i(GLint location, GLint v0, GLint v1);
typedef void   gl_uniform_matrix_4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void   gl_bind_vertex_array(GLuint array);
typedef void   gl_gen_vertex_arrays(GLsizei n, GLuint *arrays);
typedef void   gl_gen_buffers(GLsizei n, GLuint *buffers);
typedef void   gl_bind_buffer(GLenum target, GLuint buffer);
typedef void   gl_buffer_data(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void   gl_vertex_attrib_pointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void   gl_enable_vertex_attrib_array(GLuint index);
typedef void   gl_draw_elements(GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void   gl_active_texture(GLenum texture);
typedef void   gl_uniform_2f(GLint location, GLfloat v0, GLfloat v1);
typedef void   gl_buffer_sub_data(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);

global_variable gl_attach_shader *glAttachShader; 
global_variable gl_compile_shader*glCompileShader; 
global_variable gl_create_program*glCreateProgram; 
global_variable gl_create_shader *glCreateShader; 
global_variable gl_delete_program*glDeleteProgram; 
global_variable gl_link_program  *glLinkProgram; 
global_variable gl_shader_source *glShaderSource; 
global_variable gl_use_program   *glUseProgram; 
global_variable gl_validate_program *glValidateProgram;
global_variable gl_get_programiv *glGetProgramiv;
global_variable gl_get_program_info_log* glGetProgramInfoLog;
global_variable gl_get_shader_info_log* glGetShaderInfoLog;
global_variable gl_get_uniform_location* glGetUniformLocation;
global_variable gl_get_uniform_iv* glGetUniformiv;
global_variable gl_uniform_1i* glUniform1i;
global_variable gl_uniform_4f* glUniform4f;
global_variable gl_uniform_2i* glUniform2i;
global_variable gl_uniform_matrix_4fv* glUniformMatrix4fv;
global_variable gl_bind_vertex_array* glBindVertexArray;
global_variable gl_gen_vertex_arrays* glGenVertexArrays ;
global_variable gl_gen_buffers* glGenBuffers;
global_variable gl_bind_buffer* glBindBuffer;
global_variable gl_buffer_data* glBufferData;
global_variable gl_vertex_attrib_pointer* glVertexAttribPointer;
global_variable gl_enable_vertex_attrib_array* glEnableVertexAttribArray;
global_variable gl_active_texture *glActivateTexture;
global_variable gl_uniform_2f* glUniform2f;
global_variable gl_buffer_sub_data* glBufferSubData;

#include "bmf_opengl.h"
global_variable OpenglConfig opengl_config;

#include "bmf_opengl.cpp"
#include "game.cpp"

inline U64 get_file_time(char* file_name){
  FILETIME last_write_time = {};
  WIN32_FILE_ATTRIBUTE_DATA data;
  if(GetFileAttributesExA(file_name, GetFileExInfoStandard, &data)){
    last_write_time = data.ftLastWriteTime;
  }
  U64 ret = (__int64(last_write_time.dwHighDateTime)<<32) | __int64(last_write_time.dwLowDateTime);
  return ret;
}

inline B32 is_file_changed(S64 arg1, S64 arg2){
  if(CompareFileTime((FILETIME*)&arg1, (FILETIME*)&arg2) != 0){
    return 1;
  }
  return 0;
}

//File read and write functions

function ReadFileResult read_entire_file(char* file_name){
  ReadFileResult result = {};
  HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(file_handle != INVALID_HANDLE_VALUE){
    U64 file_size;
    if(GetFileSizeEx(file_handle, (PLARGE_INTEGER)&file_size)){
      result.contents = VirtualAlloc(0, file_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if(result.contents){
        U32 bytes_read;
        if(ReadFile(file_handle, result.contents, file_size, (LPDWORD)&bytes_read, 0)) {
          result.content_size = file_size;
        }else{
          VirtualFree(result.contents, 0, MEM_RELEASE);
          result.contents = 0;
        }
      }
    }
    CloseHandle(file_handle);
  }
  return result;
}

function void process_xinput_button(DWORD button_state, ButtonState* old_state, DWORD button_bit, ButtonState* new_state) {
  new_state->ended_down = ((button_state & button_bit) == button_bit);
  new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}
function F32 process_stick_value(SHORT value, SHORT dead_zone) {
  F32 result = 0;
  if (value < -dead_zone) {
    result = (F32)(value + dead_zone) / (23768.0f - dead_zone);
  } else if (value > dead_zone) {
    result = (F32)(value - dead_zone) / (23767.0f - dead_zone);
  }
  return result;
}

function void display_buffer_in_window(HDC dc, S32 width, S32 height) {
  SwapBuffers(dc);
}

function void process_window_messages() {
  MSG message;
  while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
    switch (message.message) {
      default: {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }break;
    }
  }
}

struct WindowDimention{
  S32 width;
  S32 height;
};

function WindowDimention get_window_dimention(HWND window){
  WindowDimention result;
  RECT client_rect;
  GetClientRect(window, &client_rect);
  result.height = client_rect.bottom - client_rect.top;
  result.width  = client_rect.right - client_rect.left;
  return result;
}

function void resize_buffer(OffscreenBuffer* buffer, int width, int height) {
  if (buffer->memory) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }
  buffer->width = width;
  buffer->height = height;
  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biCompression = BI_RGB;
  buffer->info.bmiHeader.biPlanes   = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biWidth    = buffer->width;
  buffer->info.bmiHeader.biHeight   = buffer->height;
  buffer->bytes_per_pixel = 4;
  buffer->pitch = width * buffer->bytes_per_pixel;
  int BitmapMemorySize = buffer->width * buffer->height * buffer->bytes_per_pixel;
  buffer->memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

function void process_keyboard_message(ButtonState* new_state, B32 is_down) {
  if (new_state->ended_down != is_down) {
    new_state->ended_down = is_down;
    ++new_state->half_transition_count;
  }
}

function void process_pending_messages(ControllerInput* keyboard_controller) {
  MSG message;
  while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
    // Bulletproof
    switch (message.message) {
      case WM_QUIT: {
        running = false;
      }break;
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP: {
        bool was_down = ((message.lParam & (1 << 30)) != 0);
        bool is_down = ((message.lParam & (1 << 31)) == 0);
        U32 vk_code = (U32)message.wParam;
        if (is_down != was_down) {
          if (vk_code == 'W') {
            process_keyboard_message(&keyboard_controller->move_up, is_down);
          } else if (vk_code == 'A') {
            process_keyboard_message(&keyboard_controller->move_left, is_down);
          } else if (vk_code == 'S') {
            process_keyboard_message(&keyboard_controller->move_down, is_down);
          } else if (vk_code == 'D') {
            process_keyboard_message(&keyboard_controller->move_right, is_down);
          } else if (vk_code == 'Q') {
            process_keyboard_message(&keyboard_controller->left_shoulder, is_down);
          } else if (vk_code == 'E') {
            process_keyboard_message(&keyboard_controller->right_shoulder, is_down);
          } else if (vk_code == VK_UP) {
            process_keyboard_message(&keyboard_controller->action_up, is_down);
          } else if (vk_code == VK_DOWN) {
            process_keyboard_message(&keyboard_controller->action_down, is_down);
          } else if (vk_code == VK_LEFT) {
            process_keyboard_message(&keyboard_controller->action_left, is_down);
          } else if (vk_code == VK_RIGHT) {
            process_keyboard_message(&keyboard_controller->action_right, is_down);
          } else if (vk_code == VK_ESCAPE) {
            running = false;
          } else if (vk_code == VK_SPACE) {
            process_keyboard_message(&keyboard_controller->start, is_down);
          } else if (vk_code == VK_BACK) {
            process_keyboard_message(&keyboard_controller->back, is_down);
          }else if(vk_code == 'L'){
            process_keyboard_message(&keyboard_controller->Key_l, is_down);
          }
          if (is_down) {
            B32 alt_key_was_down = ((message.lParam & (1 << 29)) != 0);
            if ((vk_code == VK_F4) && alt_key_was_down) {
              running = false;
            }
          }
        }
      }break;
      default: {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }break;
    }
  }
}

LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;
  switch (message) {
  case WM_PAINT: { //Resize shouldn't be that easily available now that we have pixels in the game
  PAINTSTRUCT Paint;
  HDC hdc = BeginPaint(window, &Paint);
  WindowDimention dim = get_window_dimention(window);
  display_buffer_in_window(hdc, dim.width, dim.height);
  EndPaint(window, &Paint);
}break;
case WM_QUIT: {
  running = false;
}break;
case WM_DESTROY: {
  running = false;
}break;
case WM_COMMAND: {
  if (LOWORD(WParam) == 2) {
    running = false;
  }
}break;
case WM_SETCURSOR:{
    //HCURSOR curr = CreateCursor( NULL, 0, 0, 50, 20, (const void*)"SamratGhale", NULL );

    HCURSOR curr = LoadCursor(NULL, IDC_CROSS);             // cursor handle
    SetCursor(curr);
  };
  default: {
    Result = DefWindowProc(window, message, WParam, LParam);
  }
  break;
}
return Result;
}

function void win32_init_opengl(HWND window){
  HDC window_dc = GetDC(window);

  PIXELFORMATDESCRIPTOR desired_pixel_format = {};
  desired_pixel_format.nSize      = sizeof(desired_pixel_format);
  desired_pixel_format.nVersion   = 1;
  desired_pixel_format.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
  desired_pixel_format.cColorBits = 32;
  desired_pixel_format.cAlphaBits = 8;
  desired_pixel_format.iLayerType = PFD_MAIN_PLANE;

  S32 suggested_pixel_format_index = ChoosePixelFormat(window_dc, &desired_pixel_format);
  PIXELFORMATDESCRIPTOR suggested_pixel_format;
  DescribePixelFormat(window_dc, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format);
  SetPixelFormat(window_dc, suggested_pixel_format_index, &suggested_pixel_format);
  HGLRC opengl_rc = wglCreateContext(window_dc);
  if(wglMakeCurrent(window_dc, opengl_rc)){

    B32 modern_context = false;
    wgl_create_context_attribs_arb* wglCreateContextAttribsARB = (wgl_create_context_attribs_arb*)wglGetProcAddress("wglCreateContextAttribsARB");

    glAttachShader             = (gl_attach_shader *)wglGetProcAddress("glAttachShader"); 
    glCompileShader            = (gl_compile_shader*)wglGetProcAddress("glCompileShader"); 
    glCreateProgram            = (gl_create_program*)wglGetProcAddress("glCreateProgram"); 
    glCreateShader             = (gl_create_shader *)wglGetProcAddress("glCreateShader"); 
    glDeleteProgram            = (gl_delete_program*)wglGetProcAddress("glDeleteProgram"); 
    glLinkProgram              = (gl_link_program  *)wglGetProcAddress("glLinkProgram");
    glShaderSource             = (gl_shader_source *)wglGetProcAddress("glShaderSource"); 
    glUseProgram               = (gl_use_program   *)wglGetProcAddress("glUseProgram");    
    glValidateProgram          = (gl_validate_program*)wglGetProcAddress("glValidateProgram");
    glGetProgramiv             = (gl_get_programiv *)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog        = (gl_get_program_info_log*)wglGetProcAddress("glGetProgramInfoLog");
    glGetShaderInfoLog         = (gl_get_shader_info_log*)wglGetProcAddress("glGetShaderInfoLog");
    glGetUniformLocation       = (gl_get_uniform_location*)wglGetProcAddress("glGetUniformLocation");
    glGetUniformiv             = (gl_get_uniform_iv*)wglGetProcAddress("glGetUniformiv");
    glUniform1i                = (gl_uniform_1i*)wglGetProcAddress("glUniform1i");
    glUniform4f                = (gl_uniform_4f*)wglGetProcAddress("glUniform4f");
    glUniform2i                = (gl_uniform_2i*)wglGetProcAddress("glUniform2i");
    glUniformMatrix4fv         = (gl_uniform_matrix_4fv*)wglGetProcAddress("glUniformMatrix4fv");
    glBindVertexArray          = (gl_bind_vertex_array*)wglGetProcAddress("glBindVertexArray");
    glGenVertexArrays          = (gl_gen_vertex_arrays*)wglGetProcAddress("glGenVertexArrays");
    glGenBuffers               = (gl_gen_buffers*) wglGetProcAddress("glGenBuffers");
    glBindBuffer               = (gl_bind_buffer*) wglGetProcAddress("glBindBuffer");
    glBufferData              = (gl_buffer_data*)                wglGetProcAddress("glBufferData");
    glVertexAttribPointer     = (gl_vertex_attrib_pointer*)      wglGetProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArray = (gl_enable_vertex_attrib_array*) wglGetProcAddress("glEnableVertexAttribArray");
    glActivateTexture = (gl_active_texture*)wglGetProcAddress("glActiveTexture");

    glUniform2f = (gl_uniform_2f*) wglGetProcAddress("glUniform2f");
    glBufferSubData = (gl_buffer_sub_data*)wglGetProcAddress("glBufferSubData");
    //glGetProgramInfoLog = (gl_get_program_info_log*)wglGetProcAddress("glGetProgramInfoLog");

    if(wglCreateContextAttribsARB){
      int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 0,
        WGL_CONTEXT_FLAGS_ARB, 0 | WGL_CONTEXT_DEBUG_BIT_ARB, // Note:enable for debugging
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0,};

        HGLRC share_context = 0;
        HGLRC modern_glrc = wglCreateContextAttribsARB(window_dc, share_context, attribs);
        if(modern_glrc){
          if(wglMakeCurrent(window_dc, modern_glrc)){
            modern_context = true;
            wglDeleteContext(opengl_rc);
            opengl_rc = modern_glrc;
          }
        }
      }else{
      //
      }
      opengl_init(modern_context);
      wglSwapInterval = (wgl_swap_interval_ext*)wglGetProcAddress("wglSwapIntervalEXT");
      if(wglSwapInterval){
        wglSwapInterval(1);
      }
    }else{
      assert(0);
    }
    ReleaseDC(window, window_dc);
  }

  int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code) {
    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "Practice";
    window_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    RegisterClassA(&window_class);

    HWND window = CreateWindowExA(0,window_class.lpszClassName,"Playing around", WS_OVERLAPPED | WS_VISIBLE , CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080,  0, 0, instance, 0);

  //Remove maximize window
  //SetWindowLong(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE) & ~WS_MAXIMIZEBOX); 

  //Window init finish
    RECT rect;
    GetWindowRect(window, &rect);
    S32 width  = rect.right  - rect.left;
    S32 height = rect.bottom - rect.top;
    resize_buffer(&back_buffer, width, height);
  //Game State initilize
    GameInput input[2] = {};
    GameInput* old_input = &input[0];
    GameInput* new_input = &input[1];

  //Allocation of memory for the arena

    PlatformState platform_state = {};
    platform_state.permanent_storage_size = Megabytes(256);
    platform_state.temporary_storage_size = Gigabytes(1);
    platform_state.total_size = platform_state.permanent_storage_size + platform_state.temporary_storage_size;
    platform_state.permanent_storage = VirtualAlloc(0, (size_t)platform_state.total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    platform_state.temporary_storage = (U8*)platform_state.permanent_storage + platform_state.permanent_storage_size;

    win32_init_opengl(window);
    while (running) {
      HDC hdc = GetDC(window);
      ControllerInput* new_keyboard_controller = &new_input->controllers[0];
      ControllerInput* old_keyboard_controller = &old_input->controllers[0];
      *new_keyboard_controller = {};
      new_keyboard_controller->is_connected = true;
      new_input->dtForFrame = 0.03333f;

      for (U32 i = 0; i < array_count(new_keyboard_controller->buttons); i++) {
        new_keyboard_controller->buttons[i].ended_down = old_keyboard_controller->buttons[i].ended_down;
      }
      process_pending_messages(new_keyboard_controller);


      POINT mouse_p;
      GetCursorPos(&mouse_p);
      ScreenToClient(window, &mouse_p);
      new_input->mouse_x = mouse_p.x;
      new_input->mouse_y = mouse_p.y;
      new_input->mouse_z = 0;

      process_keyboard_message(&new_input->mouse_buttons[0], GetKeyState(VK_LBUTTON) & 1 << 15);
      process_keyboard_message(&new_input->mouse_buttons[1], GetKeyState(VK_MBUTTON) & 1 << 15);
      process_keyboard_message(&new_input->mouse_buttons[2], GetKeyState(VK_RBUTTON) & 1 << 15);
      process_keyboard_message(&new_input->mouse_buttons[3], GetKeyState(VK_XBUTTON1) & 1 << 15);
      process_keyboard_message(&new_input->mouse_buttons[4], GetKeyState(VK_XBUTTON2) & 1 << 15);

      XINPUT_STATE controller_state;
      DWORD max_controller_count = XUSER_MAX_COUNT;

      for (DWORD controller_index = 0; controller_index < max_controller_count; ++controller_index) {
        DWORD our_controller_index = controller_index + 1;
        ControllerInput* old_controller = &old_input->controllers[our_controller_index];
        ControllerInput* new_controller = &new_input->controllers[our_controller_index];

        if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
          XINPUT_GAMEPAD* Pad = &controller_state.Gamepad;
          new_controller->is_connected = true;
          new_controller->is_analog = old_controller->is_analog;

          process_xinput_button(Pad->wButtons, &old_controller->action_down, XINPUT_GAMEPAD_A, &new_controller->action_down);
          process_xinput_button(Pad->wButtons, &old_controller->action_right, XINPUT_GAMEPAD_B, &new_controller->action_right);
          process_xinput_button(Pad->wButtons, &old_controller->action_left, XINPUT_GAMEPAD_X, &new_controller->action_left);
          process_xinput_button(Pad->wButtons, &old_controller->action_up, XINPUT_GAMEPAD_Y, &new_controller->action_up);
          process_xinput_button(Pad->wButtons, &old_controller->left_shoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &new_controller->left_shoulder);
          process_xinput_button(Pad->wButtons, &old_controller->right_shoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &new_controller->right_shoulder);
          process_xinput_button(Pad->wButtons, &old_controller->start, XINPUT_GAMEPAD_START, &new_controller->start);
          process_xinput_button(Pad->wButtons, &old_controller->back, XINPUT_GAMEPAD_BACK, &new_controller->back);
          new_controller->stick_x = process_stick_value(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
          new_controller->stick_y = process_stick_value(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

          if ((new_controller->stick_x != 0.0f) || (new_controller->stick_y != 0.0f)) {
            new_controller->is_analog = true;
          }
          if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
            new_controller->stick_y = 1.0f;
            new_controller->is_analog = false;
          } else if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
            new_controller->stick_y = -1.0f;
            new_controller->is_analog = false;
          } if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
            new_controller->stick_x = -1.0f;
            new_controller->is_analog = false;
          } else if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
            new_controller->stick_x = 1.0f;
            new_controller->is_analog = false;
          }

          F32 Threshold = 0.5f;
          process_xinput_button((new_controller->stick_x > Threshold) ? 1 : 0, &new_controller->move_right, 1, &old_controller->move_right);
          process_xinput_button((new_controller->stick_x < -Threshold) ? 1 : 0, &new_controller->move_left, 1, &old_controller->move_left);
          process_xinput_button((new_controller->stick_y > Threshold) ? 1 : 0, &new_controller->move_up, 1, &old_controller->move_up);
          process_xinput_button((new_controller->stick_y < -Threshold) ? 1 : 0, &new_controller->move_down, 1, &old_controller->move_down);
        }else{
          new_controller->is_connected = false;
        // NOTE: Controller unavailable
        }
      };
      render_game(&back_buffer, &platform_state, new_input);
      WindowDimention dim = get_window_dimention(window);
      display_buffer_in_window(hdc, dim.width, dim.height);
      ReleaseDC(window, hdc);
      GameInput* temp = old_input;
      old_input = new_input;
      new_input = temp;
      DwmFlush();
    }
  //glUseProgram(0);
    return 0;
  }
