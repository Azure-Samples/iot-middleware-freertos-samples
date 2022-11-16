/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Additional functions which are needed for compilation with the middleware have been placed in */
/* mock_needed_functions.c for brevity in this file. */

/*
 * Test hook.
 */
extern int vStartTestTask( void );

/*-----------------------------------------------------------*/

int main( void )
{
    return vStartTestTask();
}
/*-----------------------------------------------------------*/
