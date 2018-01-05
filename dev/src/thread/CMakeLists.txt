cmake_minimum_required(VERSION 3.2)
project(TLG)
set(THIS ${CMAKE_CURRENT_LIST_DIR})

# ----------------------------- ADD SUBDIRECTORIES -----------------------------

# -------------------------- ADD SOURCES IN DIRECTORY --------------------------
set(SRC_FILES 
	"${SRC_FILES}"
		
	${THIS}/thread_pool.cc
	${THIS}/thread_reservoir.cc
		
	PARENT_SCOPE)

# -------------------------- ADD HEADERS IN DIRECTORY --------------------------
set(HDR_FILES 
	"${HDR_FILES}"
	
	${THIS}/thread_pool.h
	${THIS}/thread_reservoir.h
	
	PARENT_SCOPE)