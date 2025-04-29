# Masm heap
The masm heap allows for allocating and freeing memory with `MALLOC (ptr) (size)` and `FREE (success) (ptr)`

## MALLOC (ptr) (size)
The malloc instruction allows for allocating (size) bytes in memory

size - Number of bytes
ptr - where the data is

ptr can also be a error code (err codes are negative)
The error codes are as following

Name                  | value | description
----------------------|-------|----------------
HEAP_ERR_OUT_OF_SPACE | -3    | Not enough space to allocate size bytes
HEAP_ERR_INVALID_ARG  | -4    | Size = 0

## FREE (success) (ptr)
The free instruction allows for freeing memory that has been previously allocated with MALLOC

success - Error code or zero if success
ptr - pointer to data that should be freed

Name                   | value | description
-----------------------|-------|----------------
HEAP_ERR_ALREADY_FREE  | -1    | Tried to free a already free chunk
HEAP_ERR_NOT_ALLOCATED | -2    | Tried to free data that has never been allocated with MALLOC

## Example
```
lbl main
MALLOC rax 15 ; allocate 14 bytes
CMP rax 0 ; Err codes are negative
jl #error

MOVTO rax 0 72   ; H
MOVTO rax 1 101  ; e
MOVTO rax 2 108  ; l
MOVTO rax 3 108  ; l
MOVTO rax 4 111  ; o
MOVTO rax 5 44   ; ,
MOVTO rax 6 32   ;  
MOVTO rax 7 87   ; W
MOVTO rax 8 111  ; o
MOVTO rax 9 114  ; r
MOVTO rax 10 108 ; l
MOVTO rax 11 100 ; d
MOVTO rax 12 33  ; !
MOVTO rax 13 10  ; \n
MOVTO rax 14 0   ; null terminator

out 1 $rax

FREE rax rax ; Free the 15 bytes and set rax to zero (if free success else rax = the error code)

hlt

lbl error
DB $100 "Error while allocating memory: "
out 1 $100
out 1 rax
cout 1 10 ; \n