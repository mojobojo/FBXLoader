#ifndef __FBX_LOADER_H_
#define __FBX_LOADER_H_

/* 
    - Currently used c functions
        - malloc
        - free
*/
#include <stdlib.h>
#include <stdint.h>

// for overriding malloc and free functions with your own memory allocater
#ifndef fbxl_malloc
#define fbxl_malloc malloc
#endif

#ifndef fbxl_mfree
#define fbxl_mfree free
#endif

#define fbxl_internal static
#define fbxl_Min(A, B) (A < B ? A : B)
#define fbxl_Max(A, B) (A > B ? A : B)

#define FBX_MAGIC "Kaydara FBX Binary  "

typedef int8_t   fbxl_i8;
typedef uint8_t  fbxl_u8;
typedef int16_t  fbxl_i16;
typedef uint16_t fbxl_u16;
typedef int32_t  fbxl_i32;
typedef uint32_t fbxl_u32;
typedef int64_t  fbxl_i64;
typedef uint64_t fbxl_u64;
typedef float    fbxl_f32;
typedef double   fbxl_f64;
typedef fbxl_i32 fbxl_b32;

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
    fbxl_b32           Compressed;
    fbxl_u32           Size;
    fbxl_b32           CompressedSize;
    void              *Data;
};

struct fbx_record {
    char          Name[64];
    fbxl_u32      PropertiesCount;
    fbx_property *Properties;
    fbx_record   *SubRecord;
    fbx_record   *Next;
};

struct fbx_file {
    fbxl_u32    Version;
    fbx_record *Records;
};

// FbxData      - Loaded fbx file in memory
// FbxDataSize  - Size of the loaded fbx file in memory
// OutFbx       - Pointer to an out files
fbxl_internal fbxl_b32 fbxl_Parse(fbxl_u8 *FbxData, size_t FbxDataSize, fbx_file *OutFbx);

// FbxFile - Pointer to a loaded fbx file
fbxl_internal void fbxl_Free(fbx_file *FbxFile);

#ifdef FBX_LOADER_IMPLEMENTATION

fbxl_internal fbxl_u32 fbxl_ReadRecord(fbxl_u8 *FbxData, fbxl_u32 Start, fbx_record *Record) {

    fbxl_u8 *s = FbxData+Start;

    fbxl_u32 EndOffset = *(fbxl_u32 *)s;

    if (EndOffset) {
        fbxl_u32 NumProperties = *(fbxl_u32 *)(s+4);
        //fbxl_u32 PropertyListLength = *(fbxl_u32 *)(s+8);
        fbxl_u8 NameLen = *(s+12);
        // TODO(joey): Figure out if names are a finite size
        Record->Name[64] = {};
        memcpy(Record->Name, s+13, fbxl_Min(NameLen, 63));

        Record->PropertiesCount = NumProperties;
        Record->Properties = (fbx_property *)fbxl_malloc(NumProperties*sizeof(fbx_property));
        memset(Record->Properties, 0, NumProperties*sizeof(fbx_property));

        s += 13+NameLen;
        for (fbxl_u32 i = 0; i < NumProperties; ++i) {
            fbx_property *p = &Record->Properties[i];
            fbxl_u8 Type = *s++;
            switch (Type) {
                case 'Y':{
                    p->Type = FBX_PROPERTY_S16;
                    p->Size = 2;
                    p->Data = s;
                    s += 2;
                    break;
                }
                case 'C':{
                    p->Type = FBX_PROPERTY_BOOL;
                    p->Size = 1;
                    p->Data = s;
                    ++s;
                    break;
                }
                case 'I':{
                    p->Type = FBX_PROPERTY_S32;
                    p->Size = 4;
                    p->Data = s;
                    s += 4;
                    break;
                }
                case 'F':{
                    p->Type = FBX_PROPERTY_F32;
                    p->Size = 4;
                    p->Data = s;
                    s += 4;
                    break;
                }
                case 'D':{
                    p->Type = FBX_PROPERTY_F64;
                    p->Size = 8;
                    p->Data = s;
                    s += 8;
                    break;
                }
                case 'L':{
                    p->Type = FBX_PROPERTY_S64;
                    p->Size = 8;
                    p->Data = s;
                    s += 8;
                    break;
                }
                case 'f':{
                    fbxl_u32 ArrayLen = *(fbxl_u32 *)s;
                    fbxl_u32 Encoding = *(fbxl_u32 *)(s+4);
                    fbxl_u32 CompressedLen = *(fbxl_u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_F32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = s+12;
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_F32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->Data = s+12;
                        s += 12+(ArrayLen*4);
                    }
                    break;
                }
                case 'd':{
                    fbxl_u32 ArrayLen = *(fbxl_u32 *)s;
                    fbxl_u32 Encoding = *(fbxl_u32 *)(s+4);
                    fbxl_u32 CompressedLen = *(fbxl_u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_F64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = s+12;
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_F64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->Data = s+12;
                        s += 12+(ArrayLen*8);
                    }
                    break;
                }
                case 'i':{
                    fbxl_u32 ArrayLen = *(fbxl_u32 *)s;
                    fbxl_u32 Encoding = *(fbxl_u32 *)(s+4);
                    fbxl_u32 CompressedLen = *(fbxl_u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_S32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = s+12;
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_S32_ARRAY;
                        p->Size = ArrayLen*4;
                        p->Data = s+12;
                        s += 12+(ArrayLen*4);
                    }
                    break;
                }

                case 'l':{
                    fbxl_u32 ArrayLen = *(fbxl_u32 *)s;
                    fbxl_u32 Encoding = *(fbxl_u32 *)(s+4);
                    fbxl_u32 CompressedLen = *(fbxl_u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_S64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = s+12;
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_S64_ARRAY;
                        p->Size = ArrayLen*8;
                        p->Data = s+12;
                        s += 12+(ArrayLen*8);
                    }
                    break;
                }
                case 'b':{
                    fbxl_u32 ArrayLen = *(fbxl_u32 *)s;
                    fbxl_u32 Encoding = *(fbxl_u32 *)(s+4);
                    fbxl_u32 CompressedLen = *(fbxl_u32 *)(s+8);

                    if (Encoding) {
                        p->Type = FBX_PROPERTY_BOOL_ARRAY;
                        p->Size = ArrayLen*1;
                        p->CompressedSize = CompressedLen;
                        p->Compressed = true;
                        p->Data = s+12;
                        s += 12+CompressedLen;
                    }
                    else {
                        p->Type = FBX_PROPERTY_BOOL_ARRAY;
                        p->Size = ArrayLen*1;
                        p->Data = s+12;
                        s += 12+(ArrayLen*1);
                    }
                    break;
                }
                case 'S':{
                    fbxl_u32 Len = *(fbxl_u32 *)s;
                    p->Type = FBX_PROPERTY_STRING;
                    p->Size = Len;
                    p->Data = s+4;
                    s += 4+Len;
                    break;
                }
                case 'R': {
                    fbxl_u32 Len = *(fbxl_u32 *)s;
                    p->Type = FBX_PROPERTY_RAW;
                    p->Size = Len;
                    p->Data = s+4;
                    s += 4+Len;
                    break;
                }
                default: {
                    //Assert(!"Unknown type");
                    *(int*)0=0;
                }
            }
        }

        fbxl_u64 NextOffset = s-FbxData;

        fbxl_u32 Next = (fbxl_u32)NextOffset;
        Record->SubRecord = (fbx_record *)fbxl_malloc(sizeof(fbx_record));
        memset(Record->SubRecord, 0, sizeof(fbx_record));
        fbx_record *CurrentRecord = Record->SubRecord;
        while (Next < EndOffset) {
            Next = fbxl_ReadRecord(FbxData, Next, CurrentRecord);
            if (!Next) {
                break;
            }
            CurrentRecord->Next = (fbx_record *)fbxl_malloc(sizeof(fbx_record));
            memset(CurrentRecord->Next, 0, sizeof(fbx_record));
            CurrentRecord = CurrentRecord->Next;
        }
    }

    return EndOffset;
}

fbxl_internal fbxl_b32 fbxl_Parse(fbxl_u8 *FbxData, size_t FbxDataSize, fbx_file *OutFbx) {
    fbxl_b32 success = false;
    fbx_file *o = OutFbx;
    if (memcmp(FbxData, FBX_MAGIC, sizeof(FBX_MAGIC)) == 0) {
        o->Version = *(fbxl_u32 *)(FbxData+0x17);

        printf("Version: %d\n", o->Version);

        fbxl_u32 Next = 0x1B;
        o->Records = (fbx_record *)fbxl_malloc(sizeof(fbx_record));
        memset(o->Records, 0, sizeof(fbx_record));
        fbx_record *CurrentRecord = o->Records;
        while (Next) {
            Next = fbxl_ReadRecord(FbxData, Next, CurrentRecord);
            CurrentRecord->Next = (fbx_record *)fbxl_malloc(sizeof(fbx_record));
            memset(CurrentRecord->Next, 0, sizeof(fbx_record));
            CurrentRecord = CurrentRecord->Next;
        }

        success = true;
    }

    return success;
}

fbxl_internal void fbxl_FreeRecord(fbx_record *Record) {
    while (Record) {
        if (Record->SubRecord) {
            fbxl_FreeRecord(Record->SubRecord);
        }
        fbxl_mfree(Record);
        Record = Record->Next;
    }
}

fbxl_internal void fbxl_Free(fbx_file *FbxFile) {
    if (FbxFile && FbxFile->Records) {
        fbxl_FreeRecord(FbxFile->Records);
    }

    FbxFile->Records = 0;
}
#endif

#endif

