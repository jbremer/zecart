CL = cl
CXX = g++

SOURCES = $(wildcard *.cpp)

ifeq ($(OS),Windows_NT)
	OBJECTS86 = $(patsubst %.cpp, %-x86.obj, $(SOURCES))
	PINTOOLS = zecart-x86.dll
else
	OBJECTS86 = $(patsubst %.cpp, %-x86.o, $(SOURCES))
	OBJECTS64 = $(patsubst %.cpp, %-x64.o, $(SOURCES))
	PINTOOLS = zecart-x86.so zecart-x64.so
endif

PINTOOL = ../pintool
LINK = C:/ProgFiles86/Microsoft\ Visual\ Studio\ 10.0/VC/Bin/link.exe

default: $(OBJECTS86) $(OBJECTS64) $(PINTOOLS)

%-x86.obj: %.cpp
	$(CL) /c /MT /EHs- /EHa- /wd4530 /DTARGET_WINDOWS \
		/DBIGARRAY_MULTIPLIER=1 /DUSING_XED /D_CRT_SECURE_NO_DEPRECATE  \
		/D_SECURE_SCL=0 /nologo /Gy /O2 /DTARGET_IA32 /DHOST_IA32 \
		/I$(PINTOOL)\source\include /I$(PINTOOL)\source\include\gen \
		/I$(PINTOOL)\source\tools\InstLib \
		/I$(PINTOOL)\extras\xed2-ia32\include \
		/I$(PINTOOL)\extras\components\include $^ /Fo$@

%-x86.dll: %-x86.obj $(OBJECTS86)
	$(LINK) /DLL /EXPORT:main /NODEFAULTLIB /NOLOGO /INCREMENTAL:NO /OPT:REF \
		/MACHINE:x86 /ENTRY:Ptrace_DllMainCRTStartup@12 /BASE:0x55000000 \
		/LIBPATH:$(PINTOOL)\ia32\lib /LIBPATH:$(PINTOOL)\ia32\lib-ext \
		/LIBPATH:$(PINTOOL)\extras\xed2-ia32\lib /OUT:$@ $^ pin.lib \
		libxed.lib libcpmt.lib libcmt.lib pinvm.lib kernel32.lib ntdll-32.lib

%-x86.o: %.cpp
	$(CXX) -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -Wall -Werror -m32 \
		-Wno-unknown-pragmas -fno-stack-protector -DTARGET_IA32 \
		-DHOST_IA32 -fPIC -DTARGET_LINUX \
		-I$(PINTOOL)/source/include/pin \
		-I$(PINTOOL)/source/include/pin/gen \
		-I$(PINTOOL)/extras/components/include \
		-I$(PINTOOL)/extras/xed2-ia32/include \
		-I$(PINTOOL)/source/tools/InstLib \
		-O3 -fomit-frame-pointer -fno-strict-aliasing -c -o $@ $^

%-x86.so: $(OBJECTS86)
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic -m32 \
		-Wl,--version-script=$(PINTOOL)/source/include/pin/pintool.ver \
		-o $@ $^ -L$(PINTOOL)/ia32/lib -L$(PINTOOL)/ia32/lib-ext \
		-L$(PINTOOL)/ia32/runtime/glibc \
		-L$(PINTOOL)/extras/xed2-ia32/lib \
		-lpin -lxed -ldwarf -lelf -ldl

%-x64.o: %.cpp
	$(CXX) -DBIGARRAY_MULTIPLIER=1 -DUSING_XED -Wall -Werror \
		-Wno-unknown-pragmas -fno-stack-protector -DTARGET_IA32E \
		-DHOST_IA32E -fPIC -DTARGET_LINUX \
		-I$(PINTOOL)/source/include/pin \
		-I$(PINTOOL)/source/include/pin/gen \
		-I$(PINTOOL)/extras/components/include \
		-I$(PINTOOL)/extras/xed2-intel64/include \
		-I$(PINTOOL)/source/tools/InstLib \
		-O3 -fomit-frame-pointer -fno-strict-aliasing -c -o $@ $^

%-x64.so: $(OBJECTS64)
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
		-Wl,--version-script=$(PINTOOL)/source/include/pin/pintool.ver \
		-o $@ $^ -L$(PINTOOL)/intel64/lib -L$(PINTOOL)/intel64/lib-ext \
		-L$(PINTOOL)/intel64/runtime/glibc \
		-L$(PINTOOL)/extras/xed2-intel64/lib \
		-lpin -lxed -ldwarf -lelf -ldl

test:
	make -C tests test

clean:
	rm -f '*.exp' '*.lib' $(OBJECTS86) $(OBJECTS64) $(PINTOOLS)
