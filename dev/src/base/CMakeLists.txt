cmake_minimum_required(VERSION 3.2)
project(TLG)
set(THIS ${CMAKE_CURRENT_LIST_DIR})

# ----------------------------- ADD SUBDIRECTORIES -----------------------------

# -------------------------- ADD SOURCES IN DIRECTORY --------------------------
set(SRC_FILES 
	"${SRC_FILES}"
		
	PARENT_SCOPE)

# -------------------------- ADD HEADERS IN DIRECTORY --------------------------
set(HDR_FILES 
	"${HDR_FILES}"
	
	${THIS}/abort.h
	${THIS}/platform.h
	${THIS}/static_type_assert.h
	
	PARENT_SCOPE)