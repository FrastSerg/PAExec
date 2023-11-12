TARGET_NAME := paexec
MACRO_DEFS  := UNICODE _UNICODE
DEBUG       := noopt
#LDFLAGS     := /MANIFESTUAC:level=\'requireAdministrator\'\ uiAccess=\'false\' /MANIFEST:EMBED
#LDFLAGS     := /MANIFEST
CPPFLAGS   := /sdl

exe:
include ~/.VSSCBE/Makefile
