#include <windows.h>

#include <string>

#include "HmFilter.h"

extern "C"
{
  char* __stdcall AStyleGetVersion();
  char* __stdcall AStyleMain( const char* textIn,
                              const char* options,
                              void( __stdcall *errorHandler )( int, char* ),
                              char*( __stdcall *memoryAlloc )( unsigned long ) );
}

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD ul_reason_for_call,
                       LPVOID lpReserved )
{
  return TRUE;
}

struct HIDEMARUFILTERINFO aFilterInfo[] =
{
  { sizeof( HIDEMARUFILTERINFO ), "Astyle", "Artistic Style", "Artistic Style", 'A', 1, 0, 0 },
  { 0, NULL, NULL, NULL, NULL, 0, 0, 0 }
};

extern "C" __declspec( dllexport ) HIDEMARUFILTERINFO* _cdecl EnumHidemaruFilter()
{
  return aFilterInfo;
}

extern "C" __declspec( dllexport ) DWORD _cdecl HidemaruFilterGetVersion()
{
  return ( 1 << 16 ) + ( 0 << 8 ) + 2;  // 1.02
}

static void __stdcall errorHandler( int errorNumber, char* errorMessage )
{
}

static char* __stdcall memoryAlloc( unsigned long memoryNeeded )
{
  return new char [memoryNeeded];
}

static std::string makeOptions( const int tab_num,
                                const bool space_indent,
                                char* pszParam,
                                int cbParamBuffer )
{
  std::string options;
  
  if ( space_indent )
  {
    options += "indent=spaces=";
  }
  else
  {
    options += "indent=tab=";
  }
  
  char buf[64];
  sprintf_s( buf, sizeof( buf ), "%d", tab_num );
  options += buf;
  
  options += ' ';
  options += pszParam;
  
  return options;
}

static char* to_utf8( const WCHAR* pwsz )
{
  const int wsz_len = wcslen( pwsz );
  
  const int buf_len = WideCharToMultiByte(
                        CP_UTF8, 0, pwsz, wsz_len,
                        NULL, 0, NULL, NULL );
  if ( buf_len <= 0 )
  {
    return NULL;
  }
  
  char* buf = new char[buf_len + 1];
  
  if ( WideCharToMultiByte( CP_UTF8, 0, pwsz, wsz_len,
                            buf, buf_len, NULL, NULL ) == 0 )
  {
    delete[] buf;
    return NULL;
  }
  
  buf[buf_len] = '\0';
  
  return buf;
}

static size_t calcIndent( const int tab_num, const WCHAR* pwsz )
{
  if ( tab_num == 0 )
  {
    return 0;
  }
  
  size_t column = 0;
  {
    const WCHAR* ptr = pwsz;
    for ( size_t i = 0; ; ++i )
    {
      const WCHAR ch = ptr[i];
      if ( ch == L' ' )
      {
        ++column;
      }
      else if ( ch == L'\t' )
      {
        column = ( ( column / tab_num ) + 1 ) * tab_num;
      }
      else
      {
        break;
      }
    }
  }
  
  return column / tab_num;
}

class LineIterator
{
public:
  LineIterator( const char* pwsz )
      : m_pwsz( pwsz ), m_cur( 0 ),
      eolWindows( 0 ), eolMacOld( 0 ), eolLinux( 0 ),
      m_pwsz_len( strlen( m_pwsz ) )
  {
    setOutputEOL();
  }
  
public:
  bool hasMoreLines() const
  {
    if ( m_cur > m_pwsz_len )
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  
  virtual WCHAR* nextLine()
  {
    const size_t start = m_cur;
    size_t len = 0;
    while ( true )
    {
      const WCHAR ch = m_pwsz[start + len];
      if ( ch == L'\0' )
      {
        if ( len == 0 )
        {
          m_cur = start + len + 1;
        }
        else
        {
          m_cur = start + len;
        }
        break;
      }
      else
      {
        const WCHAR next_ch = m_pwsz[start + len + 1];
        if ( ch == L'\x0D' )
        {
          if ( next_ch == L'\x0A' )
          {
            ++eolWindows;
          }
          else
          {
            ++eolMacOld;
          }
          m_cur = start + len + 2;
          break;
        }
        if ( ch == L'\x0A' )
        {
          ++eolLinux;
          m_cur = start + len + 1;
          break;
        }
        else
        {
          ++len;
        }
      }
    }
    
    setOutputEOL();
    
    const int buf_len = MultiByteToWideChar( CP_UTF8, 0, m_pwsz + start, len, NULL, 0 );
    if ( buf_len <= 0 )
    {
      return NULL;
    }
    
    WCHAR* buf = new WCHAR[buf_len + 1];
    
    if ( MultiByteToWideChar( CP_UTF8, 0, m_pwsz + start, len, buf, buf_len ) == 0 )
    {
      delete[] buf;
      return NULL;
    }
    
    buf[buf_len] = '\0';
    
    return buf;
  }
  
private:
  void setOutputEOL()
  {
    if ( eolWindows >= eolLinux )
    {
      if ( eolWindows >= eolMacOld )
      {
        wcscpy_s( outputEOL, 4, L"\r\n" );  // Windows (CR+LF)
      }
      else
      {
        wcscpy_s( outputEOL, 4, L"\r" );  // MacOld (CR)
      }
    }
    else
    {
      if ( eolLinux >= eolMacOld )
      {
        wcscpy_s( outputEOL, 4, L"\n" );  // Linux (LF)
      }
      else
      {
        wcscpy_s( outputEOL, 4, L"\r" );  // MacOld (CR)
      }
    }
  }
  
private:
  int eolWindows;
  int eolLinux;
  int eolMacOld;
  
public:
  WCHAR outputEOL[4];
  
private:
  size_t m_cur;
  const char *m_pwsz;
  const size_t m_pwsz_len;
};

static std::wstring makeIndentString( const size_t indent,
                                      const int tab_num,
                                      const bool space_indent )
{
  std::wstring indent_str;
  
  for ( size_t i = 0; i < indent; ++i )
  {
    if ( space_indent )
    {
      for ( size_t space = 0; space < static_cast<size_t>( tab_num ); ++space )
      {
        indent_str += L' ';
      }
    }
    else
    {
      indent_str += L'\t';
    }
  }
  
  return indent_str;
}

extern "C" __declspec( dllexport ) HGLOBAL _cdecl Astyle(
  HWND hwndHidemaru,
  WCHAR* pwsz,
  char* pszParam,
  int cbParamBuffer )
{
  char* textIn = to_utf8( pwsz );
  if ( !textIn )
  {
    return NULL;
  }
  
  const int tab_num = ( int )SendMessage(
                        hwndHidemaru, WM_HIDEMARUINFO,
                        HIDEMARUINFO_GETTABWIDTH, 0 );
  const bool space_indent = ( SendMessage(
                                hwndHidemaru, WM_HIDEMARUINFO,
                                HIDEMARUINFO_GETSPACETAB, 0 ) == 1 );
  const std::string options = makeOptions( tab_num, space_indent, pszParam, cbParamBuffer );
  
  char* textOut = AStyleMain( textIn, options.c_str(), errorHandler, memoryAlloc );
  
  delete[] textIn;
  textIn = NULL;
  
  const size_t indent = calcIndent( tab_num, pwsz );
  const bool fillemptylines = ( options.find( "fill-empty-lines" ) != std::string::npos );
  const std::wstring indent_str = makeIndentString( indent, tab_num, space_indent );
  
  std::wstring out;
  {
    LineIterator itr( textOut );
    bool bFirst = true;
    while ( itr.hasMoreLines() )
    {
      if ( bFirst )
      {
        WCHAR* str = itr.nextLine();
        
        if ( fillemptylines || ( str && ( str[0] != L'\0' ) ) )
        {
          out += indent_str;
        }
        
        if ( str )
        {
          out += str;
        }
        
        delete[] str;
        
        bFirst = false;
      }
      else
      {
        out += itr.outputEOL;
        
        WCHAR* str = itr.nextLine();
        
        if ( itr.hasMoreLines() )
        {
          if ( fillemptylines || ( str && ( str[0] != L'\0' ) ) )
          {
            out += indent_str;
          }
          
          if ( str )
          {
            out += str;
          }
        }
        
        delete[] str;
      }
    }
  }
  
  delete[] textOut;
  textOut = NULL;
  
  const size_t size = ( out.length() + 1 ) * sizeof( WCHAR );
  HGLOBAL hmemDest = GlobalAlloc( GMEM_MOVEABLE, size );
  void* pwDest = GlobalLock( hmemDest );
  memcpy( pwDest, out.c_str(), size );
  GlobalUnlock( hmemDest );
  
  return hmemDest;
}
