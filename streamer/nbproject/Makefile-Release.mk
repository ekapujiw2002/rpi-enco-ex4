#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/dictionary.o \
	${OBJECTDIR}/enstreamer.o \
	${OBJECTDIR}/iniparser.o \
	${OBJECTDIR}/thpool.o


# C Compiler Flags
CFLAGS=-Os

# CC Compiler Flags
CCFLAGS=-Os
CXXFLAGS=-Os

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lcrypto -lpthread

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/enstreamer

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/enstreamer: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/enstreamer ${OBJECTFILES} ${LDLIBSOPTIONS} -Os

${OBJECTDIR}/dictionary.o: dictionary.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dictionary.o dictionary.c

${OBJECTDIR}/enstreamer.o: enstreamer.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/enstreamer.o enstreamer.c

${OBJECTDIR}/iniparser.o: iniparser.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/iniparser.o iniparser.c

${OBJECTDIR}/thpool.o: thpool.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/thpool.o thpool.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/enstreamer

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
