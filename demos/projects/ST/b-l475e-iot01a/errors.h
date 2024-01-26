/*
 * erros.h
 *
 *  Created on: 18 paź 2022
 *      Author: Wiktor Komorowski
 */

#ifndef COMPONENTS_ERRORS_INC_ERRORS_H_
#define COMPONENTS_ERRORS_INC_ERRORS_H_

#ifdef __cplusplus
 extern "C" {
#endif

//========================================================================================================== INCLUDES
#include <stdint.h>

//========================================================================================================== DEFINITIONS

//========================================================================================================== VARIABLES
enum errors {
	FAILED = 0,
	DONE
};

typedef uint8_t error_t;

//========================================================================================================== FUNCTIONS DECLARATIONS

//========================================================================================================== FUNCTIONS DEFINITIONS

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_ERRORS_INC_ERRORS_H_ */
