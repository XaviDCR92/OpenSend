#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/*******************************************************************//**
*
* @file     Interrupts.h
*
* @author   Xavier Del Campo
*
* @brief    Include file for Interrupts module.
*
************************************************************************/

/* *************************************
 * Includes
 * *************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/* *************************************
 * Defines
 * *************************************/

/* *************************************
 * Public types definition
 * *************************************/

/*******************************************************************//**
*
* \brief    List of HW interrupt sources.
*
************************************************************************/
enum InterruptSource
{
    INT_SOURCE_VBLANK,
    INT_SOURCE_GPU,
    INT_SOURCE_CDROM,
    INT_SOURCE_DMA,
    INT_SOURCE_RCNT0,
    INT_SOURCE_RCNT1,
    INT_SOURCE_RCNT2,
    INT_CONTROLLER_MEMCARD_BYTE_RECEIVED,
    INT_SIO,
    INT_SPU,
    INT_CONTROLLER_LIGHTPEN_PIO,

    MAX_INTERRUPT_SOURCES
};

/* *************************************
 * Public variables declaration
 * *************************************/

/* *************************************
 * Public functions declaration
 * *************************************/

void InterruptsEnableInt(const enum InterruptSource intSource);
void InterruptsDisableInt(const enum InterruptSource intSource);

#ifdef __cplusplus
}
#endif

#endif /* INTERRUPTS_H */
