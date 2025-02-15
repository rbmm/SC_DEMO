#pragma once

DECLSPEC_IMPORT
protected: virtual void bad_cast::_Doraise(void)const ;

DECLSPEC_IMPORT
protected: virtual void bad_typeid::_Doraise(void)const ;

DECLSPEC_IMPORT
void _inconsistency(void);

DECLSPEC_IMPORT
int _is_exception_typeof(class type_info const &,struct _EXCEPTION_POINTERS *);

DECLSPEC_IMPORT
int (*_query_new_handler(void))(unsigned __int64);

DECLSPEC_IMPORT
int _query_new_mode(void);

DECLSPEC_IMPORT
int (*_set_new_handler(int (*)(unsigned __int64)))(unsigned __int64);

DECLSPEC_IMPORT
int _set_new_mode(int);

DECLSPEC_IMPORT
void (*_set_se_translator(void (*)(unsigned int,struct _EXCEPTION_POINTERS *)))(unsigned int,struct _EXCEPTION_POINTERS *);

DECLSPEC_IMPORT
public: int type_info::before(class type_info const &)const ;

DECLSPEC_IMPORT
public: char const * type_info::name(struct __type_info_node *)const ;

DECLSPEC_IMPORT
public: char const * type_info::name(void)const ;

DECLSPEC_IMPORT
public: char const * type_info::raw_name(void)const ;

DECLSPEC_IMPORT
void (*set_new_handler(void (*)(void)))(void);

DECLSPEC_IMPORT
void (*set_terminate(void (*)(void)))(void);

DECLSPEC_IMPORT
void (*set_unexpected(void (*)(void)))(void);

DECLSPEC_IMPORT
void terminate(void);

DECLSPEC_IMPORT
void unexpected(void);

DECLSPEC_IMPORT
public: virtual char const * exception::what(void)const ;

DECLSPEC_IMPORT
void __ExceptionPtrAssign(void *,void const *);

DECLSPEC_IMPORT
bool __ExceptionPtrCompare(void const *,void const *);

DECLSPEC_IMPORT
void __ExceptionPtrCopy(void *,void const *);

DECLSPEC_IMPORT
void __ExceptionPtrCopyException(void *,void const *,void const *);

DECLSPEC_IMPORT
void __ExceptionPtrCreate(void *);

DECLSPEC_IMPORT
void __ExceptionPtrCurrentException(void *);

DECLSPEC_IMPORT
void __ExceptionPtrDestroy(void *);

DECLSPEC_IMPORT
void __ExceptionPtrRethrow(void const *);

DECLSPEC_IMPORT
void __ExceptionPtrSwap(void *,void *);

DECLSPEC_IMPORT
bool __ExceptionPtrToBool(void const *);

DECLSPEC_IMPORT
bool __uncaught_exception(void);

