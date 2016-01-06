set(SOURCES cximage.cpp
            DDSImage.cpp
            DirectXGraphics.cpp
            DirtyRegionSolvers.cpp
            DirtyRegionTracker.cpp
            FFmpegImage.cpp
            FrameBufferObject.cpp
            GraphicContext.cpp
            GUIAction.cpp
            GUIAudioManager.cpp
            GUIBaseContainer.cpp
            GUIBorderedImage.cpp
            GUIButtonControl.cpp
            GUICheckMarkControl.cpp
            GUIColorManager.cpp
            GUIControl.cpp
            GUIControlFactory.cpp
            GUIControlGroup.cpp
            GUIControlGroupList.cpp
            GUIControlProfiler.cpp
            GUIDialog.cpp
            GUIEditControl.cpp
            GUIFadeLabelControl.cpp
            GUIFixedListContainer.cpp
            GUIFont.cpp
            GUIFontCache.cpp
            GUIFontManager.cpp
            GUIFontTTF.cpp
            GUIImage.cpp
            GUIIncludes.cpp
            GUIInfoTypes.cpp
            GUIKeyboardFactory.cpp
            GUILabelControl.cpp
            GUILabel.cpp
            GUIListContainer.cpp
            GUIListGroup.cpp
            GUIListItem.cpp
            GUIListItemLayout.cpp
            GUIListLabel.cpp
            GUIMessage.cpp
            GUIMoverControl.cpp
            GUIMultiImage.cpp
            GUIMultiSelectText.cpp
            GUIPanelContainer.cpp
            GUIProgressControl.cpp
            GUIRadioButtonControl.cpp
            GUIRenderingControl.cpp
            GUIResizeControl.cpp
            GUIRSSControl.cpp
            GUIScrollBarControl.cpp
            GUISelectButtonControl.cpp
            GUISettingsSliderControl.cpp
            GUISliderControl.cpp
            GUISpinControl.cpp
            GUISpinControlEx.cpp
            GUIStaticItem.cpp
            GUITextBox.cpp
            GUITextLayout.cpp
            GUITexture.cpp
            GUIToggleButtonControl.cpp
            GUIVideoControl.cpp
            GUIVisualisationControl.cpp
            GUIWindow.cpp
            GUIWindowManager.cpp
            GUIWrappingListContainer.cpp
            imagefactory.cpp
            IWindowManagerCallback.cpp
            JpegIO.cpp
            LocalizeStrings.cpp
            Resolution.cpp
            Shader.cpp
            StereoscopicsManager.cpp
            TextureBundle.cpp
            TextureBundleXBT.cpp
            TextureBundleXPR.cpp
            Texture.cpp
            TextureManager.cpp
            VisibleEffect.cpp
            XBTF.cpp
            XBTFReader.cpp)

if(OPENGL_FOUND)
  list(APPEND SOURCES TextureGL.cpp
                      MatrixGLES.cpp
                      GUIFontTTFGL.cpp
                      GUITextureGL.cpp)
endif()

if(GLES_FOUND)
  list(APPEND SOURCES TextureGL.cpp
                      TexturePi.cpp
                      GUIFontTTFGL.cpp
                      GUITextureGLES.cpp
                      MatrixGLES.cpp
                      GUIShader.cpp)
endif()

if(GIF_FOUND)
  list(APPEND SOURCES Gif.cpp)
endif()

if(WIN32)
  list(APPEND SOURCES D3DResource.cpp
                      GUIFontTTFDX.cpp
                      GUITextureD3D.cpp
                      GUIShaderDX.cpp
                      TextureDX.cpp)
endif()

core_add_library(guilib)
add_dependencies(guilib libcpluff ffmpeg)

if(WIN32)
  set(SHADERS_VERTEX guishader_vert.hlsl)
  set(SHADERS_PIXEL guishader_checkerboard_right.hlsl
                    guishader_checkerboard_left.hlsl
                    guishader_default.hlsl
                    guishader_fonts.hlsl
                    guishader_interlaced_right.hlsl
                    guishader_interlaced_left.hlsl
                    guishader_multi_texture_blend.hlsl
                    guishader_texture.hlsl
                    guishader_texture_noblend.hlsl
                    guishader_video.hlsl
                    guishader_video_control.hlsl)
  foreach(shader ${SHADERS_VERTEX})
    add_shader_dx(guilib
                  ${CORE_SOURCE_DIR}/system/shaders/${shader}
                  vs_4_0_level_9_1
                  VS)
  endforeach()
  foreach(shader ${SHADERS_PIXEL})
    add_shader_dx(guilib
                  ${CORE_SOURCE_DIR}/system/shaders/${shader}
                  ps_4_0_level_9_1
                  PS)
  endforeach()
endif()
