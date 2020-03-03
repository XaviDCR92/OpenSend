#ifndef IO_H
#define IO_H

/*******************************************************************//**
*
* \file     IO.h
*
* \author   Xavier Del Campo
*
* \brief    Include file for IO module.
*
************************************************************************/

/* *************************************
 * Includes
 * *************************************/

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* *************************************
 * Defines
 * *************************************/

#define IO_INVALID_FILE_SIZE    (size_t)(-1)

/* *************************************
 * Public types definition
 * *************************************/

/* *************************************
 * Public variables declaration
 * *************************************/

/* *************************************
 * Public functions declaration
 * *************************************/

const uint8_t *IOLoadFile(const char* const strFilePath, size_t* const fileSize);

#ifdef __cplusplus
}
#endif

#endif /* IO_H */
