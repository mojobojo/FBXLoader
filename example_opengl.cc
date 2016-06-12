#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <stdlib.h>
#include <stdint.h>

#define FBX_LOADER_IMPLEMENTATION
#include "fbx_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_SUPPORT_ZLIB
#include "stb_image.h"

HDC DeviceContext;
HGLRC GlContext;
int Width, Height;
bool Running;

double *Vertices;
int32_t VertexCount;

int32_t *Indices;
int32_t IndexCount;

double bad_rot;
double Time;
double LastTime;

int64_t CounterStart;
int64_t CounterFrequency;

static double GetTimeInSeconds() {
    int64_t Value;
    QueryPerformanceCounter((LARGE_INTEGER*)&Value);
    return (double)(Value - CounterStart) / (double)CounterFrequency;
}

static void LoadFbx(char *FileName) {
    FILE *f = fopen(FileName, "rb");

    if (f) {
        size_t      FileSize;
        uint8_t    *FileBuffer;

        fseek(f, 0, SEEK_END);
        FileSize = ftell(f);
        rewind(f);
        FileBuffer = (uint8_t *)malloc(FileSize);
        fread(FileBuffer, 1, FileSize, f);
        fclose(f);

        fbx_file LoadedFbx;
        if (fbxl_Parse(FileBuffer, FileSize, &LoadedFbx)) {
            fbx_record *Record = LoadedFbx.Records;
            while (Record) {
                if (strcmp(Record->Name, "Objects") == 0) {
                    Record = Record->SubRecord->SubRecord;

                    while (Record) {
                        if (strcmp(Record->Name, "Vertices") == 0) {
                            if (Record->Properties->Type == FBX_PROPERTY_F64_ARRAY && Record->Properties->Compressed) {
                                uint32_t Size = Record->Properties->Size;
                                uint32_t Count = Record->Properties->Size/8;
                                double *Buffer = (double *)malloc(Size);
                                int32_t value = stbi_zlib_decode_buffer((char *)Buffer, Size, (const char *)Record->Properties->Data, Record->Properties->CompressedSize);
                                if (value == (int32_t)Size) {
                                    Vertices = (double *)Buffer;
                                    VertexCount = Count;
                                }
                                else {
                                    OutputDebugStringA("Decompress failed\n");
                                    DebugBreak();
                                }
                            }
                            else {
                                OutputDebugStringA("Wasnt what I was looking for\n");
                                DebugBreak();
                            }
                        }
                        if (strcmp(Record->Name, "PolygonVertexIndex") == 0) {
                            if (Record->Properties->Type == FBX_PROPERTY_S32_ARRAY && !Record->Properties->Compressed) {
                                uint32_t Size = Record->Properties->Size;
                                Indices = (int32_t *)malloc(Size);
                                CopyMemory(Indices, Record->Properties->Data, Size);
                                IndexCount = Size/4;
                            }
                        }
                        Record = Record->Next;
                    }

                    break;
                }
                Record = Record->Next;
            }

            fbxl_Free(&LoadedFbx);
        }
    }
}

static void Update(double dt) {
    bad_rot += dt*24.0f;
}

static void Render() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, Width, Height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(1.5708, (double)Width/(double)Height, 0.1, 1000.0);
    gluLookAt(0.0, 0.0, 100.0, 
            0.0, 0.0, 0.0, 
            0.0, 1.0, 0.0);

    glMatrixMode(GL_MODELVIEW);

    glRotated(bad_rot, 1.0, 0.0, 0.0);
    glRotated(bad_rot, 0.0, 1.0, 0.0);


    glBegin(GL_QUADS);

    // some colors to see the cube easier
    float colors[8*3] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f,
        1.0f, 1.0f, 1.0f
    };

    for (int32_t i = 0; i < IndexCount; ++i) {
        int32_t index = Indices[i];
        if (index < 0) {
            // New quad
            index = ~index;
        }

        int32_t pos = index*3;
        glColor3fv(&colors[pos]);
        double x = Vertices[pos]*0.5;
        double y = Vertices[pos+1]*0.5;
        double z = Vertices[pos+2]*0.5;
        glVertex3d(x, y, z);
    }

    glEnd();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT LResult = 0;

    switch(uMsg) {
        case WM_CLOSE: {
            Running = false;
            PostQuitMessage(0);
            break;
        }
        case WM_SIZE: {
            Width = LOWORD(lParam);
            Height = HIWORD(lParam);
        }
        case WM_PAINT: {
            break;
        }
        default: {
            LResult = DefWindowProcA(hwnd, uMsg, wParam, lParam);
            break;
        }
    }
    return LResult;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX WindowClass = {};

    WindowClass.cbSize          = sizeof(WNDCLASSEX);
    WindowClass.style           = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc     = WindowProc;
    WindowClass.cbClsExtra      = 0;
    WindowClass.cbWndExtra      = 0;
    WindowClass.hInstance       = GetModuleHandle(NULL);
    WindowClass.hIcon           = (HICON)LoadImage(NULL, IDI_WINLOGO, IMAGE_ICON, SM_CXICON, SM_CYICON, LR_DEFAULTCOLOR | LR_SHARED);
    WindowClass.hCursor         = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, SM_CXCURSOR, SM_CYCURSOR, LR_DEFAULTCOLOR | LR_SHARED);
    WindowClass.hbrBackground   = NULL;
    WindowClass.lpszMenuName    = NULL;
    WindowClass.lpszClassName   = "fbx_loader";
    WindowClass.hIconSm         = NULL;

    if (RegisterClassEx(&WindowClass)) {
        HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "fbx loader example", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, WindowClass.hInstance, 0);

        if (WindowHandle) {
            ShowWindow(WindowHandle, nCmdShow);

            PIXELFORMATDESCRIPTOR PixelFormat = {};

            PixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            PixelFormat.nVersion = 1;
            PixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
            PixelFormat.iPixelType = PFD_TYPE_RGBA;
            PixelFormat.cColorBits = 32;
            PixelFormat.cAlphaBits = 8;
            PixelFormat.iLayerType = PFD_MAIN_PLANE;

            DeviceContext = GetDC(WindowHandle);
            SetPixelFormat(DeviceContext, ChoosePixelFormat(DeviceContext, &PixelFormat), &PixelFormat);

            GlContext = wglCreateContext(DeviceContext);
            if (wglMakeCurrent(DeviceContext, GlContext)) {
                LoadFbx("cube.fbx");
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                QueryPerformanceCounter((LARGE_INTEGER*)&CounterStart);
                QueryPerformanceFrequency((LARGE_INTEGER*)&CounterFrequency);
                Time = GetTimeInSeconds();

                Running = true;
                while (Running) {
                    MSG Message;
                    if (PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&Message);
                        DispatchMessage(&Message);

                        Time = GetTimeInSeconds();
                        double dt = Time-LastTime;
                        LastTime = Time;

                        Update(dt);
                        Render();

                        SwapBuffers(DeviceContext);
                    }
                    else {
                        Running = false;
                    }
                }
            }
        }
    }

    return S_OK;
}

