#
# 'make depend' uses makedepend to automatically generate dependencies
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = g++

# define any compile-time flags
CFLAGS = -Wall -g -pipe  -std=c++11 -ffast-math -mfloat-abi=hard -mfpu=neon-vfpv4 -Os -mtune=cortex-a7
# -march=armv7-a 

# define any directories containing header files other than /usr/include
#
INCLUDES = -I/usr/local/include/opencv -I/usr/local/include

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS =

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -ljsoncpp -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lpthread

# define the C source files
SRCS = cvProcessor.cpp parkingConfig.cpp

# define the C object files
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

# define the executable file
MAIN = exec
PACKAGE = parking

#
# The following part of the makefile is generic; it can be used to
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)
		@echo  cProcessor has been compiled

$(MAIN): $(OBJS)
		$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file)
# (see the gnu make manual section about automatic variables)
.c.o:
		$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
		$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
		makedepend $(INCLUDES) $^

install:
		sudo pkill $(MAIN) &
		sleep 1
		sudo cp $(MAIN) /home/camera/

start:
		sudo service rc.local start

stop:
		sudo pkill $(MAIN)

deb:		$(MAIN) $(PACKAGE)
		cp $(MAIN) $(PACKAGE)/usr/bin/$(PACKAGE)
		cp config.json $(PACKAGE)/etc/$(PACKAGE)/$(PACKAGE).conf
		cp getStream.py $(PACKAGE)/etc/$(PACKAGE)/
		fakeroot dpkg-deb --build $(PACKAGE)
		mv $(PACKAGE).deb $(PACKAGE)_latest_armhf.deb

# DO NOT DELETE THIS LINE -- make depend needs it
