#include "clstd.h"
#include "clRepository.h"
#include "clString.h"
#include "clUtility.h"

#define KEYINCCOUNT 8
#define NAMEINCCOUNT 128
#define OCTETSIZE 8

namespace clstd
{
  namespace
  {
    enum HeaderFlag{
      HeaderFlag_KeyFieldMask     = 0x0003,
      HeaderFlag_KeyField_1Byte   = 0x0003, // 255 Keys
      HeaderFlag_KeyField_2Bytes  = 0x0002, // 65535 Keys
      HeaderFlag_KeyField_4Bytes  = 0x0000,

      HeaderFlag_NameFieldMask    = 0x000c,
      HeaderFlag_NameField_1Byte  = 0x000c, // 255 Bytes names buffer
      HeaderFlag_NameField_2Bytes = 0x0008,
      HeaderFlag_NameField_4Bytes = 0x0000,

      HeaderFlag_DataFieldMask    = 0x00010,
      HeaderFlag_DataField_64KB   = 0x00010, // 64K buffer
      HeaderFlag_DataField_4GB    = 0x00000, // 4G buffer
      
      HeaderFlag_AllFieldMask     = HeaderFlag_KeyFieldMask | HeaderFlag_NameFieldMask | HeaderFlag_DataFieldMask
    };

    struct FILE_HEADER
    {
      CLDWORD dwMagic; // CLRP
      CLDWORD dwFlags; // HeaderFlag
      CLDWORD nKeys;
      CLDWORD cbNames;
      CLDWORD cbData;
    };

    template<typename _key_t, typename _name_t, typename _data_t>
    struct FILE_HEADERT
    {
      CLDWORD dwMagic; // CLRP
      CLDWORD dwFlags; // HeaderFlag
      _key_t  nKeys;
      _name_t cbNames;
      _data_t cbData;

      size_t CopyFrom(const FILE_HEADER& h, CLDWORD dwAddFlags)
      {
        dwMagic = h.dwMagic;
        dwFlags = h.dwFlags | dwAddFlags;
        nKeys   = (_key_t)h.nKeys;
        cbNames = (_name_t)h.cbNames;
        cbData  = (_data_t)h.cbData;
        return sizeof(FILE_HEADERT);
      }

      size_t CopyTo(FILE_HEADER& h) const
      {
        h.dwMagic = dwMagic;
        h.dwFlags = dwFlags & (~HeaderFlag_AllFieldMask);
        h.nKeys   = (CLDWORD)nKeys;
        h.cbNames = (CLDWORD)cbNames;
        h.cbData  = (CLDWORD)cbData;
        return sizeof(FILE_HEADERT);
      }
    };



    template<typename _key_t, typename _name_t> 
    size_t _PackHeader_DataT(const FILE_HEADER& header, u32 dwFlags, CLBYTE* pBuffer)
    {
      if(header.cbData <= 0xffff) {
        typedef FILE_HEADERT<_key_t, _name_t, u16> HEADER_INST;
        HEADER_INST* packed_header = reinterpret_cast<HEADER_INST*>(pBuffer);
        return packed_header->CopyFrom(header, dwFlags | HeaderFlag_DataField_64KB);
      }
      else
      {
        typedef FILE_HEADERT<_key_t, _name_t, u32> HEADER_INST;
        HEADER_INST* packed_header = reinterpret_cast<HEADER_INST*>(pBuffer);
        return packed_header->CopyFrom(header, dwFlags | HeaderFlag_DataField_4GB);
      }
    }

    template<typename _key_t> 
    size_t _PackHeader_NameDataT(const FILE_HEADER& header, u32 dwFlags, CLBYTE* pBuffer)
    {
      if(header.cbNames <= 0xff) {
        return _PackHeader_DataT<_key_t, u8>(header, dwFlags | HeaderFlag_NameField_1Byte, pBuffer);
      } else if(header.cbNames <= 0xffff) {
        return _PackHeader_DataT<_key_t, u16>(header, dwFlags | HeaderFlag_NameField_2Bytes, pBuffer);
      }
      return _PackHeader_DataT<_key_t, u32>(header, dwFlags | HeaderFlag_NameField_4Bytes, pBuffer);
    }

    size_t _PackHeader( const FILE_HEADER& header, CLBYTE* pBuffer )
    {
      if(header.nKeys <= 0xff) {
        return _PackHeader_NameDataT<u8>(header, HeaderFlag_KeyField_1Byte, pBuffer);
      } else if(header.nKeys <= 0xffff) {
        return _PackHeader_NameDataT<u16>(header, HeaderFlag_KeyField_2Bytes, pBuffer);
      }
      return _PackHeader_NameDataT<u32>(header, HeaderFlag_KeyField_4Bytes, pBuffer);
    }

    //////////////////////////////////////////////////////////////////////////

    template<typename _key_t, typename _name_t> 
    size_t _UnpackHeader_DataT(FILE_HEADER& header, const CLBYTE* pBuffer)
    {
      if((header.dwFlags & HeaderFlag_DataFieldMask) == HeaderFlag_DataField_64KB) {
        typedef FILE_HEADERT<_key_t, _name_t, u16> HEADER_INST;
        const HEADER_INST* packed_header = reinterpret_cast<const HEADER_INST*>(pBuffer);
        return packed_header->CopyTo(header);
      }
      else
      {
        typedef FILE_HEADERT<_key_t, _name_t, u32> HEADER_INST;
        const HEADER_INST* packed_header = reinterpret_cast<const HEADER_INST*>(pBuffer);
        return packed_header->CopyTo(header);
      }
    }

    template<typename _key_t> 
    size_t _UnpackHeader_NameDataT(FILE_HEADER& header, const CLBYTE* pBuffer)
    {
      if((header.dwFlags & HeaderFlag_NameFieldMask) == HeaderFlag_NameField_1Byte) {
        return _UnpackHeader_DataT<_key_t, u8>(header, pBuffer);
      } else if((header.dwFlags & HeaderFlag_NameFieldMask) == HeaderFlag_NameField_2Bytes) {
        return _UnpackHeader_DataT<_key_t, u16>(header, pBuffer);
      }
      ASSERT((header.dwFlags & HeaderFlag_NameFieldMask) == HeaderFlag_NameField_4Bytes);
      return _UnpackHeader_DataT<_key_t, u32>(header, pBuffer);
    }

    size_t _UnpackHeader( FILE_HEADER& header, const CLBYTE* pBuffer )
    {
      if((header.dwFlags & HeaderFlag_KeyFieldMask) == HeaderFlag_KeyField_1Byte) {
        return _UnpackHeader_NameDataT<u8>(header, pBuffer);
      } else if((header.dwFlags & HeaderFlag_KeyFieldMask) == HeaderFlag_KeyField_1Byte) {
        return _UnpackHeader_NameDataT<u16>(header, pBuffer);
      }
      return _UnpackHeader_NameDataT<u32>(header, pBuffer);
    }


  } // namespace

  template<typename T_LPCSTR>
  b32 Repository::LoadFromFileT(T_LPCSTR szFilename)
  {
    File file;
    if( ! file.OpenExisting(szFilename)) {
      // ERROR:�޷����ļ�
      return false;
    }

    if( ! file.ReadToBuffer(&m_Buffer)) {
      // ERROR:�޷���ȡ�ļ�
      return false;
    }

    return _ParseFromBuffer();
  }

  b32 Repository::_WriteToFile(File& file) const
  {
    FILE_HEADER header = {};
    header.dwMagic = CLMAKEFOURCC('C','L','R','P');
    header.nKeys = (CLDWORD)(m_pKeysEnd - m_pKeys);
    header.cbNames = (CLDWORD)(m_pNamesEnd - GetNamesBegin()) * sizeof(TChar);
    header.cbData = (CLDWORD)m_cbDataLen;

    CLBYTE packed_header[sizeof(FILE_HEADER)]; // ���Header�϶���չ����HeaderС
    size_t packed_header_size = _PackHeader(header, packed_header);

    file.Write(packed_header, (u32)packed_header_size);
    file.Write(m_pKeys, (u32)((size_t)m_pKeysEnd - (size_t)m_pKeys));
    file.Write(GetNamesBegin(), header.cbNames);
    file.Write(m_pData, (u32)m_cbDataLen);
    return TRUE;
  }

  b32 Repository::_DbgCheckDataOverlay() const
  {
    if((CLBYTE*)m_Buffer.GetPtr() + m_Buffer.GetSize() != m_pData + m_cbDataLen)
    {
      CLOG_ERROR("Buffer size doesn't match:\"m_Buffer.GetSize()\" is %d, m_cbDataLen is %d\n", 
        m_Buffer.GetSize(), m_pData - (CLBYTE*)m_Buffer.GetPtr() + m_cbDataLen);
      return FALSE;
    }

    KEY* pPrev = m_pKeys;
    while(pPrev < m_pKeysEnd) {
      if(pPrev->type == KeyType_Varible) {
        break;
      }
      pPrev++;
    }

    // û�ҵ�KeyType_Varible
    if(pPrev == m_pKeysEnd) {
      return TRUE;
    }

    for(KEY* pIter = pPrev + 1; pIter < m_pKeysEnd; ++pIter) {
      if(pIter->type != KeyType_Varible) {
        continue;
      }
      if(pPrev->v.offset + pPrev->v.length != pIter->v.offset) {
        return FALSE;
      }
      pPrev = pIter;
    }

    return TRUE;
  }

  Repository::Repository()
    : m_pKeys    (NULL)
    , m_pKeysEnd (NULL)
    , m_pKeysCapacity(NULL)
    , m_pNamesEnd  (NULL)
    , m_pData     (NULL)
    , m_cbDataLen (0)
  {
  }

  Repository::~Repository()
  {
  }

  Repository& Repository::operator=(const Repository& repo)
  {
    m_Buffer.Replace(0, m_Buffer.GetSize(), repo.m_Buffer.GetPtr(), repo.m_Buffer.GetSize());

    m_pKeys         = repo.m_pKeys;
    m_pKeysEnd      = repo.m_pKeysEnd;
    m_pKeysCapacity = repo.m_pKeysCapacity;
    m_pNamesEnd     = repo.m_pNamesEnd;
    m_pData         = repo.m_pData;
    m_cbDataLen     = m_cbDataLen;

    const size_t delta = (size_t)m_Buffer.GetPtr() - (size_t)repo.m_Buffer.GetPtr();
    _Locate(delta);
    return *this;
  }

  Repository::iterator Repository::begin() const
  {
    iterator it(this, m_pKeys);
    return it;
  }

  Repository::iterator Repository::end() const
  {
    iterator it(this, m_pKeysEnd);
    return it;
  }

  void Repository::Clear()
  {
    m_Buffer.Resize(0, FALSE);
    m_pKeys         = NULL;
    m_pKeysEnd      = NULL;
    m_pKeysCapacity = NULL;
    m_pNamesEnd     = NULL;
    m_pData         = NULL;
    m_cbDataLen     = 0;
  }

  b32 Repository::_ParseFromBuffer()
  {
    size_t pData = reinterpret_cast<size_t>(m_Buffer.GetPtr());
    FILE_HEADER header = { 0, ((CLDWORD*)pData)[1] }; // ��Ҫ����dwFlags��Ϊ��ѹ����
    size_t packed_header_size = _UnpackHeader(header, (const CLBYTE*)pData);
    if(header.dwMagic != CLMAKEFOURCC('C','L','R','P')) {
      // ERROR: bad file magic
      return false;
    }
    pData += packed_header_size;

    m_pKeys = reinterpret_cast<KEY*>(pData);
    m_pKeysEnd = m_pKeys + header.nKeys;
    m_pKeysCapacity = m_pKeysEnd;

    m_pNamesEnd = reinterpret_cast<LPCSTR>(reinterpret_cast<size_t>(m_pKeysEnd) + header.cbNames);
    m_pData     = (CLBYTE*)m_pNamesEnd;
    m_cbDataLen = header.cbData;

    size_t nCheckLen = packed_header_size + sizeof(KEY) * header.nKeys + header.cbNames + header.cbData;
    if(nCheckLen > m_Buffer.GetSize()) {
      // ERROR: not enough data
      return false;
    }
    else if(nCheckLen < m_Buffer.GetSize())
    {
      // WARNING: has redundancy data
      return true;
    }
    return true;
  }

  Repository::KEY* Repository::_FindKey(LPCSTR szKey) const
  {
    LPCSTR szNamesFront = GetNamesBegin();
    KEY*const result =
      clstd::BinarySearch(m_pKeys, m_pKeysEnd, szKey,
      [szNamesFront](KEY* pMid, LPCSTR key) -> int
    {
      return strcmpT(pMid->GetName(szNamesFront), key);
    });
    return result;
  }

  size_t Repository::_ResizeGlobalBuf(size_t delta, KEY*& rpKey)
  {
    size_t mem_delta = m_Buffer.Resize(m_Buffer.GetSize() + delta, FALSE);
    if(mem_delta) {
      _Locate(mem_delta);
      rpKey = (KEY*)((size_t)rpKey + mem_delta);
    }
    return mem_delta;
  }

  u32 Repository::_GetSeqOffset(KEY* pPos)
  {
    ASSERT(pPos >= m_pKeys && pPos <= m_pKeysEnd);

    while(pPos > m_pKeys) {
      if((--pPos)->type == KeyType_Varible) {
        return (pPos->v.offset + pPos->v.length);
      }
    }
    return 0;
  }

  LPCSTR Repository::GetNamesBegin() const
  {
    return reinterpret_cast<LPCSTR>(m_pKeysCapacity);
  }

  LPCSTR Repository::GetNamesCapacity() const
  {
    return reinterpret_cast<LPCSTR>(m_pData);
  }

  size_t Repository::GetDataCapacity() const
  {
    return ((size_t)m_Buffer.GetPtr() + m_Buffer.GetSize()) - (size_t)m_pData;
  }

  Repository::KEYPAIR* Repository::_PreInsertKey(KEYPAIR* pPair, LPCSTR szKey)
  {
    LPCSTR szNamesFront = GetNamesBegin();
    pPair->pTable =
      clstd::BinaryInsertPos(m_pKeys, m_pKeysEnd, szKey, &pPair->bInsert,
      [szNamesFront](KEY* pMid, LPCSTR key) -> int
    {
        return strcmpT(pMid->GetName(szNamesFront), key);
    });
    return pPair;
  }

  void Repository::_Locate(size_t memdelta)
  {
    m_pKeys         = (KEY*)((size_t)m_pKeys + memdelta);
    m_pKeysEnd      = (KEY*)((size_t)m_pKeysEnd + memdelta);
    m_pKeysCapacity = (KEY*)((size_t)m_pKeysCapacity + memdelta);
    m_pNamesEnd     = (LPCSTR)((size_t)m_pNamesEnd + memdelta);
    m_pData         = (CLBYTE*)((size_t)m_pData + memdelta);
  }

  b32 Repository::_AllocKey(size_t cbKeyNameLen/*, const void* pData*/, size_t nLength, KEY*& rPos)
  {
    size_t extened = 0;
    size_t key_inc = 0;
    size_t name_inc = 0;
    ASSERT(m_pKeysEnd <= m_pKeysCapacity);
    ASSERT(m_pNamesEnd <= GetNamesCapacity());

    if(m_pKeysEnd == m_pKeysCapacity) {
      key_inc = KEYINCCOUNT * sizeof(KEY);
      extened += key_inc;
    }

    if((size_t)m_pNamesEnd + cbKeyNameLen > (size_t)GetNamesCapacity()) {
      name_inc = (cbKeyNameLen + NAMEINCCOUNT * sizeof(TChar));
      extened += name_inc;
    }

    if(m_cbDataLen + nLength > GetDataCapacity()) {
      extened += nLength;
    }

    if(extened > 0) {
      _ResizeGlobalBuf(extened, rPos);

      if(name_inc > 0) {
        // ע��Ƕ�׵���
        memcpy(m_pData + key_inc + name_inc, m_pData, m_cbDataLen);

        if(key_inc > 0) {
          const size_t cbNamesLen = (GetNamesCapacity() - GetNamesBegin()) * sizeof(TChar);
          memcpy((CLBYTE*)GetNamesBegin() + key_inc, GetNamesBegin(), cbNamesLen);
          m_pKeysCapacity = (KEY*)((CLBYTE*)m_pKeysCapacity + key_inc);
        }

        m_pData += (key_inc + name_inc);
      }
      else if(key_inc > 0) {
        const size_t cbNamesLen = (GetNamesCapacity() - GetNamesBegin()) * sizeof(TChar);
        memcpy((CLBYTE*)GetNamesBegin() + key_inc, GetNamesBegin(), cbNamesLen + m_cbDataLen);
        m_pKeysCapacity = (KEY*)((CLBYTE*)m_pKeysCapacity + key_inc);
      }
      return TRUE;
    }
    //m_cbDataLen += nLength;

    return FALSE;
  }

  b32 Repository::_InsertKey( KEY* pPos, LPCSTR szKey, const void* pData, size_t nLength )
  {
    ASSERT(pPos >= m_pKeys && pPos <= m_pKeysEnd);
    const size_t cbKeyNameLen = ALIGN_2((strlenT(szKey) + 1) * sizeof(TChar));// ���Ͻ�β'\0'���������ֽڶ���
    if(nLength < OCTETSIZE)
    {

      _AllocKey(cbKeyNameLen, 0, pPos);

      ASSERT(m_pKeysEnd < m_pKeysCapacity);

      const size_t cbKeyLen = (m_pKeysEnd - pPos) * sizeof(KEY);
      if(cbKeyLen) {
        memcpy(pPos + 1, pPos, cbKeyLen);
      }
      m_pKeysEnd++;
      pPos->name = (u32)(m_pNamesEnd - GetNamesBegin());
      pPos->type = (KeyType)(KeyType_Octet_0 + nLength);
      memcpy(pPos->o.data, pData, nLength);
    }
    else
    {
      _AllocKey(cbKeyNameLen, nLength, pPos);

      ASSERT(m_pKeysEnd < m_pKeysCapacity);

      const size_t cbKeyLen = (m_pKeysEnd - pPos) * sizeof(KEY);
      if(cbKeyLen) {
        memcpy(pPos + 1, pPos, cbKeyLen);
      }
      m_pKeysEnd++;
      pPos->name = (u32)(m_pNamesEnd - GetNamesBegin());
      pPos->type = KeyType_Varible;
      pPos->v.offset = _GetSeqOffset(pPos);
      pPos->v.length = 0;

      ASSERT((CLBYTE*)m_Buffer.GetPtr() + m_Buffer.GetSize() == m_pData + m_cbDataLen + nLength);
      _ResizeKey(pPos, nLength);
      ASSERT(_DbgCheckDataOverlay());
      memcpy(m_pData + pPos->v.offset, pData, nLength);
    }

    // Copy name
    CLBYTE*const pName = (CLBYTE*)GetNamesBegin() + pPos->name;
    memset(pName, 0, cbKeyNameLen);
    strcpyT((TChar*)pName, szKey);
    m_pNamesEnd = (LPCSTR)((CLBYTE*)m_pNamesEnd + cbKeyNameLen);

    return true;
  }

  void Repository::_ResizeKey(KEY*& rKey, size_t delta)
  {
    ASSERT(rKey->type == KeyType_Varible);
    const size_t nDataEndian = rKey->v.offset + rKey->v.length;
    CLBYTE*const pDataEndian = m_pData + nDataEndian;

    // TODO: �����֤����˳����Key˳��һ�µĻ�pIter���Դ�rKey��ʼ
    for(KEY* pIter = m_pKeys; pIter < m_pKeysEnd; ++pIter)
    {
      if(TEST_FLAG_NOT(pIter->type, KeyType_Octet) && pIter != rKey && pIter->v.offset >= rKey->v.offset) {
        ASSERT(rKey->v.offset + rKey->v.length <= pIter->v.offset); // ������ݸ���
        pIter->v.offset += static_cast<u32>(delta);
      }
    }
    rKey->v.length += (u32)delta;
    //return m_pData + rKey->v.offset + rKey->v.length; // ���ص�ǰ��ֵ���ݵĽ�β
    if(m_cbDataLen > nDataEndian) {
      memcpy(pDataEndian + delta, pDataEndian, m_cbDataLen - nDataEndian);
    }
    m_cbDataLen += delta;
  }

  b32 Repository::_ReallocData(KEY*& rKey, size_t new_length)
  {
    // û�д���KT_Octet_0 ~ KT_Octet_8���
    ASSERT(rKey->type == KeyType_Varible);
    ASSERT((CLBYTE*)m_Buffer.GetPtr() + m_Buffer.GetSize() == m_pData + m_cbDataLen);

    size_t delta = new_length - static_cast<size_t>(rKey->GetDataSize());
    //const size_t nDataEndian = rKey->v.offset + rKey->v.length;
    //CLBYTE*const pDataEndian = m_pData + nDataEndian;

    if(rKey->v.length < new_length)
    {
      _ResizeGlobalBuf(delta, rKey);

      _ResizeKey(rKey, delta);
      //memcpy(pDataEndian + delta, pDataEndian, m_cbDataLen - nDataEndian);

      //rKey->v.length += delta;
    }
    else if(rKey->v.length > new_length)
    {
      //CLBYTE*const pDataEndian = m_pData + nDataEndian;
      _ResizeKey(rKey, delta);
      //memcpy(pDataEndian + delta, pDataEndian, m_cbDataLen - nDataEndian);

      _ResizeGlobalBuf(delta, rKey);
    }
    else {
      CLBREAK; // ����������洦������������
    }

    //m_cbDataLen += delta;

    ASSERT((CLBYTE*)m_Buffer.GetPtr() + m_Buffer.GetSize() == m_pData + m_cbDataLen);
    return TRUE;
  }

  b32 Repository::_ReplaceKey(KEY* pPos, LPCSTR szKey, const void* pData, size_t nLength)
  {
    ASSERT(pPos >= m_pKeys && pPos <= m_pKeysEnd);
    if(nLength <= OCTETSIZE)
    {
      if(TEST_FLAG(pPos->type, KeyType_Octet)) {
        pPos->type = (KeyType)(KeyType_Octet + nLength);
        memcpy(pPos->o.data, pData, nLength);
        ASSERT(_DbgCheckDataOverlay());
        return TRUE;
      }
      else if(pPos->type == KeyType_Varible) {
        const size_t key_delta = 0 - pPos->v.length;
        _ResizeKey(pPos, key_delta);
        _ResizeGlobalBuf(key_delta, pPos);
        pPos->type = (KeyType)(KeyType_Octet + nLength);
        memcpy(pPos->o.data, pData, nLength);
        ASSERT(_DbgCheckDataOverlay());
        return TRUE;
      }
      else {
        CLBREAK;
      }
    }
    else
    {
      if(TEST_FLAG(pPos->type, KeyType_Octet)) {
        pPos->type = KeyType_Varible;
        pPos->v.offset = _GetSeqOffset(pPos);
        pPos->v.length = 0;

        if(_ReallocData(pPos, nLength)) {
          memcpy(m_pData + pPos->v.offset, pData, nLength);
          ASSERT(_DbgCheckDataOverlay());
          return TRUE;
        }

      }
      else if(pPos->type, KeyType_Varible) {
        if(pPos->v.length == nLength || _ReallocData(pPos, nLength)) {
          memcpy(m_pData + pPos->v.offset, pData, nLength);
          ASSERT(_DbgCheckDataOverlay());
          return TRUE;
        }
      }
      else {
        CLBREAK;
      }
    }

    return FALSE;
  }


  void Repository::_Initialize(size_t cbDataLen)
  {
    ASSERT(m_pKeys == NULL);
    m_Buffer.Resize(sizeof(KEY) * KEYINCCOUNT + sizeof(TChar) * NAMEINCCOUNT + cbDataLen, TRUE);
    m_pKeys         = reinterpret_cast<KEY*>(m_Buffer.GetPtr());
    m_pKeysEnd      = m_pKeys;
    m_pKeysCapacity = m_pKeys + KEYINCCOUNT;

    m_pNamesEnd     = GetNamesBegin();
    m_pData         = (CLBYTE*)GetNamesBegin() + sizeof(TChar) * NAMEINCCOUNT;          // Name capacity
    m_cbDataLen     = 0;
  }

  b32 Repository::LoadFromFile( CLLPCSTR szFilename )
  {
    return LoadFromFileT(szFilename);
  }

  b32 Repository::LoadFromFile( CLLPCWSTR szFilename )
  {
    return LoadFromFileT(szFilename);
  }

  b32 Repository::LoadFromMemory( const void* pData, size_t nLength )
  {
    m_Buffer.Append(pData, nLength);
    return _ParseFromBuffer();
  }

  b32 Repository::SaveToFile( CLLPCSTR szFilename ) const
  {
    File file;
    if(file.CreateAlways(szFilename)) {
      return _WriteToFile(file);
    }
    return false;
  }

  b32 Repository::SaveToFile( CLLPCWSTR szFilename ) const
  {
    File file;
    if(file.CreateAlways(szFilename)) {
      return _WriteToFile(file);
    }
    return false;
  }


  b32 Repository::SaveToMemory(MemBuffer& buffer) const
  {
    FILE_HEADER header = {};
    header.dwMagic = CLMAKEFOURCC('C', 'L', 'R', 'P');
    header.nKeys = (CLDWORD)(m_pKeysEnd - m_pKeys);
    header.cbNames = (CLDWORD)(m_pNamesEnd - GetNamesBegin()) * sizeof(TChar);
    header.cbData = (CLDWORD)m_cbDataLen;

    CLBYTE packed_header[sizeof(FILE_HEADER)]; // ���Header�϶���չ����HeaderС
    size_t packed_header_size = _PackHeader(header, packed_header);

    buffer.Reserve(packed_header_size + (m_pKeysEnd - m_pKeys) * sizeof(KEY) +
      header.cbNames + m_cbDataLen);

    buffer.Append(packed_header, (u32)packed_header_size);
    buffer.Append(m_pKeys, (u32)((size_t)m_pKeysEnd - (size_t)m_pKeys));
    buffer.Append(GetNamesBegin(), header.cbNames);
    buffer.Append(m_pData, (u32)m_cbDataLen);
    return TRUE;
  }

  size_t Repository::GetNumOfKeys() const
  {
    return m_pKeysEnd - m_pKeys;
  }

  b32 Repository::SetKey( LPCSTR szKey, const void* pData, size_t nLength )
  {
    if(m_Buffer.GetSize() == 0) {
      _Initialize(nLength > OCTETSIZE ? nLength : 0);
      return _InsertKey(m_pKeys, szKey, pData, nLength);
    }

    KEYPAIR pair;
    if(_PreInsertKey(&pair, szKey)->bInsert) {
      return _InsertKey(pair.pTable, szKey, pData, nLength);
    }
    else {
      return _ReplaceKey(pair.pTable, szKey, pData, nLength);
    }
    return false;
  }

  size_t Repository::GetKey( LPCSTR szKey, void* pData, size_t nLength ) const
  {
    KEY* pKey = _FindKey(szKey);
    if(pKey) {
      size_t copy_size = 0;
      CLBYTE*const pSrcData = pKey->GetDataPtr(m_pData, &copy_size);

      if(pData == NULL || nLength == NULL) {
        return copy_size;
      }

      copy_size = clMin(copy_size, nLength);
      memcpy(pData, pSrcData, copy_size);
      return copy_size;
    }
    return 0;
  }

  void* Repository::GetDataPtr( LPCSTR szKey, size_t* pLength ) const
  {
    KEY* pKey = _FindKey(szKey);
    if(pKey) {
      size_t len = 0;
      return pKey->GetDataPtr(m_pData, pLength ? pLength : &len);
    }
    return NULL;
  }

  //b32 Repository::RemoveKey( LPCSTR szKey )
  //{
  //  return false;
  //}

  //b32 Repository::RemoveKey( CLDWORD dwKey )
  //{
  //  return false;
  //}

  //size_t Repository::GetKey( CLDWORD szKey, void* pData, size_t nLength ) const
  //{
  //  return false;
  //}

  //b32 Repository::SetKey( CLDWORD szKey, const void* pData, size_t nLength )
  //{
  //  return false;
  //}

  Repository::iterator::iterator( const Repository* _repo, KEY* _key )
    : repo(_repo)
    , key(_key)
  {    
  }

  LPCSTR Repository::iterator::name() const
  {
    return key->GetName(repo->GetNamesBegin());
  }

  void* Repository::iterator::ptr() const
  {
    return key->GetDataPtr(repo->m_pData);
  }

  size_t Repository::iterator::size() const
  {
    return key->GetDataSize();
  }

  Repository::iterator& Repository::iterator::operator++()
  {
    ++key;
    return *this;
  }

  Repository::iterator& Repository::iterator::operator++(int)
  {
    return operator++();
  }

  b32 Repository::iterator::operator==(const iterator& it)
  {
    ASSERT(repo == it.repo);
    return key == it.key;
  }

  b32 Repository::iterator::operator!=(const iterator& it)
  {
    return ! operator==(it);
  }

  b32 Repository::iterator::operator==(LPCSTR szKey)
  {
    return strcmpT(name(), szKey) == 0;
  }

  b32 Repository::iterator::operator!=(LPCSTR szKey)
  {
    return ! operator==(szKey);
  }

  //////////////////////////////////////////////////////////////////////////
  LPCSTR Repository::KEY::GetName(LPCSTR szFirstName) const
  {
    return szFirstName + name;
  }

  CLBYTE* Repository::KEY::GetDataPtr(CLBYTE* pDataBase, size_t* pSizeOut) const
  {
    if(TEST_FLAG(type, KeyType_Octet)) {
      *pSizeOut = (size_t)(type - KeyType_Octet_0);
      return (CLBYTE*)o.data;
    } else if(type == KeyType_Varible) {
      *pSizeOut = v.length;
      return pDataBase + v.offset;
    }
    CLBREAK;
    return NULL;
  }

  CLBYTE* Repository::KEY::GetDataPtr(CLBYTE* pDataBase) const
  {
    if(TEST_FLAG(type, KeyType_Octet)) {
      return (CLBYTE*)o.data;
    } else if(type == KeyType_Varible) {
      return pDataBase + v.offset;
    }
    CLBREAK;
    return NULL;
  }

  size_t Repository::KEY::GetDataSize() const
  {
    if(TEST_FLAG(type, KeyType_Octet)) {
      return (size_t)(type - KeyType_Octet_0);
    } else if(type == KeyType_Varible) {
      return v.length;
    }
    CLBREAK;
    return 0;
  }
  //////////////////////////////////////////////////////////////////////////

} // namespace clstd