TARGET_NAME := paexec
MACRO_DEFS  := UNICODE _UNICODE
DEBUG       := noopt
LDFLAGS     := '/MANIFESTUAC:level=\"requireAdministrator\" /MANIFEST' '/MANIFESTUAC:uiAccess=\"true\"'

exe:
include ~/.VSSCBE/Makefile
