
set(FTGL_SRC "./ftgl/src")

set(libftgl_la_SOURCES
    ${FTGL_SRC}/FTBuffer.cpp
    ${FTGL_SRC}/FTCharmap.cpp
    ${FTGL_SRC}/FTCharmap.h
    ${FTGL_SRC}/FTCharToGlyphIndexMap.h
    ${FTGL_SRC}/FTCleanup.cpp
    ${FTGL_SRC}/FTCleanup.h
    ${FTGL_SRC}/FTContour.cpp
    ${FTGL_SRC}/FTContour.h
    ${FTGL_SRC}/FTFace.cpp
    ${FTGL_SRC}/FTFace.h
    ${FTGL_SRC}/FTGL.cpp
    ${FTGL_SRC}/FTGlyphContainer.cpp
    ${FTGL_SRC}/FTGlyphContainer.h
    ${FTGL_SRC}/FTInternals.h
    ${FTGL_SRC}/FTLibrary.cpp
    ${FTGL_SRC}/FTLibrary.h
    ${FTGL_SRC}/FTList.h
    ${FTGL_SRC}/FTPoint.cpp
    ${FTGL_SRC}/FTSize.cpp
    ${FTGL_SRC}/FTSize.h
    ${FTGL_SRC}/FTVector.h
    ${FTGL_SRC}/FTVectoriser.cpp
    ${FTGL_SRC}/FTVectoriser.h
    ${FTGL_SRC}/FTUnicode.h
)

set(ftgl_headers
    ${FTGL_SRC}/FTGL/ftgl.h
    ${FTGL_SRC}/FTGL/FTBBox.h
    ${FTGL_SRC}/FTGL/FTBuffer.h
    ${FTGL_SRC}/FTGL/FTPoint.h
    ${FTGL_SRC}/FTGL/FTGlyph.h
    ${FTGL_SRC}/FTGL/FTBitmapGlyph.h
    ${FTGL_SRC}/FTGL/FTBufferGlyph.h
    ${FTGL_SRC}/FTGL/FTExtrdGlyph.h
    ${FTGL_SRC}/FTGL/FTOutlineGlyph.h
    ${FTGL_SRC}/FTGL/FTPixmapGlyph.h
    ${FTGL_SRC}/FTGL/FTPolyGlyph.h
    ${FTGL_SRC}/FTGL/FTTextureGlyph.h
    ${FTGL_SRC}/FTGL/FTFont.h
    ${FTGL_SRC}/FTGL/FTGLBitmapFont.h
    ${FTGL_SRC}/FTGL/FTBufferFont.h
    ${FTGL_SRC}/FTGL/FTGLExtrdFont.h
    ${FTGL_SRC}/FTGL/FTGLOutlineFont.h
    ${FTGL_SRC}/FTGL/FTGLPixmapFont.h
    ${FTGL_SRC}/FTGL/FTGLPolygonFont.h
    ${FTGL_SRC}/FTGL/FTGLTextureFont.h
    ${FTGL_SRC}/FTGL/FTGLTriangleExtractorFont.h
    ${FTGL_SRC}/FTGL/FTLayout.h
    ${FTGL_SRC}/FTGL/FTSimpleLayout.h
)

set(ftglyph_sources
    ${FTGL_SRC}/FTGlyph/FTGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTGlyphGlue.cpp
    ${FTGL_SRC}/FTGlyph/FTBitmapGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTBitmapGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTBufferGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTBufferGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTExtrudeGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTExtrudeGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTOutlineGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTOutlineGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTPixmapGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTPixmapGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTPolygonGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTPolygonGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTTextureGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTTextureGlyphImpl.h
    ${FTGL_SRC}/FTGlyph/FTTriangleExtractorGlyph.cpp
    ${FTGL_SRC}/FTGlyph/FTTriangleExtractorGlyphImpl.h
)

set(ftfont_sources
    ${FTGL_SRC}/FTFont/FTFont.cpp
    ${FTGL_SRC}/FTFont/FTFontImpl.h
    ${FTGL_SRC}/FTFont/FTFontGlue.cpp
    ${FTGL_SRC}/FTFont/FTBitmapFont.cpp
    ${FTGL_SRC}/FTFont/FTBitmapFontImpl.h
    ${FTGL_SRC}/FTFont/FTBufferFont.cpp
    ${FTGL_SRC}/FTFont/FTBufferFontImpl.h
    ${FTGL_SRC}/FTFont/FTExtrudeFont.cpp
    ${FTGL_SRC}/FTFont/FTExtrudeFontImpl.h
    ${FTGL_SRC}/FTFont/FTOutlineFont.cpp
    ${FTGL_SRC}/FTFont/FTOutlineFontImpl.h
    ${FTGL_SRC}/FTFont/FTPixmapFont.cpp
    ${FTGL_SRC}/FTFont/FTPixmapFontImpl.h
    ${FTGL_SRC}/FTFont/FTPolygonFont.cpp
    ${FTGL_SRC}/FTFont/FTPolygonFontImpl.h
    ${FTGL_SRC}/FTFont/FTTextureFont.cpp
    ${FTGL_SRC}/FTFont/FTTextureFontImpl.h
    ${FTGL_SRC}/FTFont/FTTriangleExtractorFont.cpp
    ${FTGL_SRC}/FTFont/FTTriangleExtractorFontImpl.h
)

set(ftlayout_sources
    ${FTGL_SRC}/FTLayout/FTLayout.cpp
    ${FTGL_SRC}/FTLayout/FTLayoutImpl.h
    ${FTGL_SRC}/FTLayout/FTLayoutGlue.cpp
    ${FTGL_SRC}/FTLayout/FTSimpleLayout.cpp
    ${FTGL_SRC}/FTLayout/FTSimpleLayoutImpl.h
)

add_compile_options(-Wno-deprecated -Wno-conversion)

add_library(ftgl STATIC ${libftgl_la_SOURCES} ${ftgl_headers} ${ftglyph_sources} ${ftfont_sources} ${ftlayout_sources})

target_compile_definitions(ftgl PRIVATE PACKAGE_VERSION="2.3.14")
target_compile_definitions(ftgl PRIVATE __FUNC__="func")

if(WIN32)
	target_compile_definitions(ftgl PRIVATE _USE_MATH_DEFINES=1 GL_SILENCE_DEPRECATION=1)
endif()
