#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FBX_LOADER_IMPLEMENTATION
#include "fbx_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_SUPPORT_ZLIB
#include "stb_image.h"

#define internal static

#define THIS_IS_FUCKING_DUMB(x) #x
#define PREPROC_NUM_TO_STRING(x) THIS_IS_FUCKING_DUMB(x)

#define Assert(x) if (!(x)) { fprintf(stderr, "Assert fired in file " __FILE__ " at line " PREPROC_NUM_TO_STRING(__LINE__) " EVAL: " #x "\n"); *(int*)0 = 0; } 

#define mfree free

typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;
typedef i32      b32;

internal char *fbx_PropertyTypeToString(fbx_property_type Type) {
    char *TypeString = "";
    switch (Type) {
        case FBX_PROPERTY_S16: {
            TypeString = "FBX_PROPERTY_S16";
            break;
        }
        case FBX_PROPERTY_BOOL: {
            TypeString = "FBX_PROPERTY_BOOL";
            break;
        }
        case FBX_PROPERTY_BOOL_ARRAY: {
            TypeString = "FBX_PROPERTY_BOOL_ARRAY";
            break;
        }
        case FBX_PROPERTY_S32: {
            TypeString = "FBX_PROPERTY_S32";
            break;
        }
        case FBX_PROPERTY_S32_ARRAY: {
            TypeString = "FBX_PROPERTY_S32_ARRAY";
            break;
        }
        case FBX_PROPERTY_S64: {
            TypeString = "FBX_PROPERTY_S64";
            break;
        }
        case FBX_PROPERTY_S64_ARRAY: {
            TypeString = "FBX_PROPERTY_S64_ARRAY";
            break;
        }
        case FBX_PROPERTY_F32: {
            TypeString = "FBX_PROPERTY_F32";
            break;
        }
        case FBX_PROPERTY_F32_ARRAY: {
            TypeString = "FBX_PROPERTY_F32_ARRAY";
            break;
        }
        case FBX_PROPERTY_F64: {
            TypeString = "FBX_PROPERTY_F64";
            break;
        }
        case FBX_PROPERTY_F64_ARRAY: {
            TypeString = "FBX_PROPERTY_F64_ARRAY";
            break;
        }
        case FBX_PROPERTY_STRING: {
            TypeString = "FBX_PROPERTY_STRING";
            break;
        }
        case FBX_PROPERTY_RAW: {
            TypeString = "FBX_PROPERTY_RAW";
            break;
        }
        default: {
            Assert(!"This shouldnt happen");
        }
    }

    return TypeString;
}

i32 RecordIndent = 0;
internal void
fbx_PrintRecords(fbx_record *Record) {
    ++RecordIndent;
    while (Record) {
        // NOTE(joey): Right now my fbx parser outputs a blank record at the end of each
        // linked list. Im stuck right now on figuring out how to fix that so leaving it
        // for later.
        if (strlen(Record->Name)) {
            char Indent[256] = {};
            i32 Written = 0;
            for (i32 i = 0; i < RecordIndent; ++i) {
                Written = sprintf(Indent+Written, "    ");
            }
            printf("%sName: %s\n", Indent, Record->Name);
            for (u32 i = 0; i < Record->PropertiesCount; ++i) {
                //printf("%s    Data %d: Type %s\n", Indent, i, fbx_PropertyTypeToString(Record->Properties[i].Type));
                //printf("%s        ", Indent);
                fbx_property_type Type = Record->Properties[i].Type;
                b32 Compressed = Record->Properties[i].Compressed;
                u32 Size = Record->Properties[i].Size;
                u32 CompressedSize = Record->Properties[i].CompressedSize;
                void *Data = Record->Properties[i].Data;

                printf("%s    Type: %s\n", Indent, fbx_PropertyTypeToString(Record->Properties[i].Type));
                printf("%s    Compressed: %d\n", Indent, Compressed);
                printf("%s    Size: %d\n", Indent, Size);
                printf("%s    Compressed Size: %d\n", Indent, CompressedSize);
                printf("%s    Data: ", Indent);

                switch (Type) {
                    case FBX_PROPERTY_S16: {
                        printf("%d", *(i16 *)Data);
                        break;
                    }
                    case FBX_PROPERTY_BOOL: {
                        printf("%s", *(u8 *)Data != 0 ? "true" : "false");
                        break;
                    }
                    case FBX_PROPERTY_BOOL_ARRAY: {
                        u32 Count = Size/1;
                        u8 *Buffer = (u8 *)Data;

                        if (Compressed) {
                            Buffer = (u8 *)malloc(Size);
                            i32 value = stbi_zlib_decode_buffer((char *)Buffer, Size, (const char *)Data, CompressedSize);
                            Assert(value == (i32)Size);
                        }

                        for (u32 n = 0; n < Count; ++n) {
                            printf("%s", Buffer[n] != 0 ? "true" : "false");
                        }

                        if (Compressed) {
                            mfree(Buffer);
                        }
                        break;
                    }
                    case FBX_PROPERTY_S32: {
                        printf("%d", *(i32 *)Data);
                        break;
                    }
                    case FBX_PROPERTY_S32_ARRAY: {
                        u32 Count = Size/4;
                        i32 *Buffer = (i32 *)Data;

                        if (Compressed) {
                            Buffer = (i32 *)malloc(Size);
                            i32 value = stbi_zlib_decode_buffer((char *)Buffer, Size, (const char *)Data, CompressedSize);
                            Assert(value == (i32)Size);
                        }

                        for (u32 n = 0; n < Count; ++n) {
                            printf("%d ", Buffer[n]);
                        }

                        if (Compressed) {
                            mfree(Buffer);
                        }
                        break;
                    }
                    case FBX_PROPERTY_S64: {
                        printf("%lld", *(i64 *)Data);
                        break;
                    }
                    case FBX_PROPERTY_S64_ARRAY: {
                        u32 Count = Size/8;
                        i64 *Buffer = (i64 *)Data;

                        if (Compressed) {
                            Buffer = (i64 *)malloc(Size);
                            i32 value = stbi_zlib_decode_buffer((char *)Buffer, Size, (const char *)Data, CompressedSize);
                            Assert(value == (i32)Size);
                        }

                        for (u32 n = 0; n < Count; ++n) {
                            printf("%lld ", Buffer[n]);
                        }

                        if (Compressed) {
                            mfree(Buffer);
                        }
                        break;
                    }
                    case FBX_PROPERTY_F32: {
                        printf("%f", *(f32 *)Data);
                        break;
                    }
                    case FBX_PROPERTY_F32_ARRAY: {
                        u32 Count = Size/4;
                        f32 *Buffer = (f32 *)Data;

                        if (Compressed) {
                            Buffer = (f32 *)malloc(Size);
                            i32 value = stbi_zlib_decode_buffer((char *)Buffer, Size, (const char *)Data, CompressedSize);
                            Assert(value == (i32)Size);
                        }

                        for (u32 n = 0; n < Count; ++n) {
                            printf("%f ", Buffer[n]);
                        }

                        if (Compressed) {
                            mfree(Buffer);
                        }
                        break;
                    }
                    case FBX_PROPERTY_F64: {
                        printf("%f", *(f64 *)Data);
                        break;
                    }
                    case FBX_PROPERTY_F64_ARRAY: {
                        u32 Count = Size/8;
                        f64 *Buffer = (f64 *)Data;

                        if (Compressed) {
                            Buffer = (f64 *)malloc(Size);
                            i32 value = stbi_zlib_decode_buffer((char *)Buffer, Size, (const char *)Data, CompressedSize);
                            Assert(value == (i32)Size);
                        }

                        for (u32 n = 0; n < Count; ++n) {
                            printf("%f ", Buffer[n]);
                        }

                        if (Compressed) {
                            mfree(Buffer);
                        }
                        break;
                    }
                    case FBX_PROPERTY_STRING: {
                        char str[256] = {};
                        memcpy(str, Data, Size);
                        printf("%s", str);
                        break;
                    }
                    case FBX_PROPERTY_RAW: {
                        u8 *Buffer = (u8 *)Data;
                        for (u32 n = 0; n < Size; ++n) {
                            printf("%08X", Buffer[n]);
                        }
                        break;
                    }
                    default: {
                        Assert(!"This shouldnt happen");
                    }
                }

                printf("\n\n");

            }
            if (Record->SubRecord && strlen(Record->SubRecord->Name)) {
                fbx_PrintRecords(Record->SubRecord);
            }
        }
        Record = Record->Next;
    }
    --RecordIndent;
}

int
main(int argc, char **argv) {
    FILE *f = fopen("cube.fbx", "rb");
    u8 *FileBuffer;
    size_t FileSize;

    if (f) {
        fseek(f, 0, SEEK_END);
        FileSize = ftell(f);
        rewind(f);
        FileBuffer = (u8 *)malloc(FileSize);
        fread(FileBuffer, 1, FileSize, f);
        fclose(f);

        fbx_file FbxFile;
        if (fbxl_Parse(FileBuffer, FileSize, &FbxFile)) {
            printf("Parsed file!\n");

            fbx_PrintRecords(FbxFile.Records);

            fbxl_Free(&FbxFile);
        }
    }

    return 0;
}

