set(SOURCES GUIDialogPictureInfo.cpp
            GUIViewStatePictures.cpp
            GUIWindowPictures.cpp
            GUIWindowSlideShow.cpp
            Picture.cpp
            PictureInfoLoader.cpp
            PictureInfoTag.cpp
            PictureScalingAlgorithm.cpp
            PictureThumbLoader.cpp
            SlideShowPicture.cpp)

core_add_library(pictures)
add_dependencies(pictures ffmpeg)
