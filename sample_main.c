/*
 * Copyright (C) 2013 KLab Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * gcc:
 * gcc -o exif sample_main.c exif.c
 *
 * Microsoft Visual C++:
 * cl.exe /o exif sample_main.c exif.c
 */

#ifdef _MSC_VER
#include <windows.h>
#include <malloc.h>
#endif
#include <stdio.h>
#include <stdlib.h>     // for malloc, free
#include <string.h>     // for strcpy

#include "exif.h"

// sample functions
int sample_addThumbnail(const char *srcJpgFileName, const char*srcThumbFileName, const char *outJpgFileName);
int sample_removeExifSegment(const char *srcJpgFileName, const char *outJpgFileName);
int sample_removeSensitiveData(const char *srcJpgFileName, const char *outJpgFileName);
int sample_queryTagExists(const char *srcJpgFileName);
int sample_updateTagData(const char *srcJpgFileName, const char *outJpgFileName);
int sample_saveThumbnail(const char *srcJpgFileName, const char *outFileName);

void reportResult(int result, const char* filename)
{
    // check result status
    switch (result) {
    case 0: // no IFDs
        printf("[%s] does not seem to contain the Exif segment.\n", filename);
        break;
    case ERR_READ_FILE:
        printf("failed to open or read [%s].\n", filename);
        break;
    case ERR_INVALID_JPEG:
        printf("[%s] is not a valid JPEG file.\n", filename);
        break;
    case ERR_INVALID_APP1HEADER:
        printf("[%s] does not have valid Exif segment header.\n", filename);
        break;
    case ERR_INVALID_IFD:
        printf("[%s] contains one or more IFD errors. use -v for details.\n", filename);
        break;
    default:
        printf("[%s] createIfdTableArray: result=%d\n", filename, result);
        break;
    }
}

// sample
int main(int ac, char *av[])
{
    void **ifdArray;
    TagNodeInfo *tag;
    int i, result;
    int addFlag = 0;
    int infoFlag = 0;
    int removeFlag = 0;
    int stripFlag = 0;
    int thumbnailFlag = 0;
    int updateFlag = 0;

#ifdef _MSC_VER
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
#endif

    if (ac < 2) {
        printf("usage: %s <JPEG FileName> [-a]dd [-i]nfo [-r]emove [-s]trip [-t]humbnail [-u]pdate [-v]erbose\n", av[0]);
        return 0;
    }

    for (i = 2; i < ac; ++i) {
        // -v option
        const char* arg = av[i];
        if (arg[0] == '-' || arg[0] == '/') {
            switch (arg[1]) {
                case 'a':
                    addFlag = 1;;
                    break;
                case 'i':
                    infoFlag = 1;
                    break;
                case 'r':
                    removeFlag = 1;
                    break;
                case 's':
                    stripFlag = 1;
                    break;
                case 't':
                    thumbnailFlag = 1;
                    break;
                case 'u':
                    updateFlag = 1;
                    break;
                case 'v':
                    setVerbose(1);
                    break;
                default:
                    fprintf(stderr, "Invalid option %s!\n", arg);
                    return -1;
            }
        }
    }

    // parse the JPEG header and create the pointer array of the IFD tables
    const char* filename = av[1];
    ifdArray = createIfdTableArray(filename, &result);
    reportResult(result, filename);

    if (!ifdArray) {
        return 0;
    }

    // dump all IFD tables
    for (i = 0; ifdArray[i] != NULL; i++) {
        dumpIfdTable(ifdArray[i]);
    }
    // or dumpIfdTableArray(ifdArray);

    printf("\n");

    // get [Model] tag value from 0th IFD
    tag = getTagInfo(ifdArray, IFD_0TH, TAG_Model);
    if (tag) {
        if (!tag->error) {
            printf("0th IFD : Model = [%s]\n", tag->byteData);
        }
        freeTagInfo(tag);
    }

    // get [DateTimeOriginal] tag value from Exif IFD
    tag = getTagInfo(ifdArray, IFD_EXIF, TAG_DateTimeOriginal);
    if (tag) {
        if (!tag->error) {
            printf("Exif IFD : DateTimeOriginal = [%s]\n", tag->byteData);
        }
        freeTagInfo(tag);
    }

    // get [GPSLatitude] tag value from GPS IFD
    tag = getTagInfo(ifdArray, IFD_GPS, TAG_GPSLatitude);
    if (tag) {
        if (!tag->error) {
            printf("GPS IFD : GPSLatitude = ");
            for (i = 0; i < (int)tag->count*2; i+=2) {
                printf("%u/%u ", tag->numData[i], tag->numData[i+1]);
            }
            printf("\n");
        }
        freeTagInfo(tag);
    }

    // free IFD table array
    freeIfdTableArray(ifdArray);


    // sample function A: remove the Exif segment in a JPEG file
    if (stripFlag) {
        result = sample_removeExifSegment(filename, "removeExif.jpg");
        printf("sample_removeExifSegment(%s)=%d\n", filename, result);
    }

    // sample function B: remove sensitive Exif data in a JPEG file
    if (removeFlag) {
        result = sample_removeSensitiveData(filename, "removeSensitive.jpg");
        printf("sample_removeSensitiveData(%s)=%d\n", filename, result);
    }

    // sample function C: check if "GPSLatitude" tag exists in GPS IFD
    if (infoFlag) {
        result = sample_queryTagExists(filename);
        printf("sample_queryTagExists(%s)=%d\n", filename, result);
    }

    // sample function D: Update the value of "Make" tag in 0th IFD
    if (updateFlag) {
        result = sample_updateTagData(filename, "updateTag.jpg");
        printf("sample_updateTagData(%s)=%d\n", filename, result);
    }

    // sample function E: Write Exif thumbnail data to file
    if (thumbnailFlag) {
        result = sample_saveThumbnail(filename, "thumbnail.jpg");
        printf("sample_saveThumbnail(%s)=%d\n", filename, result);
    }

    // sample function F: Add Exif thumbnail data to file
    if (addFlag) {
        result = sample_addThumbnail(filename, "thumbnail.jpg", "withthumbnail.jpg");
        printf("sample_addThumbnail(%s)=%d\n", filename, result);
    }

    return result;
}

/**
 * sample_removeExifSegment()
 *
 * remove the Exif segment in a JPEG file
 *
 */
int sample_removeExifSegment(const char *srcJpgFileName, const char *outJpgFileName)
{
    int sts = removeExifSegmentFromJPEGFile(srcJpgFileName, outJpgFileName);
    if (sts <= 0) {
        printf("removeExifSegmentFromJPEGFile: ret=%d\n", sts);
    }
    return sts;
}

/**
 * sample_addThumbnail()
 *
 * remove sensitive Exif data in a JPEG file
 *
 */
const uint8_t JFIF_header[] =
{
    0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01
};
int sample_addThumbnail(const char *srcJpgFileName, const char*srcThumbFileName, const char *outJpgFileName)
{
    // First try to read in thumbnail file
    FILE* fp = fopen(srcThumbFileName, "rb");
    if (fp == NULL) {
        printf("sampleAddThumbnail: Can't open file [%s]\n", srcThumbFileName);
        return ERR_NOT_EXIST;
    }
    // Get size of file
    fseek(fp, 0L, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    printf("sample_addThumbnail: len = %u\n", len);
    uint8_t* buf = (uint8_t*) malloc(len);
    if (buf == NULL) {
        return ERR_MEMALLOC;        
    }
    size_t numread = fread(buf, 1, len, fp);
    if (numread != len) {
        return ERR_READ_FILE;
    }
    fclose(fp);

    uint8_t* thumb = buf;
    if (memcmp(buf, JFIF_header, sizeof(JFIF_header) - 8) == 0) {
        thumb += sizeof(JFIF_header);
        len -= sizeof(JFIF_header);
        thumb[0] = 0xff;
        thumb[1] = 0xd8;
    }

    int sts;
    void* ifdTable[32];
    int count = fillIfdTableArray(srcJpgFileName, ifdTable);
    if (count <= 0) {
        printf("createIfdTableArray: ret=%d\n", count);
        return count;
    }

    // add ThumbnailData
    sts = setThumbnailDataOnIfdTableArray(ifdTable, thumb, len);
    if (sts < 0) {
        printf("setThumbnailDataOnIfdTableArray: ret=%d\n", sts);
    }
    free(buf);

    // update the Exif segment
    sts = updateExifSegmentInJPEGFile(srcJpgFileName, outJpgFileName, ifdTable);
    if (sts < 0) {
        printf("updateExifSegmentInJPEGFile: ret=%d\n", sts);
    }
    freeIfdTables(ifdTable);
    return sts;
}

/**
 * sample_removeSensitiveData()
 *
 * remove sensitive Exif data in a JPEG file
 *
 */
int sample_removeSensitiveData(const char *srcJpgFileName, const char *outJpgFileName)
{
    int sts, result;
    void **ifdTableArray = createIfdTableArray(srcJpgFileName, &result);

    if (!ifdTableArray) {
        printf("createIfdTableArray: ret=%d\n", result);
        return result;
    }

    // remove GPS IFD and 1st IFD if exist
    removeIfdTableFromIfdTableArray(ifdTableArray, IFD_GPS);
    removeIfdTableFromIfdTableArray(ifdTableArray, IFD_1ST);
    
    // remove tags if exist
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_Make);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_Model);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_DateTime);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_ImageDescription);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_Software);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_Artist);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_MakerNote);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_UserComment);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_DateTimeOriginal);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_DateTimeDigitized);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_SubSecTime);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_SubSecTimeOriginal);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_SubSecTimeDigitized);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_ImageUniqueID);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_CameraOwnerName);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_BodySerialNumber);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_LensMake);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_LensModel);
    removeTagNodeFromIfdTableArray(ifdTableArray, IFD_EXIF, TAG_LensSerialNumber);
    
    // update the Exif segment
    sts = updateExifSegmentInJPEGFile(srcJpgFileName, outJpgFileName, ifdTableArray);
    if (sts < 0) {
        printf("updateExifSegmentInJPEGFile: ret=%d\n", sts);
    }
    freeIfdTableArray(ifdTableArray);
    return sts;
}

/**
 * sample_queryTagExists()
 *
 * check if "GPSLatitude" tag exists in GPS IFD
 *
 */
int sample_queryTagExists(const char *srcJpgFileName)
{
    int sts, result;
    void **ifdTableArray = createIfdTableArray(srcJpgFileName, &result);
    if (!ifdTableArray) {
        printf("createIfdTableArray: ret=%d\n", result);
        return result;
    }

    sts = queryTagNodeIsExist(ifdTableArray, IFD_GPS, TAG_GPSLatitude);
    printf("GPSLatitude tag is %s in [%s]\n", (sts) ? "exists" : "not exists", srcJpgFileName);

    freeIfdTableArray(ifdTableArray);
    return sts;
}

/**
 * sample_updateTagData()
 *
 * Update the value of "Make" tag in 0th IFD
 *
 */
int sample_updateTagData(const char *srcJpgFileName, const char *outJpgFileName)
{
    TagNodeInfo *tag;
    int sts, result;
    void **ifdTableArray = createIfdTableArray(srcJpgFileName, &result);

    if (ifdTableArray != NULL) {
        if (queryTagNodeIsExist(ifdTableArray, IFD_0TH, TAG_Make)) {
            removeTagNodeFromIfdTableArray(ifdTableArray, IFD_0TH, TAG_Make);
        }
    } else { // Exif segment not exists
        // create new IFD table
        ifdTableArray = insertIfdTableToIfdTableArray(NULL, IFD_0TH, &result);
        if (!ifdTableArray) {
            printf("insertIfdTableToIfdTableArray: ret=%d\n", result);
            return 0;
        }
    }
    // create a tag info
    tag = createTagInfo(TAG_Make, TYPE_ASCII, 6, &result);
    if (!tag) {
        printf("createTagInfo: ret=%d\n", result);
        freeIfdTableArray(ifdTableArray);
        return result;
    }
    // set tag data
    strcpy((char*)tag->byteData, "ABCDE");
    // insert to IFD table
    insertTagNodeToIfdTableArray(ifdTableArray, IFD_0TH, tag);
    freeTagInfo(tag);

    // write file
    sts = updateExifSegmentInJPEGFile(srcJpgFileName, outJpgFileName, ifdTableArray);

    if (sts < 0) {
        printf("updateExifSegmentInJPEGFile: ret=%d\n", sts);
    }
    freeIfdTableArray(ifdTableArray);
    return sts;
}

/**
 * sample_saveThumbnail()
 *
 * Write Exif thumbnail data to file
 *
 */
int sample_saveThumbnail(const char *srcJpgFileName, const char *outFileName)
{
    unsigned char *p;
    unsigned int len;
    FILE *fp;
    int result;

    void **ifdTableArray = createIfdTableArray(srcJpgFileName, &result);
    if (!ifdTableArray) {
        printf("createIfdTableArray: ret=%d\n", result);
        return result;
    }

    // try to get thumbnail data from 1st IFD
    p = getThumbnailDataOnIfdTableArray(ifdTableArray, &len, &result);
    if (!p) {
        printf("getThumbnailDataOnIfdTableArray: ret=%d\n", result);
        freeIfdTableArray(ifdTableArray);
        return result;
    }
    // save thumbnail
    fp = fopen(outFileName, "wb");
    if (!fp) {
        printf("failed to create [%s]\n", outFileName);
        freeIfdTableArray(ifdTableArray);
        return 0;
    }
    fwrite(p, 1, len, fp);
    fclose(fp);

    free(p);
    freeIfdTableArray(ifdTableArray);
    return 0;
}
