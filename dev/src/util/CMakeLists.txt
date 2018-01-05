cmake_minimum_required(VERSION 3.2)
project(TLG)
set(THIS ${CMAKE_CURRENT_LIST_DIR})

# ----------------------------- ADD SUBDIRECTORIES -----------------------------

# -------------------------- ADD SOURCES IN DIRECTORY --------------------------
set(SRC_FILES 
	"${SRC_FILES}"
		
	${THIS}/bits.cc
	${THIS}/cannonical_errors.cc
	${THIS}/make_cleanup.cc
	${THIS}/status.cc
		
	PARENT_SCOPE)

# -------------------------- ADD HEADERS IN DIRECTORY --------------------------
set(HDR_FILES 
	"${HDR_FILES}"
	
	${THIS}/assert.h
	${THIS}/bits.h
	${THIS}/cannonical_errors.h
	${THIS}/deleterptr.h
	${THIS}/fixed_point.h
	${THIS}/loan.h
	${THIS}/make_cleanup.h
	${THIS}/noncopyable.h
	${THIS}/pstruct.h
	${THIS}/reflect.h
	${THIS}/status.h
	${THIS}/status_macros.h
	${THIS}/statusor.h	
	${THIS}/stopwatch.h
	${THIS}/strcat.h	
	${THIS}/tostring.h
		
	PARENT_SCOPE)