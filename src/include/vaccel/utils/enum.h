// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Define enums (with optional values) and string mapping functions.
 * This macro generates:
 * - <name>_t enum with specified values
 * - <name>_to_str() function to get an enum item name as string
 * - <name>_to_base_str() function to get an enum item base (unprefixed) name
 *   as string */
#define VACCEL_ENUM_DEF_WITH_STR_FUNCS(name, prefix, VACCEL_ENUM_LIST) \
	typedef enum {                                                 \
		VACCEL_ENUM_DEF_ITEMS(prefix, VACCEL_ENUM_LIST)        \
	} name##_t;                                                    \
                                                                       \
	VACCEL_ENUM_DEF_STR_FUNCS(name, prefix, VACCEL_ENUM_LIST)

/* Define enum items from a given list.
 * Note: It automatically adds a <prefix>_MAX guard as the last element. */
#define VACCEL_ENUM_DEF_ITEMS(prefix, VACCEL_ENUM_LIST) \
	VACCEL_ENUM_LIST(VACCEL_ENUM_ITEM)              \
	VACCEL_ENUM_ITEM(MAX, prefix)

/* Define enum string mapping functions from a given list */
#define VACCEL_ENUM_DEF_STR_FUNCS(name, prefix, VACCEL_ENUM_LIST)    \
	static inline const char *name##_to_str(name##_t value)      \
	{                                                            \
		switch (value) {                                     \
			VACCEL_ENUM_LIST(VACCEL_ENUM_STR_CASE)       \
		default:                                             \
			return VACCEL_ENUM_STR(MAX, prefix);         \
		}                                                    \
	}                                                            \
                                                                     \
	static inline const char *name##_to_base_str(name##_t value) \
	{                                                            \
		switch (value) {                                     \
			VACCEL_ENUM_LIST(VACCEL_ENUM_BASE_STR_CASE)  \
		default:                                             \
			return "MAX";                                \
		}                                                    \
	}

/* Define enum item.
 * This macro will generate an enum item of the form:
 * <prefix>_<name>[ = <value>]
 * Usage: VACCEL_ENUM_ITEM(<name>,[ <value>,] <prefix>) */
#define VACCEL_ENUM_ITEM(...)                                           \
	VACCEL_ENUM_GET_MACRO(__VA_ARGS__, VACCEL_ENUM_ITEM_WITH_VALUE, \
			      VACCEL_ENUM_ITEM_NO_VALUE)                \
	(__VA_ARGS__),

/* Define switch case to return a string from an enum item.
 * This macro will generate a switch case of the form:
 * case <prefix>_<name>: return "<prefix>_<name>"
 * Usage: VACCEL_ENUM_STR_CASE(<name>,[ <value>,] <prefix>) */
#define VACCEL_ENUM_STR_CASE(...)                                           \
	VACCEL_ENUM_GET_MACRO(__VA_ARGS__, VACCEL_ENUM_STR_CASE_WITH_VALUE, \
			      VACCEL_ENUM_STR_CASE_NO_VALUE)                \
	(__VA_ARGS__)

/* Define switch case to return a base string from an enum item.
 * This macro will generate a switch case of the form:
 * case <prefix>_<name>: return "<name>"
 * Usage: VACCEL_ENUM_BASE_STR_CASE(<name>,[ <value>,] <prefix>) */
#define VACCEL_ENUM_BASE_STR_CASE(...)                              \
	VACCEL_ENUM_GET_MACRO(__VA_ARGS__,                          \
			      VACCEL_ENUM_BASE_STR_CASE_WITH_VALUE, \
			      VACCEL_ENUM_BASE_STR_CASE_NO_VALUE)   \
	(__VA_ARGS__)

/* Misc helper macros to define enum item and switch case string */
#define VACCEL_ENUM_ITEM_NO_VALUE(name, prefix) prefix##_##name
#define VACCEL_ENUM_ITEM_WITH_VALUE(name, value, prefix) prefix##_##name = value

#define VACCEL_ENUM_STR_CASE_NO_VALUE(name, prefix)   \
	case VACCEL_ENUM_ITEM_NO_VALUE(name, prefix): \
		return VACCEL_ENUM_STR(name, prefix);
#define VACCEL_ENUM_STR_CASE_WITH_VALUE(name, value, prefix) \
	case VACCEL_ENUM_ITEM_NO_VALUE(name, prefix):        \
		return VACCEL_ENUM_STR(name, prefix);
#define VACCEL_ENUM_STR(name, prefix) #prefix "_" #name

#define VACCEL_ENUM_BASE_STR_CASE_NO_VALUE(name, prefix) \
	case VACCEL_ENUM_ITEM_NO_VALUE(name, prefix):    \
		return #name;
#define VACCEL_ENUM_BASE_STR_CASE_WITH_VALUE(name, value, prefix) \
	case VACCEL_ENUM_ITEM_NO_VALUE(name, prefix):             \
		return #name;

#define VACCEL_ENUM_GET_MACRO(_1, _2, _3, NAME, ...) NAME

#ifdef __cplusplus
}
#endif
