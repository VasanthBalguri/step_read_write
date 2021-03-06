cmake_minimum_required(VERSION 3.1.0)

project(step_read_write)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)
find_package(OCE REQUIRED)
FIND_PACKAGE( OpenSceneGraph COMPONENTS osgDB osgGA osgViewer osg osgUtil osgText)

include_directories(${OCE_INCLUDE_DIRS})

add_executable(step_read_write
    mainwindow.ui
    qtosgwidget.cpp
    mainwindow.cpp
    ocefeature.cpp
    main.cpp
)

target_link_libraries(step_read_write Qt5::Widgets ${OPENSCENEGRAPH_LIBRARIES} FWOSPlugin PTKernel TKBinL TKBin TKBinTObj TKBinXCAF TKBool TKBO TKBRep TKCAF TKCDF TKernel TKFeat TKFillet TKGeomAlgo TKGeomBase TKG2d TKG3d TKHLR TKIGES TKLCAF TKMath TKMesh TKMeshVS TKNIS TKOffset TKOpenGl TKPCAF TKPLCAF TKPrim TKPShape TKService TKShapeSchema TKShHealing TKStdLSchema TKStdSchema TKSTEPAttr TKSTEPBase TKSTEP TKSTEP209 TKSTL TKTObj TKTopAlgo TKVoxel TKVRML TKV3d TKXCAFSchema TKXCAF TKXDEIGES TKXDESTEP TKXMesh TKXmlL TKXml TKXmlTObj TKXmlXCAF TKXSBase)
