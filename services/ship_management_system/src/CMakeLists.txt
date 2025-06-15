set(DB_CONNECTION_STRING "host=db port=5432 dbname=postgres user=postgres password=test")

configure_file(${CMAKE_SOURCE_DIR}/include/config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_BINARY_DIR})

set(SRC_FILES
    main.cpp
    admiral_handler.cpp
    captain_handler.cpp
    storage.cpp
    jwt.cpp
    user_handler.cpp
    utils.cpp
    global.cpp
)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${EXECUTABLE_NAME} ${SRC_FILES})
target_include_directories(${EXECUTABLE_NAME}
                           PRIVATE 
                           ${CROW_INCLUDE_DIR}
                           ${Boost_INCLUDE_DIRS}
)
target_link_libraries(${EXECUTABLE_NAME} ${CROW_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} -lpqxx -lpq -lbcrypt)
target_link_libraries(${EXECUTABLE_NAME} jwt-cpp::jwt-cpp)
target_link_libraries(${EXECUTABLE_NAME} dl pthread)


install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)