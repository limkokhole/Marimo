#ifndef _GRAPX_TEXTURE_H_
#define _GRAPX_TEXTURE_H_

class GXGraphics;
class GTextureBase : public GResource
{
public:
  GTextureBase(GXUINT nPriority, GXDWORD dwType) : GResource(0, dwType){}
public:
  GXSTDINTERFACE(GXHRESULT    AddRef            ());
  GXSTDINTERFACE(GXHRESULT    Release           ());
public:
  GXSTDINTERFACE(GXDWORD      GetUsage          ());
  GXSTDINTERFACE(GXFormat     GetFormat         ());
  GXSTDINTERFACE(GXVOID       GenerateMipMaps   ());
  GXSTDINTERFACE(GXGraphics*  GetGraphicsUnsafe ());      // 不会增加引用计数
  GXSTDINTERFACE(GXBOOL       SaveToFileW       (GXLPCWSTR szFileName, GXLPCSTR szDestFormat));
};

class GTexture : public GTextureBase
{
public:
  typedef struct __tagLOCKEDRECT
  {
    GXINT     Pitch;
    GXLPVOID  pBits;
  }LOCKEDRECT, *LPLOCKEDRECT;

public:
  GTexture() : GTextureBase(0, RESTYPE_TEXTURE2D){}
  GXSTDINTERFACE(GXHRESULT    AddRef            ());
  GXSTDINTERFACE(GXHRESULT    Release           ());

  GXSTDINTERFACE(GXBOOL       Clear             (GXCONST GXLPRECT lpRect, GXCOLOR dwColor));  // 实现不同, 建议不要在运行时随意使用!
  GXSTDINTERFACE(GXBOOL       GetRatio          (GXINT* pWidthRatio, GXINT* pHeightRatio));   // 去屏幕比例,如果是屏幕对齐纹理,返回负值的比率,否则返回纹理尺寸,如果从文件读取的纹理是原始文件的大小
  GXSTDINTERFACE(GXUINT       GetWidth          ());    // 取m_nWidth成员的值
  GXSTDINTERFACE(GXUINT       GetHeight         ());    // 取m_nHeight成员的值, 参考GetWidth()
  GXSTDINTERFACE(GXBOOL       GetDimension      (GXUINT* pWidth, GXUINT* pHeight));  // 取纹理的尺寸,这个值可能会跟屏幕尺寸变化
  //GXSTDINTERFACE(GXDWORD      GetUsage          ());
  //GXSTDINTERFACE(GXFormat     GetFormat         ());
  //GXSTDINTERFACE(GXVOID       GenerateMipMaps   ());
  GXSTDINTERFACE(GXBOOL       GetDesc           (GXBITMAP*lpBitmap));
  GXSTDINTERFACE(GXBOOL       CopyRect          (GTexture* pSrc, GXLPCRECT lprcSource, GXLPCPOINT lpptDestination));
  GXSTDINTERFACE(GXBOOL       StretchRect       (GTexture* pSrc, GXLPCRECT lpDest, GXLPCRECT lpSrc, GXTextureFilterType eFilter));
  GXSTDINTERFACE(GXBOOL       LockRect          (LPLOCKEDRECT lpLockRect, GXLPCRECT lpRect, GXDWORD Flags)); // TODO: 考虑以后是不是不要用lock, 用外围的接口代替
  GXSTDINTERFACE(GXBOOL       UnlockRect        ());
  //GXSTDINTERFACE(GXGraphics*  GetGraphicsUnsafe ());      // 不会增加引用计数
                                              
  //GXSTDINTERFACE(GXBOOL       SaveToFileW       (GXLPCWSTR szFileName, GXLPCSTR szDestFormat));
};

class GTexture3D : public GTextureBase
{
public:
  typedef struct __tagLOCKEDBOX
  {
    GXINT     RowPitch;
    GXINT     SlicePitch;
    GXLPVOID  pBits;
  }LOCKEDBOX, *LPLOCKEDBOX;

  typedef struct __tagBOX {
    GXUINT Left;
    GXUINT Top;
    GXUINT Right;
    GXUINT Bottom;
    GXUINT Front;
    GXUINT Back;
  } BOX, *LPBOX;


public:
  GTexture3D() : GTextureBase(0, RESTYPE_TEXTURE3D){}

  GXSTDINTERFACE(GXHRESULT    AddRef            ());
  GXSTDINTERFACE(GXHRESULT    Release           ());

  GXSTDINTERFACE(GXBOOL       Clear             (GXCONST LPBOX lpRect, GXCOLOR dwColor));
  GXSTDINTERFACE(GXUINT       GetWidth          ());
  GXSTDINTERFACE(GXUINT       GetHeight         ());
  GXSTDINTERFACE(GXUINT       GetDepth          ());
  GXSTDINTERFACE(GXBOOL       GetDimension      (GXUINT* pWidth, GXUINT* pHeight, GXUINT* pDepth));
  //GXSTDINTERFACE(GXDWORD      GetUsage          ());
  //GXSTDINTERFACE(GXFormat     GetFormat         ());
  //GXSTDINTERFACE(GXVOID       GenerateMipMaps   ());
  GXSTDINTERFACE(GXBOOL       CopyBox           (GTexture3D* pSrc, GXCONST LPBOX lprcSource, GXUINT x, GXUINT y, GXUINT z));
  GXSTDINTERFACE(GXBOOL       LockBox           (LPLOCKEDBOX lpLockRect, GXCONST LPBOX lpBox, GXDWORD Flags)); // TODO: 考虑以后是不是不要用lock, 用外围的接口代替
  GXSTDINTERFACE(GXBOOL       UnlockBox         ());
  //GXSTDINTERFACE(GXGraphics*  GetGraphicsUnsafe ());      // 不会增加引用计数
  //GXSTDINTERFACE(GXBOOL       SaveToFileW       (GXLPCWSTR szFileName, GXLPCSTR szDestFormat));
};

class GTextureCube : public GTextureBase
{
public:
  typedef struct __tagLOCKEDRECT
  {
    GXINT     Pitch;
    GXLPVOID  pBits;
  }LOCKEDRECT, *LPLOCKEDRECT;

public:
  GTextureCube() : GTextureBase(0, RESTYPE_TEXTURE_CUBE){}

  GXSTDINTERFACE(GXHRESULT    AddRef            ());
  GXSTDINTERFACE(GXHRESULT    Release           ());

  GXSTDINTERFACE(GXBOOL       Clear             (GXCONST GXLPRECT lpRect, GXCOLOR dwColor));  // 实现不同, 建议不要在运行时随意使用!
  GXSTDINTERFACE(GXUINT       GetSize           ());    // 取m_nWidth成员的值
  GXSTDINTERFACE(GXDWORD      GetUsage          ());
  GXSTDINTERFACE(GXFormat     GetFormat         ());
  GXSTDINTERFACE(GXVOID       GenerateMipMaps   ());
  //GXSTDINTERFACE(GXBOOL       CopyRect          (GTexture* pSrc, GXLPCRECT lprcSource, GXLPCPOINT lpptDestination));
  //GXSTDINTERFACE(GXBOOL       StretchRect       (GTexture* pSrc, GXLPCRECT lpDest, GXLPCRECT lpSrc, GXTextureFilterType eFilter));
  //GXSTDINTERFACE(GXBOOL       LockRect          (LPLOCKEDRECT lpLockRect, GXLPCRECT lpRect, GXDWORD Flags)); // TODO: 考虑以后是不是不要用lock, 用外围的接口代替
  //GXSTDINTERFACE(GXBOOL       UnlockRect        ());
  GXSTDINTERFACE(GXGraphics*  GetGraphicsUnsafe ());      // 不会增加引用计数

  GXSTDINTERFACE(GXBOOL       SaveToFileW       (GXLPCWSTR pszFileName, GXLPCSTR pszDestFormat));
};

typedef GTexture* GLPTEXTURE;


#else
#pragma message(__FILE__": warning : Duplicate included this file.")
#endif // _GRAPX_TEXTURE_H_