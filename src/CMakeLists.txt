
set (
    BASE_SOURCE
    Saba/Base/File.cpp
    Saba/Base/Log.cpp
    Saba/Base/Path.cpp
    Saba/Base/Singleton.cpp
    Saba/Base/Time.cpp
    Saba/Base/UnicodeUtil.cpp
)
set (
    BASE_HEADER
    Saba/Base/File.h
    Saba/Base/Log.h
    Saba/Base/Path.h
    Saba/Base/Singleton.h
    Saba/Base/Time.h
    Saba/Base/UnicodeUtil.h
)

set (
    MODEL_OBJ_SOURCE
    Saba/Model/OBJ/OBJModel.cpp
    Saba/Model/OBJ/tinyobjloader.cpp
)
set (
    MODEL_OBJ_HEADER
    Saba/Model/OBJ/OBJModel.h
)

set (
    MODEL_MMD_SOURCE
    Saba/Model/MMD/MMDIkSolver.cpp
    Saba/Model/MMD/MMDMaterial.cpp
    Saba/Model/MMD/MMDModel.cpp
    Saba/Model/MMD/MMDMorph.cpp
    Saba/Model/MMD/MMDNode.cpp
    Saba/Model/MMD/MMDPhysics.cpp
    Saba/Model/MMD/PMDFile.cpp
    Saba/Model/MMD/PMDModel.cpp
    Saba/Model/MMD/PMXFile.cpp
    Saba/Model/MMD/PMXModel.cpp
    Saba/Model/MMD/SjisToUnicode.cpp
    Saba/Model/MMD/VMDAnimation.cpp
    Saba/Model/MMD/VMDFile.cpp
)
set (
    MODEL_MMD_HEADER
    Saba/Model/MMD/MMDFileString.h
    Saba/Model/MMD/MMDIkSolver.h
    Saba/Model/MMD/MMDMaterial.h
    Saba/Model/MMD/MMDModel.h
    Saba/Model/MMD/MMDMorph.h
    Saba/Model/MMD/MMDNode.h
    Saba/Model/MMD/MMDPhysics.h
    Saba/Model/MMD/PMDFile.h
    Saba/Model/MMD/PMDModel.h
    Saba/Model/MMD/PMXFile.h
    Saba/Model/MMD/PMXModel.h
    Saba/Model/MMD/SjisToUnicode.h
    Saba/Model/MMD/VMDAnimation.h
    Saba/Model/MMD/VMDFile.h
)

add_library(
    Saba
    ${BASE_SOURCE}
    ${BASE_HEADER}
    ${MODEL_OBJ_SOURCE}
    ${MODEL_OBJ_HEADER}
    ${MODEL_MMD_SOURCE}
    ${MODEL_MMD_HEADER}
)
target_link_libraries(Saba ${BULLET_LIBRARIES})

SOURCE_GROUP(Base FILES ${BASE_SOURCE} ${BASE_HEADER})
SOURCE_GROUP(Model\\OBJ FILES ${MODEL_OBJ_SOURCE} ${MODEL_OBJ_HEADER})
SOURCE_GROUP(Model\\MMD FILES ${MODEL_MMD_SOURCE} ${MODEL_MMD_HEADER})
