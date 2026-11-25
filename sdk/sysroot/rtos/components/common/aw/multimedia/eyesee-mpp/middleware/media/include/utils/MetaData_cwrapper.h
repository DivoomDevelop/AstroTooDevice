/*
 * Copyright (C) 2009 The Android Open Source Project
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
 */

#ifndef META_DATA_CWRAPPER_H_
#define META_DATA_CWRAPPER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to MetaData C++ object */
typedef void* MetaDataHandle;

/* Create a new MetaData object */
MetaDataHandle MetaData_Create(void);

/* Destroy a MetaData object */
void MetaData_Destroy(MetaDataHandle handle);

/* Set int32 value */
int MetaData_setInt32(MetaDataHandle handle, uint32_t key, int32_t value);

/* Find int32 value */
int MetaData_findInt32(MetaDataHandle handle, uint32_t key, int32_t *value);

/* Set int64 value */
int MetaData_setInt64(MetaDataHandle handle, uint32_t key, int64_t value);

/* Find int64 value */
int MetaData_findInt64(MetaDataHandle handle, uint32_t key, int64_t *value);

/* Set C string value */
int MetaData_setCString(MetaDataHandle handle, uint32_t key, const char *value);

/* Find C string value */
int MetaData_findCString(MetaDataHandle handle, uint32_t key, const char **value);

/* Set float value */
int MetaData_setFloat(MetaDataHandle handle, uint32_t key, float value);

/* Find float value */
int MetaData_findFloat(MetaDataHandle handle, uint32_t key, float *value);

/* Set pointer value */
int MetaData_setPointer(MetaDataHandle handle, uint32_t key, void *value);

/* Find pointer value */
int MetaData_findPointer(MetaDataHandle handle, uint32_t key, void **value);

#ifdef __cplusplus
}
#endif

#endif  /* META_DATA_CWRAPPER_H_ */
