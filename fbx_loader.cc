#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_SUPPORT_ZLIB
#include "stb_image.h"

#define internal static
#define local_persist static
#define global_variable static

#define DEBUG

#define THIS_IS_FUCKING_DUMB(x) #x
#define PREPROC_NUM_TO_STRING(x) THIS_IS_FUCKING_DUMB(x)

#ifdef DEBUG
#ifdef _WIN32
#define Assert(x) if (!(x)) { fprintf(stderr, "Assert fired in file " __FILE__ " at line " PREPROC_NUM_TO_STRING(__LINE__) " EVAL: " #x "\n"); *(int*)0 = 0; } 
#else
#define Assert(x) if (!(x)) { *(int*)0 = 0; } 
#endif
#else
#define Assert(x)
#endif

#define Min(A, B) (A < B ? A : B)
#define Max(A, B) (A > B ? A : B)
#define Square(A) ((A)*(A))

#define ArrayCount(x) (sizeof(x) / sizeof(x[0]))

#define Kilobytes(Value) (Value * 1024ULL)
#define Megabytes(Value) (Kilobytes(Value) * 1024ULL)
#define Gigabytes(Value) (Megabytes(Value) * 1024ULL)
#define Terabytes(Value) (Gigabytes(Value) * 1024ULL)

#define PI32 3.1415926f
#define PI64 3.1415926535897932

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


const char fbx_Magic[] = "Kaydara FBX Binary  ";

enum fbx_property_type {
    FBX_PROPERTY_S16,
    FBX_PROPERTY_BOOL,
    FBX_PROPERTY_BOOL_ARRAY,
    FBX_PROPERTY_S32,
    FBX_PROPERTY_S32_ARRAY,
    FBX_PROPERTY_S64,
    FBX_PROPERTY_S64_ARRAY,
    FBX_PROPERTY_F32,
    FBX_PROPERTY_F32_ARRAY,
    FBX_PROPERTY_F64,
    FBX_PROPERTY_F64_ARRAY,
    FBX_PROPERTY_STRING,
    FBX_PROPERTY_RAW
};

struct fbx_property {
    fbx_property_type  Type;
    b32                Compressed;
    u32                Size;
    b32                CompressedSize;
    void              *Data;
};

struct fbx_record {
    char          Name[64];
    u32           PropertiesCount;
    fbx_property *Properties;
    fbx_record   *SubRecord;
    fbx_record   *Next;
};

struct fbx_file {
    u32         Version;
    fbx_record *Records;
};

u8 *FileBuffer;
size_t FileSize;

internal u32
fbx_ReadRecord(u8 *FbxData, u32 Start, fbx_record *Record) {

    u8 *s = FbxData+Start;

    u32 EndOffset = *(u32 *)s;

    if (EndOffset) {
        u32 NumProperties = *(u32 *)(s+4);
        //u32 PropertyListLength = *(u32 *)(s+8);
        u8 NameLen = *(s+12);
        // TODO(joey): Figure out if names are a finite size
        Record->Name[64] = {};
        memcpy(Record->Name, s+13, Min(NameLen, 63));

        Record->PropertiesCount = NumProperties;
        Record->Properties = (fbx_property *)malloc(NumProperties*sizeof(fbx_property));
        memset(Record->Properties, 0, NumProperties*sizeof(fbx_property));

        s += 13+NameLen;
        for (u32 i = 0; i < NumProperties; ++i) {
            fbx_property *p = &Record->Properties[i];
            u8 Type = *s++;
            switch (Type) {
                case 'Y':{
                    p->Type = FBX_PROPERTY_S16;
                    p->Size = 2;
                    p->Data = malloc(2);
                    *(i16 *)p->Data = *(i16 *)s;
                    s += 2;
                    break;
                }
                case 'C':{
                    p->Type = FBX_PROPERTY_BOOL;
                    p->Size = 1;
                    p->Data = malloc(1);
                    *(i8 *)p->Data = *(i8 *)s;
                    ++s;
                    break;
                }
                case 'I':{
                    p->Type = FBX_PROPERTY_S32;
                    p->Size = 4;
                    p->Data = malloc(4);
                    *(i32 *)p->Data = *(i32 *)s;
                    s += 4;
                    break;
                }
                case 'F':{
                    p->Type = FBX_PROPERTY_F32;
                    p->Size = 4;
                    p->Data = malloc(4);
                    *(f32 *)p->Data = *(f32 *)s;
                    s += 4;
                    break;
                }
                case 'D':{
                    p->Type = FBX_PROPERTY_F64;
                    p->Size = 8;
                    p->Data = malloc(8);
                    *(f64 *)p->Data = *(f64 *)s;
                    s += 8;
                    break;
                }
                case 'L':{
                    p->Type = FBX_PROPERTY_S64;
                    p->Size = 8;
                    p->Data = malloc(8);
                    *(i64 *)p->Data = *(i64 *)s;
                    s += 8;
                    break;
                }
                case 'f':{
                    u32 ArrayLen = *(u32 *)s;
                    u32 Encoding = *(u32 *)(s+4);
                    u32 CompressedLen = *(u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_F32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = malloc(CompressedLen);
                        memcpy(p->Data, s+12, CompressedLen);
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_F32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->Data = malloc(ArrayLen*4);
                        memcpy(p->Data, s+12, ArrayLen*4);
                        s += 12+(ArrayLen*4);
                    }
                    break;
                }
                case 'd':{
                    u32 ArrayLen = *(u32 *)s;
                    u32 Encoding = *(u32 *)(s+4);
                    u32 CompressedLen = *(u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_F64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = malloc(CompressedLen);
                        memcpy(p->Data, s+12, CompressedLen);
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_F64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->Data = malloc(ArrayLen*8);
                        memcpy(p->Data, s+12, ArrayLen*8);
                        s += 12+(ArrayLen*8);
                    }
                    break;
                }
                case 'i':{
                    u32 ArrayLen = *(u32 *)s;
                    u32 Encoding = *(u32 *)(s+4);
                    u32 CompressedLen = *(u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_S32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = malloc(CompressedLen);
                        memcpy(p->Data, s+12, CompressedLen);
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_S32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->Data = malloc(ArrayLen*4);
                        memcpy(p->Data, s+12, ArrayLen*4);
                        s += 12+(ArrayLen*4);
                    }
                    break;
                }

                case 'l':{
                    u32 ArrayLen = *(u32 *)s;
                    u32 Encoding = *(u32 *)(s+4);
                    u32 CompressedLen = *(u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_S64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = malloc(CompressedLen);
                        memcpy(p->Data, s+12, CompressedLen);
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_S64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->Data = malloc(ArrayLen*8);
                        memcpy(p->Data, s+12, ArrayLen*8);
                        s += 12+(ArrayLen*8);
                    }
                    break;
                }
                case 'b':{
                    u32 ArrayLen = *(u32 *)s;
                    u32 Encoding = *(u32 *)(s+4);
                    u32 CompressedLen = *(u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_BOOL_ARRAY;
                        p->Size = ArrayLen*1;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = malloc(CompressedLen);
                        memcpy(p->Data, s+12, CompressedLen);
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_BOOL_ARRAY;
                        p->Size = ArrayLen*1;
                        p->Data = malloc(ArrayLen*1);
                        memcpy(p->Data, s+12, ArrayLen*1);
                        s += 12+(ArrayLen*1);
                    }
                    break;
                }
                case 'S':{
                    u32 Len = *(u32 *)s;
                    p->Type = FBX_PROPERTY_STRING;
                    p->Size = Len;
                    p->Data = malloc(Len);
                    memcpy(p->Data, s+4, Len);
                    s += 4+Len;
                    break;
                }
                case 'R': {
                    u32 Len = *(u32 *)s;
                    p->Type = FBX_PROPERTY_RAW;
                    p->Size = Len;
                    p->Data = malloc(Len);
                    memcpy(p->Data, s+4, Len);
                    s += 4+Len;
                    break;
                }
                default: {
                    Assert(!"Unknown type");
                }
            }
        }

        u64 NextOffset = s-FbxData;
        Assert((NextOffset&0xFFFFFFFF00000000) == 0);

        u32 Next = (u32)NextOffset;
        Record->SubRecord = (fbx_record *)malloc(sizeof(fbx_record));
        memset(Record->SubRecord, 0, sizeof(fbx_record));
        fbx_record *CurrentRecord = Record->SubRecord;
        while (Next < EndOffset) {
            Next = fbx_ReadRecord(FbxData, Next, CurrentRecord);
            if (!Next) {
                break;
            }
            CurrentRecord->Next = (fbx_record *)malloc(sizeof(fbx_record));
            memset(CurrentRecord->Next, 0, sizeof(fbx_record));
            CurrentRecord = CurrentRecord->Next;
        }
    }

    return EndOffset;
}

internal b32
fbx_Parse(u8 *FbxData, size_t FbxDataSize, fbx_file *OutFbx) {
    b32 success = false;
    fbx_file *o = OutFbx;
    if (memcmp(FbxData, fbx_Magic, sizeof(fbx_Magic)) == 0) {
        o->Version = *(u32 *)(FbxData+0x17);

        printf("Version: %d\n", o->Version);

        u32 Next = 0x1B;
        o->Records = (fbx_record *)malloc(sizeof(fbx_record));
        memset(o->Records, 0, sizeof(fbx_record));
        fbx_record *CurrentRecord = o->Records;
        while (Next) {
            Next = fbx_ReadRecord(FbxData, Next, CurrentRecord);
            CurrentRecord->Next = (fbx_record *)malloc(sizeof(fbx_record));
            memset(CurrentRecord->Next, 0, sizeof(fbx_record));
            CurrentRecord = CurrentRecord->Next;
        }

        success = true;
    }

    return success;
}

i32 RecordIndent = 0;

internal char *
fbx_PropertyTypeToString(fbx_property_type Type) {
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

internal void
fbx_PrintRecords(fbx_record *Record) {
    ++RecordIndent;
    while (Record) {
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

    if (f) {
        fseek(f, 0, SEEK_END);
        FileSize = ftell(f);
        rewind(f);
        FileBuffer = (u8 *)malloc(FileSize);
        fread(FileBuffer, 1, FileSize, f);
        fclose(f);

        fbx_file FbxFile;
        if (fbx_Parse(FileBuffer, FileSize, &FbxFile)) {
            printf("Parsed file!\n");

            fbx_PrintRecords(FbxFile.Records);
        }
    }

    return 0;
}

