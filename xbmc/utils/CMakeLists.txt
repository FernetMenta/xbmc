set(SOURCES ActorProtocol.cpp
            AlarmClock.cpp
            AliasShortcutUtils.cpp
            Archive.cpp
            AsyncFileCopy.cpp
            AutoPtrHandle.cpp
            auto_buffer.cpp
            Base64.cpp
            BitstreamConverter.cpp
            BitstreamStats.cpp
            BooleanLogic.cpp
            CharsetConverter.cpp 
            CharsetDetection.cpp
            CPUInfo.cpp
            Crc32.cpp
            CryptThreading.cpp
            DatabaseUtils.cpp
            EndianSwap.cpp
            Environment.cpp
            Fanart.cpp
            FileOperationJob.cpp
            FileUtils.cpp
            fstrcmp.c
            GLUtils.cpp
            GroupUtils.cpp
            HTMLUtil.cpp
            HttpHeader.cpp
            HttpParser.cpp
            HttpRangeUtils.cpp
            HttpResponse.cpp
            InfoLoader.cpp
            JobManager.cpp
            JSONVariantParser.cpp
            JSONVariantWriter.cpp
            LabelFormatter.cpp
            LangCodeExpander.cpp
            LegacyPathTranslation.cpp
            Locale.cpp
            log.cpp
            md5.cpp
            Mime.cpp
            Observer.cpp
            PerformanceSample.cpp
            PerformanceStats.cpp
            POUtils.cpp
            RecentlyAddedJob.cpp
            RegExp.cpp
            rfft.cpp
            RingBuffer.cpp
            RssManager.cpp
            RssReader.cpp
            ProgressJob.cpp
            SaveFileStateJob.cpp
            ScraperParser.cpp
            ScraperUrl.cpp
            Screenshot.cpp
            SeekHandler.cpp
            SortUtils.cpp
            Speed.cpp
            Splash.cpp
            Stopwatch.cpp
            StreamDetails.cpp
            StreamUtils.cpp
            StringUtils.cpp
            StringValidation.cpp
            SystemInfo.cpp
            Temperature.cpp
            TextSearch.cpp
            TimeSmoother.cpp
            TimeUtils.cpp
            URIUtils.cpp
            UrlOptions.cpp
            Utf8Utils.cpp
            Variant.cpp
            Vector.cpp
            Weather.cpp
            XBMCTinyXML.cpp
            XMLUtils.cpp
            XSLTUtils.cpp)


# The large map trips the clang optimizer
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set_source_files_properties(Mime.cpp PROPERTIES COMPILE_FLAGS -O0)
endif()

core_add_library(utils)
add_dependencies(utils libcpluff ffmpeg crossguid)
