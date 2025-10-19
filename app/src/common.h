/**
 * @file common.h
 * @brief Common definitions and includes for NISC Medical Wearable Device
 * @details This header provides common includes, macros, and type definitions
 * used throughout the medical wearable device application. It establishes
 * consistent coding standards and utility functions for the entire project.
 * 
 * @author NISC Medical Devices
 * @version 1.0.0
 * @date 2024
 * 
 * @note This file should be included by all source files in the project
 * to ensure consistent definitions and avoid conflicts.
 */

#ifndef COMMON_H
#define COMMON_H

/*============================================================================*/
/* Standard System Includes                                                   */
/*============================================================================*/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*============================================================================*/
/* Application Version Information                                            */
/*============================================================================*/

/** @defgroup AppVersion Application Version
 * @brief Version information for the medical wearable device
 * @details These constants define the firmware version following semantic
 * versioning principles for medical device software lifecycle management.
 * @{
 */

/** @brief Major version number */
#define APP_VERSION_MAJOR    1U

/** @brief Minor version number */
#define APP_VERSION_MINOR    0U

/** @brief Patch version number */
#define APP_VERSION_PATCH    0U

/** @brief Complete version string */
#define APP_VERSION_STRING   "1.0.0"

/** @} */ /* End of AppVersion group */

/*============================================================================*/
/* Device Information                                                         */
/*============================================================================*/

/** @defgroup DeviceInfo Device Information
 * @brief Static device identification information
 * @details These constants identify the device model and manufacturer
 * for regulatory compliance and device management.
 * @{
 */

/** @brief Device commercial name */
#define DEVICE_NAME          "NISC Medical Wearable"

/** @brief Device model identifier */
#define DEVICE_MODEL         "NMW-nRF52840"

/** @brief Manufacturer name */
#define MANUFACTURER         "NISC Medical Devices"

/** @} */ /* End of DeviceInfo group */

/*============================================================================*/
/* Common Error Codes                                                         */
/*============================================================================*/

/** @defgroup ErrorCodes Common Error Codes
 * @brief Standardized error codes used throughout the application
 * @details These error codes provide consistent error reporting across
 * all modules and subsystems for better debugging and error handling.
 * @{
 */

/** @brief Operation completed successfully */
#define SUCCESS              0

/** @brief Invalid parameter provided to function */
#define ERROR_INVALID_PARAM  -1

/** @brief Operation not supported in current context */
#define ERROR_NOT_SUPPORTED  -2

/** @brief Operation timed out */
#define ERROR_TIMEOUT        -3

/** @brief Insufficient memory available */
#define ERROR_NO_MEMORY      -4

/** @brief Resource is busy or unavailable */
#define ERROR_BUSY           -5

/** @} */ /* End of ErrorCodes group */

/*============================================================================*/
/* Utility Macros                                                             */
/*============================================================================*/

/** @defgroup UtilityMacros Utility Macros
 * @brief Common utility macros for code simplification and safety
 * @details These macros provide common operations and improve code
 * readability while maintaining type safety and consistency.
 * Note: Some common macros like MIN, MAX, CLAMP, and ARRAY_SIZE are 
 * already provided by Zephyr in zephyr/sys/util.h
 * @{
 */

/** @brief Mark unused parameter to avoid compiler warnings */
#define UNUSED(x)            ARG_UNUSED(x)

/** @} */ /* End of UtilityMacros group */

/*============================================================================*/
/* Time Conversion Macros                                                     */
/*============================================================================*/

/** @defgroup TimeConversion Time Conversion Macros
 * @brief Macros for time unit conversions
 * @details These macros provide consistent time conversions for Zephyr
 * kernel operations and timing calculations.
 * @{
 */

/** @brief Convert milliseconds to Zephyr ticks */
#define MS_TO_TICKS(ms)      K_MSEC(ms).ticks

/** @brief Convert seconds to Zephyr ticks */
#define SEC_TO_TICKS(sec)    K_SECONDS(sec).ticks

/** @brief Convert minutes to Zephyr ticks */
#define MIN_TO_TICKS(min)    K_MINUTES(min).ticks

/** @} */ /* End of TimeConversion group */

/*============================================================================*/
/* Memory and Alignment Macros                                               */
/*============================================================================*/

/** @defgroup MemoryMacros Memory and Alignment Macros
 * @brief Macros for memory operations and alignment
 * @details These macros assist with memory management and alignment
 * requirements for medical device applications.
 * @{
 */

/** @brief Align size to specified alignment boundary */
#define ALIGN_SIZE(size, align) (((size) + (align) - 1U) & ~((align) - 1U))

/** @} */ /* End of MemoryMacros group */

/*============================================================================*/
/* Bit Manipulation Macros                                                    */
/*============================================================================*/

/** @defgroup BitManipulation Bit Manipulation Macros  
 * @brief Safe bit manipulation operations
 * @details These macros provide safe and clear bit manipulation
 * operations commonly used in embedded medical device software.
 * @{
 */

/** @brief Set a specific bit in a register */
#define SET_BIT(reg, bit)    ((reg) |= (1U << (bit)))

/** @brief Clear a specific bit in a register */
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(1U << (bit)))

/** @brief Toggle a specific bit in a register */
#define TOGGLE_BIT(reg, bit) ((reg) ^= (1U << (bit)))

/** @brief Check if a specific bit is set */
#define IS_BIT_SET(reg, bit) (((reg) & (1U << (bit))) != 0U)

/** @} */ /* End of BitManipulation group */

/*============================================================================*/
/* String Utility Macros                                                      */
/*============================================================================*/

/** @defgroup StringUtils String Utility Macros
 * @brief String manipulation and conversion macros
 * @details These macros provide string utilities for logging and
 * debug output in medical device applications.
 * @{
 */

/** @brief Helper macro for stringification */
#define STR_HELPER(x)        #x

/** @brief Convert macro value to string */
#define STR(x)               STR_HELPER(x)

/** @} */ /* End of StringUtils group */

/*============================================================================*/
/* Compiler Attributes                                                        */
/*============================================================================*/

/** @defgroup CompilerAttributes Compiler Attributes
 * @brief Compiler-specific attributes for code optimization
 * @details These attributes provide compiler hints for optimization
 * and code generation in medical device firmware.
 * @{
 */

/** @brief Mark structure as packed (no padding) */
#define PACKED               __packed

/** @brief Align variable or structure to specified boundary */
#define ALIGNED(n)           __aligned(n)

/** @brief Mark function as weak (can be overridden) */
#define WEAK                 __weak

/** @brief Mark function as never returning */
#define NO_RETURN            __attribute__((noreturn))

/** @} */ /* End of CompilerAttributes group */

/*============================================================================*/
/* Build-Time Assertions                                                      */
/*============================================================================*/

/** @defgroup BuildAssertions Build-Time Assertions
 * @brief Compile-time assertion utilities
 * @details These macros enable compile-time validation of constants
 * and assumptions critical for medical device safety.
 * Note: BUILD_ASSERT is already provided by Zephyr toolchain
 * @{
 */

/** @note BUILD_ASSERT macro is provided by Zephyr toolchain headers */

/** @} */ /* End of BuildAssertions group */

/*============================================================================*/
/* Logging Configuration                                                       */
/*============================================================================*/

/** @defgroup LoggingConfig Logging Configuration
 * @brief Default logging configuration
 * @details Default logging module configuration for files that don't
 * define their own LOG_MODULE_NAME.
 * @{
 */

/** @brief Default logging module name */
#ifndef LOG_MODULE_NAME
#define LOG_MODULE_NAME      medical_wearable
#endif

/** @} */ /* End of LoggingConfig group */

#endif /* COMMON_H */