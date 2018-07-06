
#ifndef __CONFIG_H
#define __CONFIG_H

#define _S(x)                       #x
#define S(x)                        _S(x)
#define _CONCAT(X,Y)                X##Y
#define CONCAT(X,Y)                 _CONCAT(X,Y)

#define FUNCTION_PREFIX             S(MACKE_PREFIX)
#define MACKE_ID_NAME(name)         CONCAT(MACKE_PREFIX, name)

#define DRIVER_PTR_ID               MACKE_ID_NAME(DRIVER_PTR_NAME_SUFFIX)
#define DRIVER_PTR_ID_STRING        S(DRIVER_PTR_ID)

#define DRIVER_ARRAY_ID             MACKE_ID_NAME(DRIVER_ARRAY_ID_SUFFIX)
#define DRIVER_ARRAY_ID_STRING      S(DRIVER_ARRAY_ID)

#define DRIVER_DESC_ID              MACKE_ID_NAME(DRIVER_DESC_SUFFIX)
#define DRIVER_DESC_ID_STRING       S(DRIVER_DESC_ID)


#define MACKE_PREFIX                macke_fuzzer_
#define DRIVER_PTR_NAME_SUFFIX      ptr_driver
#define DRIVER_ARRAY_ID_SUFFIX      drivers
#define DRIVER_DESC_SUFFIX          driver_desc


/* For array splitting/extraction */
#ifndef ESCAPE_CHAR
#define ESCAPE_CHAR '\xF9'
#endif

#ifndef DELIMITER_CHAR
#define DELIMITER_CHAR '\xFA'
#endif

#ifndef INITIAL_INPUT_FILL_CHAR
#define INITIAL_INPUT_FILL_CHAR '\0'
#endif


#ifdef __cplusplus

constexpr const char* DriverPtrName = DRIVER_PTR_ID_STRING;
constexpr const char* DriverDescName = DRIVER_DESC_ID_STRING;
constexpr const char* DriverArrayName = DRIVER_ARRAY_ID_STRING;
constexpr const char* LibFuzzerDriverName = "LLVMFuzzerTestOneInput";
constexpr const char* LibFuzzerInitializerName = "LLVMFuzzerTestOneInput";
#endif

#endif // __CONFIG_H
