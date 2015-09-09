set(SOURCES DVDFactorySubtitle.cpp
            DVDSubtitleLineCollection.cpp
            DVDSubtitleParserMicroDVD.cpp
            DVDSubtitleParserMPL2.cpp
            DVDSubtitleParserSami.cpp
            DVDSubtitleParserSubrip.cpp
            DVDSubtitleParserVplayer.cpp
            DVDSubtitleStream.cpp
            DVDSubtitlesLibass.cpp
            DVDSubtitleParserSSA.cpp
            DVDSubtitleTagMicroDVD.cpp
            DVDSubtitleTagSami.cpp)

core_add_library(dvdsubtitles)
add_dependencies(dvdsubtitles ffmpeg)
